#include "net/EventLoop.h"
#include "net/Channel.h"
#include "base/Utils.h"
#include "base/Log.h"
#include "base/Backtrace.h"
#include "base/Error.h"
#include "net/Epoller.h"
#include "net/EventFd.h"
#include "net/TimerQueue.h"

#include <array>
#include <cstdint>
#include <ctime>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

extern "C" {
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
}

#ifdef TAG
#undef TAG
#endif
static constexpr std::string_view TAG = "EventLoop";

using namespace utils;
using namespace std;

// static check
static_assert(EPOLLIN == POLLIN,        "epoll uses same flag values as poll");
static_assert(EPOLLPRI == POLLPRI,      "epoll uses same flag values as poll");
static_assert(EPOLLOUT == POLLOUT,      "epoll uses same flag values as poll");
static_assert(EPOLLRDHUP == POLLRDHUP,  "epoll uses same flag values as poll");
static_assert(EPOLLERR == POLLERR,      "epoll uses same flag values as poll");
static_assert(EPOLLHUP == POLLHUP,      "epoll uses same flag values as poll");

namespace net {

/************************************************************/
/********************** EventLoop ***************************/
/************************************************************/
thread_local int EventLoop::tCurrentLoopTid = -1;

EventLoop::EventLoop() {
    LOG_INFO("%s +", __FUNCTION__);
    assertTrue(tCurrentLoopTid == -1, "Every thread can hold only one event loop!");

    tCurrentLoopTid = ::gettid();
    LOG_INFO("%s: tCurrentLoopTid=%d, EventLoop=%p", __FUNCTION__, tCurrentLoopTid, this);

    try {
        mpPoller = Epoller::createEpoller();
        // TODO:
        // disable now
        // mpTimerQueue = TimerQueue::createTimerQueue(this);
        mpWakeupFd = EventFd::createEventFd();
        mpWakeupChannel = Channel::createChannel(mpWakeupFd->getFd(), this);
        mpWakeupChannel->setReadCallback([&] {
            LOG_INFO("handleRead");
            mpWakeupFd->handleRead(); 
        });
        mpWakeupChannel->enableRead();
    } catch (SystemException& e) {
        LOG_ERR("%s", e.what());
        printBacktrace();
        throw;
    }

    // EventLoop not start now, set mIsExit true
    mIsExit = true;
    mIsLoopingNow = false;
    mIsDoPendingWorks = false;
    LOG_INFO("%s -", __FUNCTION__);
}

EventLoop::~EventLoop() {
    assertInLoopThread();
    // Restore thread state.
    LOG_INFO("%s +", __FUNCTION__);
    mpWakeupChannel = nullptr;
    mpWakeupFd = nullptr;
    mpTimerQueue = nullptr;
    mpPoller = nullptr;
    tCurrentLoopTid = -1;
    LOG_INFO("%s -", __FUNCTION__);
}

void EventLoop::addChannel(Channel *channel) {
    LOG_INFO("%s: channel %p, fd %d", __FUNCTION__, channel, channel->getFd());
    assertInLoopThread();
    mpPoller->addChannel(channel);
}

void EventLoop::removeChannel(Channel *channel) {
    LOG_INFO("%s: channel %p, fd %d", __FUNCTION__, channel, channel->getFd());
    assertInLoopThread();
    if (mIsLoopingNow) {
        assertTrue(mpCurrentChannel == channel, "Channel can only remove by itself!");
    }
    mpPoller->removeChannel(channel);
}

void EventLoop::updateChannel(Channel *channel) {
    LOG_INFO("%s: channel %p, fd %d", __FUNCTION__, channel, channel->getFd());
    assertInLoopThread();
    mpPoller->updateChannel(channel);
}

void EventLoop::startLoop() {
    LOG_INFO("%s +", __FUNCTION__);
    assertInLoopThread();
    mIsExit = false;
    [[likely]]
    while (!mIsExit) {
        mIsLoopingNow = true;
        auto activeChannels = mpPoller->poll();
        [[unlikely]]
        if (activeChannels.size() == 0) {
            LOG_DEBUG("%s: wait timeout", __FUNCTION__);
            mIsLoopingNow = false;
        }
        for (auto* channel : activeChannels) {
            mpCurrentChannel = channel;
            LOG_DEBUG("%s: current channel %p. fd %d", __FUNCTION__, mpCurrentChannel, mpCurrentChannel->getFd());
            channel->handleEvent();
            mpCurrentChannel = nullptr;
        }
        mIsLoopingNow = false;
        doPendingTasks();
    }
    LOG_INFO("%s -", __FUNCTION__);
}

void EventLoop::quitLoop() {
    LOG_INFO("%s", __FUNCTION__);
    assertInLoopThread();
    mIsExit = true;
    if (!isInLoopThread()) {
        wakeup();
    }
}

void EventLoop::queueInLoop(std::function<void ()>&& cb) {
    if (isInLoopThread()) {
        mPendingTasks.push_back(std::move(cb));
    } else {
        std::lock_guard lock { mMutex };
        mPendingTasks.push_back(std::move(cb));
    }
    if (!isInLoopThread() || mIsDoPendingWorks) {
        wakeup();
    }
}

void EventLoop::runInLoop(std::function<void ()>&& cb) {
    if (isInLoopThread()) {
        cb();
    } else {
        queueInLoop(std::move(cb));
    }
}

void EventLoop::runAfter(std::function<void ()>&& cb, uint32_t microseconds) {
    auto runAfterUnlock = [&] {
        try {
            mpTimerQueue->addTimer(std::move(cb), TimerType::Relative, microseconds);
        } catch (SystemException& e) {
            printBacktrace();
            LOG_ERR("System Exception %s", e.what());
        } catch (...) {
            printBacktrace();
            throw ;
        }
    };
    runInLoop(std::move(runAfterUnlock));
}

bool EventLoop::isInLoopThread() {
    thread_local static int tCachedCurrentTid = -1;
    if (tCachedCurrentTid < 0) {
        tCachedCurrentTid = ::gettid();
    }
    return (tCachedCurrentTid == tCurrentLoopTid);
}

void EventLoop::assertInLoopThread() {
//#ifdef DEBUG_BUILD
#if 1
    if (!isInLoopThread()) {
        LOG_FATAL("assertInLoopThread failed!");
    }
#endif
}

void EventLoop::wakeup() {
    LOG_DEBUG("%s", __FUNCTION__);
    mpWakeupFd->wakeup();
}

// TODO : Use thread-pool to complete pending tasks.
void EventLoop::doPendingTasks() {
    LOG_DEBUG("%s +", __FUNCTION__);
    std::vector<std::function<void ()>> pendingTasks;
    {
        std::lock_guard lock { mMutex };
        std::swap(pendingTasks, mPendingTasks);
    }
    mIsDoPendingWorks = true;
    LOG_DEBUG("%s :pendingTasks count: %d", __FUNCTION__, pendingTasks.size());
    for (auto&& task : pendingTasks) {
        task();
    }
    mIsDoPendingWorks = false;
    LOG_DEBUG("%s -", __FUNCTION__);
}

} // namespace net
