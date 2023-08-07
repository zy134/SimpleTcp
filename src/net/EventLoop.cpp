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
#include <ctime>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <future>

extern "C" {
#include <unistd.h>
}

static constexpr std::string_view TAG = "EventLoop";

using namespace simpletcp;
using namespace std;

namespace simpletcp::net {

/************************************************************/
/********************** EventLoop ***************************/
/************************************************************/

static thread_local EventLoop* tCurrentLoop = nullptr;
static thread_local int tCurrentLoopTid = -1;

EventLoop::EventLoop() {
    LOG_INFO("{}: E", __FUNCTION__);
    assertTrue(tCurrentLoop == nullptr, "Every thread can hold only one event loop!");
    tCurrentLoop = this;
    tCurrentLoopTid = static_cast<int>(::gettid());

    LOG_INFO("{}: current thread EventLoop: {}", __FUNCTION__, static_cast<void *>(this));
    LOG_INFO("{}: loop thread :{}", __FUNCTION__, tCurrentLoopTid);

    try {
        mpPoller = Epoller::createEpoller();
        mpTimerQueue = TimerQueue::createTimerQueue(this);
        mpWakeupFd = EventFd::createEventFd();
        mpWakeupChannel = Channel::createChannel(mpWakeupFd->getFd(), this);
        mpWakeupChannel->setChannelInfo("Wakeup channel");
        mpWakeupChannel->setReadCallback([&] {
            mpWakeupFd->handleRead();
        });
        mpWakeupChannel->enableRead();
    } catch (SystemException& e) {
        LOG_ERR("{}", e.what());
        printBacktrace();
        tCurrentLoop = nullptr;
        tCurrentLoopTid = -1;
        throw;
    }

    // EventLoop not start now, set mIsExit true
    mIsExit = true;
    mIsLoopingNow = false;
    mIsDoPendingWorks = false;
    LOG_INFO("{}: X", __FUNCTION__);
}

EventLoop::~EventLoop() {
    assertInLoopThread();
    // Restore thread state.
    LOG_INFO("{}: E", __FUNCTION__);
    LOG_INFO("{}: current thread EventLoop: {}", __FUNCTION__, static_cast<void *>(this));
    mpWakeupChannel = nullptr;
    mpWakeupFd = nullptr;
    mpTimerQueue = nullptr;
    mpPoller = nullptr;
    tCurrentLoop = nullptr;
    tCurrentLoopTid = -1;
    LOG_INFO("{}: X", __FUNCTION__);
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
    // FIXME:
    // In the procedure of stack unwind, Channel may destroied when mIsLoopingNow is true.
    assertTrue(mIsLoopingNow == false || std::uncaught_exceptions() > 0
            , "Don't remove channel in loop! Should queueInLoop...");
    mpPoller->removeChannel(channel);
}

void EventLoop::updateChannel(Channel *channel) {
    LOG_INFO("{}: channel {}, fd {}"
            , __FUNCTION__, static_cast<void *>(channel), channel->getFd());
    assertInLoopThread();
    mpPoller->updateChannel(channel);
}

void EventLoop::startLoop() {
    LOG_INFO("{}: E", __FUNCTION__);
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
            LOG_DEBUG("{}: current channel({}), fd({}), info({})"
                    , __FUNCTION__, static_cast<void *>(mpCurrentChannel)
                    , mpCurrentChannel->getFd(), mpCurrentChannel->getInfo());
            channel->handleEvent();
            mpCurrentChannel = nullptr;
        }
        mIsLoopingNow = false;
        // Do pending tasks after loop.
        doPendingTasks();
    }
    LOG_INFO("{}: X", __FUNCTION__);
}

void EventLoop::quitLoop() {
    LOG_INFO("{}", __FUNCTION__);
    mIsExit = true;
    // Wakeup loop if not in loop thread.
    if (!isInLoopThread()) {
        wakeup();
    }
}

void EventLoop::queueInLoop(std::function<void ()>&& cb) {
    std::lock_guard lock { mMutex };
    mPendingTasks.emplace_back(std::move(cb));
    wakeup();
}

void EventLoop::runInLoop(std::function<void ()>&& cb) {
    TRACE();
    if (isInLoopThread()) {
        cb();
    } else {
        auto task = std::make_shared<std::packaged_task<void()>>([cb = std::move(cb)] {
            cb();
        });
        auto fu = task->get_future();
        queueInLoop([task] {
            (*task)();
        });
        fu.wait();
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

bool EventLoop::isInLoopThread() const noexcept {
    return (tCurrentLoop != nullptr);
}

int EventLoop::getLoopTid() const noexcept {
    return static_cast<int>(tCurrentLoopTid);
}

void EventLoop::assertInLoopThread() {
#ifdef DEBUG_BUILD
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
    mIsDoPendingWorks = true;
    std::vector<std::function<void ()>> pendingTasks;
    {
        std::lock_guard lock { mMutex };
        std::swap(pendingTasks, mPendingTasks);
    }
    LOG_DEBUG("{} :pendingTasks count: {}", __FUNCTION__, pendingTasks.size());
    for (auto&& task : pendingTasks) {
        task();
    }
    mIsDoPendingWorks = false;
    LOG_DEBUG("{} X", __FUNCTION__);
}

} // namespace net
