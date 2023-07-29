#include "base/Error.h"
#include "base/Log.h"
#include "net/Timer.h"
#include "net/TimerQueue.h"
#include "net/Channel.h"
#include "net/EventLoop.h"
#include "net/FileDesc.h"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fmt/format.h>
#include <memory>
#include <mutex>

static constexpr std::string_view TAG = "TimerQueue";

using namespace utils;

namespace net {

TimerQueue::TimerQueue(EventLoop* loop) : mpEventloop(loop) {}

TimerQueue::~TimerQueue() {
    TRACE();
    // First, release all Channel
    for (auto&& iter : mTimerMap) {
        iter.second.channel = nullptr;
    }
    // and then release FileDesc
    mTimerMap.clear();
}

TimerId TimerQueue::addOneshotTimer(TimerCallback&& cb, TimerType type, std::chrono::microseconds delay) {
    try {
        std::shared_ptr<Timer> newTimer = net::Timer::createTimer();
        newTimer->setDelay(type, delay);
        TimerId newId = std::chrono::steady_clock::now().time_since_epoch().count();
        LOG_INFO("{}: create new timer :{}", __FUNCTION__, newId);
        // Pass the copy of newTimer to lambda.
        mpEventloop->queueInLoop([this, newTimer, newId, cb = std::move(cb)] () mutable {
            // Create new channel must run in loop thread.
            auto newChannel = net::Channel::createChannel(newTimer->getFd(), mpEventloop, ChannelPriority::High);
            newChannel->setReadCallback([this, newTimer, newId, cb = std::move(cb)] () {
                cb();
                newTimer->handleRead();
                // Task have done, and then remove the timer.
                removeTimer(newId);
            });
            newChannel->setErrorCallback([newTimer] () {
                newTimer->handleError();
            });
            newChannel->enableRead();
            TimerStruct newTimerStruct {
                .timer = std::move(newTimer),
                .channel = std::move(newChannel),
            };
            // Lock before modify map.
            std::lock_guard lock { mMutex };
            mTimerMap.emplace(newId, std::move(newTimerStruct));
        });
        return newId;
    } catch (NormalException& e) {
        LOG_ERR("{}: {}", __FUNCTION__, e.what());
        return INVALID_TIMER_ID;
    } catch (SystemException& e) {
        LOG_ERR("{}: {}", __FUNCTION__, e.what());
        return INVALID_TIMER_ID;
    } // ignore other exception
}

TimerId TimerQueue::addRepeatingTimer(TimerCallback&& cb, TimerType type, std::chrono::microseconds interval) {
    try {
        std::shared_ptr<Timer> newTimer = net::Timer::createTimer();
        newTimer->setRepeating(type, interval);
        TimerId newId = std::chrono::steady_clock::now().time_since_epoch().count();
        LOG_INFO("{}: create new timer :{}", __FUNCTION__, newId);
        // Pass the copy of newTimer to lambda.
        mpEventloop->queueInLoop([this, newTimer, newId, cb = std::move(cb)] () mutable {
            // Create new channel must run in loop thread.
            auto newChannel = net::Channel::createChannel(newTimer->getFd(), mpEventloop, ChannelPriority::High);
            newChannel->setReadCallback([newTimer, cb = std::move(cb)] () {
                newTimer->handleRead();
                cb();
                // repeating task, don't remove the timer.
            });
            newChannel->setErrorCallback([newTimer] () {
                newTimer->handleError();
            });
            newChannel->enableRead();
            TimerStruct newTimerStruct {
                .timer = std::move(newTimer),
                .channel = std::move(newChannel),
            };
            // Lock before modify map.
            std::lock_guard lock { mMutex };
            mTimerMap.emplace(newId, std::move(newTimerStruct));
        });
        return newId;
    } catch (NormalException& e) {
        LOG_ERR("{}: {}", __FUNCTION__, e.what());
        return INVALID_TIMER_ID;
    } catch (SystemException& e) {
        LOG_ERR("{}: {}", __FUNCTION__, e.what());
        return INVALID_TIMER_ID;
    } // ignore other exception
}

void TimerQueue::removeTimer(TimerId timerId) {
    LOG_INFO("{}: remove timer :{}", __FUNCTION__, timerId);
    mpEventloop->queueInLoop([this, timerId] {
        std::lock_guard lock { mMutex };
        auto timerIter = std::find_if(mTimerMap.begin(), mTimerMap.end(), [timerId] (const auto& iter) {
            return iter.first == timerId;
        });
        if (timerIter == mTimerMap.end()) {
            LOG_ERR("{}: timer has not been register!", __FUNCTION__);
            return ;
        }
        // First, release Channel
        timerIter->second.channel = nullptr;
        // and then release FileDesc
        timerIter->second.timer = nullptr;
        mTimerMap.erase(timerIter->first);
    });
}

}
