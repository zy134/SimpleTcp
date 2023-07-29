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
#include <chrono>
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

static thread_local EventLoop* tCurrentLoop = nullptr;

EventLoop::EventLoop() {
    LOG_INFO("{}: start", __FUNCTION__);
    assertTrue(tCurrentLoop == nullptr, "Every thread can hold only one event loop!");
    tCurrentLoop = this;

    LOG_INFO("{}: current thread EventLoop: {}", __FUNCTION__, static_cast<void *>(this));

    try {
        mpPoller = Epoller::createEpoller();
        mpTimerQueue = TimerQueue::createTimerQueue(this);
        mpWakeupFd = EventFd::createEventFd();
        mpWakeupChannel = Channel::createChannel(mpWakeupFd->getFd(), this);
        mpWakeupChannel->setReadCallback([&] {
            LOG_INFO("handleRead");
            mpWakeupFd->handleRead();
        });
        mpWakeupChannel->enableRead();
    } catch (SystemException& e) {
        LOG_ERR("{}", e.what());
        printBacktrace();
        tCurrentLoop = nullptr;
        throw;
    }

    // EventLoop not start now, set mIsExit true
    mIsExit = true;
    mIsLoopingNow = false;
    mIsDoPendingWorks = false;
    LOG_INFO("{}: end", __FUNCTION__);
}

EventLoop::~EventLoop() {
    assertInLoopThread();
    // Restore thread state.
    LOG_INFO("{}: start", __FUNCTION__);
    LOG_INFO("{}: current thread EventLoop: {}", __FUNCTION__, static_cast<void *>(this));
    mpWakeupChannel = nullptr;
    mpWakeupFd = nullptr;
    mpTimerQueue = nullptr;
    mpPoller = nullptr;
    tCurrentLoop = nullptr;
    LOG_INFO("{}: end", __FUNCTION__);
}

void EventLoop::addChannel(Channel *channel) {
    LOG_INFO("{}: channel {}, fd {}"
            , __FUNCTION__, static_cast<void *>(channel), channel->getFd());
    assertInLoopThread();
    mpPoller->addChannel(channel);
}

void EventLoop::removeChannel(Channel *channel) {
    LOG_INFO("{}: channel {}, fd {}"
            , __FUNCTION__, static_cast<void *>(channel), channel->getFd());
    assertInLoopThread();
    assertTrue(mIsLoopingNow == false, "Don't remove channel in loop! Should queueInLoop...");
    mpPoller->removeChannel(channel);
}

void EventLoop::updateChannel(Channel *channel) {
    LOG_INFO("{}: channel {}, fd {}"
            , __FUNCTION__, static_cast<void *>(channel), channel->getFd());
    assertInLoopThread();
    mpPoller->updateChannel(channel);
}

void EventLoop::startLoop() {
    LOG_INFO("{} start", __FUNCTION__);
    assertInLoopThread();
    mIsExit = false;
    [[likely]]
    while (!mIsExit) {
        mIsLoopingNow = true;
        auto activeChannels = mpPoller->poll();
        [[unlikely]]
        if (activeChannels.size() == 0) {
            LOG_DEBUG("{}: wait timeout", __FUNCTION__);
            mIsLoopingNow = false;
        }
        for (auto* channel : activeChannels) {
            mpCurrentChannel = channel;
            LOG_DEBUG("{}: current channel {}. fd {}"
                    , __FUNCTION__, static_cast<void *>(mpCurrentChannel), mpCurrentChannel->getFd());
            channel->handleEvent();
            mpCurrentChannel = nullptr;
        }
        mIsLoopingNow = false;
        // Do pending tasks after loop.
        doPendingTasks();
    }
    LOG_INFO("{} end", __FUNCTION__);
}

void EventLoop::quitLoop() {
    LOG_INFO("{}", __FUNCTION__);
    // assertInLoopThread();
    mIsExit = true;
    // Wakeup loop if not in loop thread.
    if (!isInLoopThread()) {
        wakeup();
    }
}

void EventLoop::queueInLoop(std::function<void ()>&& cb) {
    if (isInLoopThread()) {
        mPendingTasks.emplace_back(std::move(cb));
    } else {
        std::lock_guard lock { mMutex };
        mPendingTasks.emplace_back(std::move(cb));
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

TimerId EventLoop::runAfter(std::function<void ()>&& cb, std::chrono::microseconds delay) {
    LOG_DEBUG("{} E", __FUNCTION__);
    return mpTimerQueue->addOneshotTimer(std::move(cb), TimerType::Relative, delay);
    LOG_DEBUG("{} X", __FUNCTION__);
}

TimerId EventLoop::runEvery(std::function<void ()>&& cb, std::chrono::microseconds interval) {
    LOG_DEBUG("{} E", __FUNCTION__);
    return mpTimerQueue->addRepeatingTimer(std::move(cb), TimerType::Relative, interval);
    LOG_DEBUG("{} X", __FUNCTION__);
}

void EventLoop::removeTimer(TimerId timerId) {
    LOG_DEBUG("{} E", __FUNCTION__);
    mpTimerQueue->removeTimer(timerId);
    LOG_DEBUG("{} X", __FUNCTION__);
}

bool EventLoop::isInLoopThread() {
    return (tCurrentLoop != nullptr);
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
    LOG_DEBUG("{}", __FUNCTION__);
    mpWakeupFd->wakeup();
}

// TODO : Use thread-pool to complete pending tasks.
void EventLoop::doPendingTasks() {
    LOG_DEBUG("{} E", __FUNCTION__);
    std::vector<std::function<void ()>> pendingTasks;
    {
        std::lock_guard lock { mMutex };
        std::swap(pendingTasks, mPendingTasks);
    }
    mIsDoPendingWorks = true;
    LOG_DEBUG("{} :pendingTasks count: {}", __FUNCTION__, pendingTasks.size());
    for (auto&& task : pendingTasks) {
        task();
    }
    mIsDoPendingWorks = false;
    LOG_DEBUG("{} X", __FUNCTION__);
}

} // namespace net
