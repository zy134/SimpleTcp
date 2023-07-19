#pragma once
#include "base/Utils.h"
#include <cstdint>
#include <memory>
#include <string_view>

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

    void setTimer(TimerType, uint32_t);

    void resetTimer();

    void cancelTimer();

    void handleRead();

    [[nodiscard]]
    uint32_t getTimer() const;

private:
    Timer(int fd) : mFd(fd) {};

    int mFd;
};

using TimerPtr = Timer::TimerPtr;

inline auto operator<=>(const TimerPtr& lhs, const int fd) noexcept {
    return lhs->getFd()<=>fd;
};

inline auto operator<=>(const TimerPtr& lhs, const TimerPtr& rhs) noexcept {
    return lhs->getFd()<=>rhs->getFd();
};


} // namespace utils
