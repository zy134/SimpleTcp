#pragma once

#include "base/Utils.h"
#include "net/EventLoop.h"
#include <condition_variable>
#include <thread>
#include <unordered_map>
#include <functional>

namespace simpletcp::net {

class EventLoopPool {
public:
    DISABLE_COPY(EventLoopPool);
    DISABLE_MOVE(EventLoopPool);

    EventLoopPool(EventLoop* loop, int maxThreadNum);
    ~EventLoopPool();

    [[nodiscard]]
    EventLoop* acquireLoop();

    [[nodiscard]]
    int getLoopNums() const noexcept { return mMaxThreadNum; }
    // void releaseLoop(EventLoop* loop);

private:
    enum class State {
        Initialized,
        Running,
        Exit,
    };

    EventLoop*                  mpMainLoop;
    int                         mMaxThreadNum;

    std::vector<std::thread>    mSubThreads;
    std::vector<EventLoop *>    mSubLoops;
    std::unordered_map<EventLoop *, State>
                                mSubLoopStates;

    size_t                      mIndex;
    std::mutex                  mMutex;
    std::condition_variable     mCond;
};

} // namespace simpletcp::net
