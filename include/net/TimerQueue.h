#pragma once

#include "base/Utils.h"
#include "net/Timer.h"
#include "net/Channel.h"

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <functional>

namespace net {

class EventLoop;

// TODO: Not usable now!
class TimerQueue {
public:
    //DISABLE_COPY(TimerQueue);
    //DISABLE_MOVE(TimerQueue);
    ~TimerQueue();
    using TimerChannelMap = std::unordered_map<std::shared_ptr<Timer>, std::shared_ptr<Channel>>;
    using TimerCallback = std::function<void ()>;

    static std::unique_ptr<TimerQueue> createTimerQueue(EventLoop* loop);

    // Not thread safety.
    void addTimer(TimerCallback&&, TimerType, uint32_t);

private:
    TimerQueue(EventLoop* loop);
    void removeTimer(int fd);

    net::EventLoop*     mpEventloop;
    TimerChannelMap     mTimerMap;
};

}
