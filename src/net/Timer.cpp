#include "base/Error.h"
#include "base/Log.h"
#include "net/Timer.h"
#include <chrono>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <memory>
#include <unistd.h>

static constexpr std::string_view TAG = "Timer";

extern "C" {
#include <sys/timerfd.h>
}

using namespace std;
using namespace std::chrono;
using namespace simpletcp;

namespace simpletcp::net {

TimerPtr Timer::createTimer() {
    auto timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    return TimerPtr(new Timer(timerfd));
}

Timer::~Timer() {
    LOG_DEBUG("{}: E", __FUNCTION__);
    ::close(getFd());
    LOG_DEBUG("{}: X", __FUNCTION__);
}

void Timer::setDelay(TimerType type, std::chrono::microseconds delay) {
    TRACE();
    itimerspec newTime {};

    auto secs = duration_cast<seconds>(delay);
    auto nsecs = duration_cast<nanoseconds>(delay - secs);
    newTime.it_value.tv_sec = secs.count();
    newTime.it_value.tv_nsec = nsecs.count();
    int flag = (type == TimerType::Absolute)?
                    TFD_TIMER_ABSTIME : 0;
    auto res = ::timerfd_settime(getFd(), flag, &newTime, nullptr);
    LOG_INFO("{}: set argument:{:6d}s.{:9d}ns", __FUNCTION__, newTime.it_value.tv_sec, newTime.it_value.tv_nsec);
    if (res != 0) {
        LOG_ERR("{}: {}, error argument:{:6d}s.{:9d}ns", __FUNCTION__, strerror(errno),
                newTime.it_value.tv_sec, newTime.it_value.tv_nsec);
    }
}

void Timer::setRepeating(TimerType type, std::chrono::microseconds interval) {
    TRACE();
    itimerspec newTime {};

    auto secs = duration_cast<seconds>(interval);
    auto nsecs = duration_cast<nanoseconds>(interval - secs);
    newTime.it_value.tv_sec = secs.count();
    newTime.it_value.tv_nsec = nsecs.count();
    newTime.it_interval.tv_sec = secs.count();
    newTime.it_interval.tv_nsec = nsecs.count();
    int flag = (type == TimerType::Absolute)?
                    TFD_TIMER_ABSTIME : 0;
    auto res = ::timerfd_settime(getFd(), flag, &newTime, nullptr);
    LOG_INFO("{}: set argument:{:6d}s.{:9d}ns", __FUNCTION__, newTime.it_interval.tv_sec, newTime.it_interval.tv_nsec);
    if (res != 0) {
        LOG_ERR("{}: {}, error argument:{:6d}s.{:9d}ns", __FUNCTION__, strerror(errno),
                newTime.it_interval.tv_sec, newTime.it_interval.tv_nsec);
    }
    mIsRepeating = true;
}

void Timer::resetTimer() {
    TRACE();
    itimerspec newTime {};
    itimerspec oldTime {};
    ::memset(&newTime, 0, sizeof(newTime));
    ::memset(&oldTime, 0, sizeof(oldTime));
    auto res = ::timerfd_settime(getFd(), 0, &newTime, &oldTime);
    if (res != 0) {
        LOG_ERR("{}: {}", __FUNCTION__, strerror(errno));
    }
}

void Timer::cancelTimer() {
    TRACE();
    auto res = ::timerfd_settime(getFd(), TFD_TIMER_CANCEL_ON_SET, nullptr, nullptr);
    if (res != 0) {
        LOG_ERR("{}: {}", __FUNCTION__, strerror(errno));
    }
}

microseconds Timer::getTimer() const {
    TRACE();
    itimerspec oldTime {};
    auto res = ::timerfd_gettime(getFd(), &oldTime);
    if (res != 0) {
        LOG_ERR("{}: {}", __FUNCTION__, strerror(errno));
        return microseconds(0);
    }

    seconds secs;
    nanoseconds usecs;
    if (!mIsRepeating) {
        secs = seconds { oldTime.it_value.tv_sec };
        usecs = nanoseconds { oldTime.it_value.tv_nsec };
    } else {
        secs = seconds { oldTime.it_interval.tv_sec };
        usecs = nanoseconds { oldTime.it_interval.tv_nsec };
    }
    return duration_cast<microseconds>(secs) + duration_cast<microseconds>(usecs);
}

void Timer::handleRead() {
    TRACE();
    uint64_t bytes;
    auto res = ::read(getFd(), &bytes, sizeof(bytes));
    LOG_DEBUG("{}: read from timer {}", __FUNCTION__, bytes);
    if (res < 0) {
        LOG_ERR("{}: {}", __FUNCTION__, strerror(errno));
    }
}

void Timer::handleError() {
    TRACE();
    LOG_ERR("{}: {}", __FUNCTION__, strerror(errno));
}

}
