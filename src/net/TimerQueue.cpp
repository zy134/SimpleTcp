#include "base/Error.h"
#include "base/Log.h"
#include "net/TimerQueue.h"
#include "net/Channel.h"
#include "net/EventLoop.h"
#include "net/FileDesc.h"
#include <algorithm>
#include <cstdlib>

using namespace utils;

namespace net {

TimerQueue::TimerQueue(EventLoop* loop) : mpEventloop(loop) {}

TimerQueue::~TimerQueue() {
    // First, release all Channel
    for (auto&& iter : mTimerMap) {
        iter.second = nullptr;
        iter.first->cancelTimer();
    }
    // and then release FileDesc
    mTimerMap.clear();
}

void TimerQueue::addTimer(TimerCallback&& cb, TimerType type, uint32_t microseconds) {
    try {
        auto newTimerFd = net::Timer::createTimer();
        newTimerFd->setTimer(type, microseconds);
        auto newTimerChannel = net::Channel::createChannel(newTimerFd->getFd(), mpEventloop);
        newTimerChannel->setReadCallback([&, cb = std::move(cb)] () {
            cb();
            newTimerFd->handleRead();
        });
        newTimerChannel->enableRead();
        mTimerMap.insert( { std::move(newTimerFd), std::move(newTimerChannel) } );
    } catch (NormalException& e) {

    } catch (SystemException& e) {

    } // ignore other exception
}

void TimerQueue::removeTimer(int timerFd) {
    auto iter = std::find_if(mTimerMap.begin(), mTimerMap.end(), [timerFd] (const auto& iter) {
        return iter.first->getFd() == timerFd;
    });
    assertTrue(iter != mTimerMap.end(), "[TimerQueue] timer has not been register!");
    // First, release Channel
    iter->second = nullptr;
    // and then release FileDesc
    mTimerMap.erase(iter->first);
}

}
