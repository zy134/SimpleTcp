#pragma once
#include "base/Utils.h"
#include "net/TimerQueue.h"

#include <chrono>
#include <functional>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace simpletcp::net {

class Channel;
class Epoller;
class EventFd;
class TimerQueue;

class EventLoop final {
public:
    DISABLE_COPY(EventLoop);
    DISABLE_MOVE(EventLoop);

    /**
     * @brief EventLoop : Create a new EventLoop and initialize poller. Every thread can hold only
     *                    one instance of EventLoop!
     */
    EventLoop();

    ~EventLoop();

    /**
     * @brief addChannel : Internal method, add new channel to EventLoop, only invoked by Channel!
     *
     * @param channel: 
     */
    void addChannel(Channel* channel);

    /**
     * @brief addChannel : Internal method, remove channel from EventLoop, only invoked by Channel!
     *
     * @param channel: 
     */
    void removeChannel(Channel* channel);

    /**
     * @brief addChannel : Internal method, update channel from EventLoop, only invoked by Channel!
     *
     * @param channel: 
     */
    void updateChannel(Channel* channel);

    /**
     * @brief startLoop : User interface, start loop and wait for enable channels.
     */
    void startLoop();

    /**
     * @brief quitLoop : User interface, quit loop.
     */
    void quitLoop();

    /**
     * @brief queueInLoop : User interface, enqueue a new callback functor in loop thread
     *                      , and then wakeup poller to invoke it.
     *
     * @param cb: 
     */
    void queueInLoop(std::function<void()>&& cb);

    /**
     * @brief runInLoop : User interface, invoke callback in specific loop thread, may block current thread.
     *                    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
     *                    The main difference of queueInLoop and runInLoop is runInLoop would wait
     *                    for the callback function to complete.
     *                    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
     *
     * @param cb: 
     */
    void runInLoop(std::function<void()>&& cb);

    /**
     * @brief runAfter : Start the task with a delay of duration ms.
     *
     * @param cb: 
     * @param delay: The delay before task start. We use microseconds as the metric because
     *                  the EventFd does not support latency with nanosecond accuracy
     *
     * @return : The identification of Timer.
     */
    TimerId runAfter(std::function<void()>&& cb, std::chrono::microseconds delay);

    /**
     * @brief runContinuous : Invoke the callback function every duration ms.
     *
     * @param cb: 
     * @param interval: The delay before task start. We use microseconds as the metric because
     *                  the EventFd does not support latency with nanosecond accuracy
     *
     * @return : The identification of Timer.
     */
    TimerId runEvery(std::function<void()>&& cb, std::chrono::microseconds interval);

    void removeTimer(TimerId timerId);

    /**
     * @brief assertInLoopThread : In current thread is not same as the thread of loop, abort process.
     */
    void assertInLoopThread();

    [[nodiscard]]
    bool isInLoopThread() const noexcept;

    [[nodiscard]]
    int getLoopTid() const noexcept;

private:
    // atomic variant
    bool                                            mIsExit;
    bool                                            mIsLoopingNow;
    bool                                            mIsDoPendingWorks;
    std::mutex                                      mMutex;
    Channel*                                        mpCurrentChannel;
    // EventLoop only manager three type of file descriptors.
    // epoll fd, event fd, timer fd.
    std::unique_ptr<Epoller>                        mpPoller;
    // Wakeup channel and timer channel is the special case that
    // EventLoop would hold instances of these Channels.
    std::unique_ptr<EventFd>                        mpWakeupFd;
    std::unique_ptr<Channel>                        mpWakeupChannel;
    std::unique_ptr<TimerQueue>                     mpTimerQueue;
    std::vector<std::function<void ()>>             mPendingTasks;

private:

    void wakeup();

    void doPendingTasks();
};

}; // namespace net
