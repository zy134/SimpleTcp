#include "base/Error.h"
#include "net/Timer.h"
#include <cstdint>
#include <cstring>
#include <memory>
#include <sys/select.h>
#include <unistd.h>

extern "C" {
#include <sys/timerfd.h>
}

using namespace utils;

namespace net {

TimerPtr Timer::createTimer() {
    auto timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    return TimerPtr(new Timer(timerfd));
}

constexpr auto MICROSECONDS_PER_SECOND = 1000 * 1000;
// constexpr auto NANOSECONDS_PER_SECOND = 1000 * 1000 * 1000;
constexpr auto NANOSECONDS_PER_USEC = 1000;

Timer::~Timer() {
    ::close(getFd());
}

void Timer::setTimer(TimerType type, uint32_t microSeconds) {
    itimerspec newTime {};
    newTime.it_value.tv_nsec = microSeconds * NANOSECONDS_PER_USEC;
    int flag = (type == TimerType::Absolute)?
                    TFD_TIMER_ABSTIME : 0;
    auto res = ::timerfd_settime(getFd(), flag, &newTime, nullptr);
    if (res != 0) {
        throw SystemException("[Timer] setTimer failed");
    }
}

void Timer::resetTimer() {
    itimerspec newTime {};
    itimerspec oldTime {};
    ::memset(&newTime, 0, sizeof(newTime));
    ::memset(&oldTime, 0, sizeof(oldTime));
    auto res = ::timerfd_settime(getFd(), 0, &newTime, &oldTime);
    if (res != 0) {
        throw SystemException("[Timer] setTimer failed");
    }
}

void Timer::cancelTimer() {
    auto res = ::timerfd_settime(getFd(), TFD_TIMER_CANCEL_ON_SET, nullptr, nullptr);
    if (res != 0) {
        throw SystemException("[Timer] cancelTimer failed");
    }
}

uint32_t Timer::getTimer() const {
    itimerspec oldTime {};
    auto res = ::timerfd_gettime(getFd(), &oldTime);
    if (res != 0) {
        throw SystemException("[Timer] getTimer failed");
    }
    return oldTime.it_value.tv_sec * MICROSECONDS_PER_SECOND + (oldTime.it_value.tv_nsec / NANOSECONDS_PER_USEC);
}

void Timer::handleRead() {
    uint64_t bytes;
    auto res = ::read(getFd(), &bytes, sizeof(bytes));
    if (res != 0) {
        throw SystemException("[Timer] handleRead failed");
    }
}

}
