#include "net/Channel.h"
#include "net/EventLoop.h"
#include "base/Log.h"
#include "base/Error.h"

#include <array>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

extern "C" {
#include <unistd.h>
#include <fcntl.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
}

#ifdef TAG
#undef TAG
#endif
static constexpr std::string_view TAG = "Channel";

using namespace simpletcp;
using namespace std;

namespace simpletcp::net {

/************************************************************/
/*********************** Channel ****************************/
/************************************************************/

const auto CHANNEL_READ_EVENT = EPOLLIN | EPOLLPRI;
const auto CHANNEL_WRITE_EVENT = EPOLLOUT;
const auto CHANNEL_NONE_EVENT = 0;

Channel::Channel(int fd, EventLoop* loop, ChannelPriority priority)
    :mpEventLoop(loop), mFd(fd), mPriority(priority)
{
    LOG_INFO("{}", __FUNCTION__);
    mpEventLoop->assertInLoopThread();
    assertTrue(fd >= 0, "[Channl] File descriptor must be valid!");

    [[unlikely]]
    if (auto res = fcntl(mFd, F_SETFL, O_NONBLOCK); res != 0) {
        throw SystemException("[Channl] File descriptor must be non-blocking!");
    }

    mEvent = EPOLLERR;

    // set default callback
    mReadCb = [] {};
    mWriteCb = [] {};
    mCloseCb = [] { LOG_INFO("close callback."); };
    mErrorCb = [] { LOG_INFO("error callback"); };

    mpEventLoop->addChannel(this);
}

Channel::~Channel() {
    LOG_INFO("{} +", __FUNCTION__);
    mpEventLoop->assertInLoopThread();
    mpEventLoop->removeChannel(this);
    LOG_INFO("{} -", __FUNCTION__);
}

// Just call by EventLoop::loop()
// Must run in loop.
void Channel::handleEvent() {
    LOG_INFO("{} +", __FUNCTION__);
    if ((mRevent & EPOLLHUP) && !(mRevent & EPOLLIN)) {
        mCloseCb();
        return ;
    }
    if (mRevent & EPOLLERR) {
        mErrorCb();
        return ;
    }
    if (mRevent & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
        mReadCb();
    }
    if (mRevent & EPOLLOUT) {
        mWriteCb();
    }
    LOG_INFO("{} -", __FUNCTION__);
}

void Channel::enableRead() {
    LOG_INFO("{}", __FUNCTION__);
    if (mEvent & CHANNEL_READ_EVENT) { return; }
    mEvent |= CHANNEL_READ_EVENT;
    mpEventLoop->updateChannel(this);
}

void Channel::enableWrite() {
    LOG_INFO("{}", __FUNCTION__);
    if (mEvent & CHANNEL_WRITE_EVENT) { return; }
    mEvent |= CHANNEL_WRITE_EVENT;
    mpEventLoop->updateChannel(this);
}

void Channel::disableRead() {
    LOG_INFO("{}", __FUNCTION__);
    if (!(mEvent & CHANNEL_READ_EVENT)) { return; }
    mEvent &= (~CHANNEL_READ_EVENT);
    mpEventLoop->updateChannel(this);
}
void Channel::disableWrite() {
    LOG_INFO("{}", __FUNCTION__);
    if (!(mEvent & CHANNEL_WRITE_EVENT)) { return; }
    mEvent &= (~CHANNEL_WRITE_EVENT);
    mpEventLoop->updateChannel(this);
}

void Channel::disableAll() {
    LOG_INFO("{}", __FUNCTION__);
    mEvent = CHANNEL_NONE_EVENT;
    mpEventLoop->updateChannel(this);
}

bool Channel::isWriting() const noexcept { return mEvent & CHANNEL_WRITE_EVENT; }

bool Channel::isReading() const noexcept { return mEvent & CHANNEL_READ_EVENT; }

} // namespace net

