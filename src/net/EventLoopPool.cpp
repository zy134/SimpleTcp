#include "net/EventLoopPool.h"
#include "base/Log.h"
#include "net/EventLoop.h"
#include <algorithm>
#include <mutex>
#include <string_view>
#include <thread>
#include <chrono>

using namespace simpletcp;
using namespace simpletcp::net;
using namespace std;
using namespace std::chrono_literals;

static constexpr std::string_view TAG = "EventLoopPool";

namespace simpletcp::net {

EventLoopPool::EventLoopPool(EventLoop* mainLoop, int maxThreadNum): mpMainLoop(mainLoop), mMaxThreadNum(maxThreadNum) {
    LOG_INFO("{}: E", __FUNCTION__);
    assertTrue(mMaxThreadNum >= 0, "[EventLoopPool] bad maxThreadNum");
    assertTrue(mpMainLoop != nullptr, "[EventLoopPool] loop must not be none!");

    if (mMaxThreadNum > (int)std::thread::hardware_concurrency()) {
        LOG_WARN("{}: maxThreadNum {} is bigger than hardware_concurrency num {}!"
                , __FUNCTION__, mMaxThreadNum, std::thread::hardware_concurrency());
        mMaxThreadNum = (int)std::thread::hardware_concurrency();
    }

    for (int i = 0; i < mMaxThreadNum; ++i) {
        mSubThreads.emplace_back([this] {

            EventLoop loop {};
            LOG_INFO("EventLoopPool: create new loop {} in thread {}"
                    , static_cast<void *>(&loop), loop.getLoopTid());
            {
                std::unique_lock lock { mMutex };
                mSubLoops.push_back(&loop);
                mSubLoopStates.insert({ &loop, State::Initialized });
                while (mSubLoopStates[&loop] != State::Running) {
                    mCond.wait(lock);
                }
            }
            LOG_INFO("EventLoopPool: start loop {}", static_cast<void *>(&loop));
            loop.startLoop();
            std::lock_guard lock { mMutex };
            if (mSubLoopStates[&loop] == State::Exit) {
                LOG_INFO("EventLoopPool: quit loop {}", static_cast<void *>(&loop));
                return ;
            }
        });
    }
    mIndex = 0;
    LOG_INFO("{}: X", __FUNCTION__);
}

EventLoopPool::~EventLoopPool() {
    LOG_INFO("{}: E", __FUNCTION__);
    std::lock_guard lock { mMutex };
    for (auto loop : mSubLoops) {
        mSubLoopStates[loop] = State::Exit;
        loop->quitLoop();
    }
    mCond.notify_all();
    for (auto&& thread : mSubThreads) {
        if (thread.joinable())
            thread.join();
    }
    LOG_INFO("{}: X", __FUNCTION__);
}

EventLoop* EventLoopPool::acquireLoop() {
    if (mMaxThreadNum == 0) {
        LOG_INFO("{}: return main loop", __FUNCTION__);
        return mpMainLoop;
    }

    std::lock_guard lock { mMutex };
    auto loop = mSubLoops[ mIndex % mSubLoops.size() ];
    LOG_INFO("{}: return sub loop {} in index {}", __FUNCTION__, static_cast<void *>(loop), mIndex % mSubLoops.size());
    ++mIndex;
    // First time enter loop.
    if (mSubLoopStates[loop] == State::Initialized) {
        mSubLoopStates[loop] = State::Running;
        mCond.notify_all();
    }
    return loop;
}

} // namespace simpletcp::net
