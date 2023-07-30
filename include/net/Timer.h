#pragma once
#include "base/Utils.h"
#include <memory>
#include <ratio>
#include <string_view>
#include <chrono>
#include <concepts>
#include <type_traits>

namespace net {

enum class TimerType {
    Relative,
    Absolute,
};

class Timer final {
public:
    using TimerPtr = std::unique_ptr<Timer>;
    static TimerPtr createTimer();

    DISABLE_COPY(Timer);
    DISABLE_MOVE(Timer);
    ~Timer();

    [[nodiscard]]
    int getFd() const noexcept { return mFd; }

    void setDelay(TimerType type, std::chrono::microseconds delay);

    void setRepeating(TimerType type, std::chrono::microseconds interval);

    void resetTimer();

    void cancelTimer();

    void handleRead();

    void handleError();

    [[nodiscard]]
    std::chrono::microseconds getTimer() const;

private:
    Timer(int fd) : mFd(fd), mIsRepeating(false) {};

    int mFd;
    bool mIsRepeating;
};

using TimerPtr = Timer::TimerPtr;

} // namespace utils
