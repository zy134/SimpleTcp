#pragma once

#include "base/Utils.h"
#include "net/Timer.h"
#include "net/Channel.h"

#include <chrono>
#include <memory>
#include <unordered_map>
#include <functional>
#include <mutex>

namespace simpletcp::net {

class EventLoop;

// Use timestamp of steady clock as the identification of Timer.
using TimerId = std::chrono::microseconds::rep;
inline constexpr TimerId INVALID_TIMER_ID = 0;

class TimerQueue {
private:
    struct TimerStruct {
        std::shared_ptr<Timer> timer;
        std::shared_ptr<Channel> channel;
    };

    using TimerChannelMap = std::unordered_map<TimerId, TimerStruct>;

public:
    DISABLE_COPY(TimerQueue);
    DISABLE_MOVE(TimerQueue);
    ~TimerQueue();
    using TimerCallback = std::function<void ()>;

    static std::unique_ptr<TimerQueue> createTimerQueue(EventLoop* loop) {
        return std::unique_ptr<TimerQueue>(new TimerQueue(loop));
    }

    TimerId addOneshotTimer(TimerCallback&&, TimerType, std::chrono::microseconds);

    TimerId addRepeatingTimer(TimerCallback&&, TimerType, std::chrono::microseconds);

    void removeTimer(TimerId timerId);

private:
    TimerQueue(EventLoop* loop);

    std::mutex          mMutex;
    net::EventLoop*     mpEventloop;
    TimerChannelMap     mTimerMap;
};

}
