#include "base/Log.h"
#include "base/Error.h"
#include "base/Backtrace.h"
#include "net/Channel.h"
#include "net/Epoller.h"
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <netdb.h>
#include <vector>

extern "C" {
#include <signal.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <unistd.h>
}

static constexpr std::string_view TAG = "Epoller";

// static check
static_assert(EPOLLIN == POLLIN,        "epoll uses same flag values as poll");
static_assert(EPOLLPRI == POLLPRI,      "epoll uses same flag values as poll");
static_assert(EPOLLOUT == POLLOUT,      "epoll uses same flag values as poll");
static_assert(EPOLLRDHUP == POLLRDHUP,  "epoll uses same flag values as poll");
static_assert(EPOLLERR == POLLERR,      "epoll uses same flag values as poll");
static_assert(EPOLLHUP == POLLHUP,      "epoll uses same flag values as poll");

using namespace simpletcp;

namespace simpletcp::net {

constexpr auto EPOLL_MAX_WAIT_NUM = 256;
constexpr auto EPOLL_MAX_WAIT_TIMEOUT = 1000;

std::string static transEventToStr(uint32_t event) {
    std::string result;
    if (event & EPOLLIN) { result.append("IN "); }
    if (event & EPOLLOUT) { result.append("OUT "); }
    if (event & EPOLLERR) { result.append("ERR "); }
    if (event & EPOLLHUP) { result.append("HUP "); }
    if (event & EPOLLRDHUP) { result.append("RDHUP "); }
    if (event & EPOLLPRI) { result.append("PRI "); }
    return result;
}

// Ignore sig pipe.
[[gnu::constructor]]
static void ignoreSigpipe() {
    ::signal(SIGPIPE, SIG_IGN);
}

Epoller::~Epoller() {
    LOG_INFO("{}", __FUNCTION__);
    ::close(getFd());
}

EpollerPtr Epoller::createEpoller() {
    LOG_INFO("{}", __FUNCTION__);
    auto epollFd = ::epoll_create(EPOLL_MAX_WAIT_NUM);
    if (epollFd < 0) {
        throw SystemException("createEpoller failed.");
    }
    return EpollerPtr(new Epoller(epollFd));
}

bool Epoller::hasChannel(Channel * channel) const noexcept {
    return mChannelSet.count(channel) != 0;
}

void Epoller::addChannel(Channel *channel) {
    assertTrue(!hasChannel(channel), "addChannel: channel has existed.");
    LOG_INFO("{}: fd {}, event {}", __FUNCTION__, channel->getFd(), transEventToStr(channel->getEvent()));
    epoll_event event {};
    event.data.fd = channel->getFd();
    auto res = ::epoll_ctl(getFd(), EPOLL_CTL_ADD, channel->getFd(), &event);
    if (res != 0) {
        throw SystemException("addChannel failed.");
    }
    mChannelMap.insert({ channel->getFd(), channel });
    mChannelSet.insert(channel);
}

void Epoller::updateChannel(Channel *channel) {
    assertTrue(hasChannel(channel), "updateChannel: channel has not existed.");
    LOG_INFO("{}: fd {}, event {}", __FUNCTION__, channel->getFd(), transEventToStr(channel->getEvent()));
    epoll_event event {};
    event.data.fd = channel->getFd();
    event.events = channel->getEvent();
    auto res = ::epoll_ctl(getFd(), EPOLL_CTL_MOD, channel->getFd(), &event);
    if (res != 0) {
        throw SystemException("updateChannel failed.");
    }
}

void Epoller::removeChannel(Channel *channel) {
    assertTrue(hasChannel(channel), "removeChannel: channel has not exist.");
    LOG_INFO("{}: fd {}, event {}", __FUNCTION__, channel->getFd(), transEventToStr(channel->getEvent()));
    epoll_event event {};
    event.data.fd = channel->getFd();
    auto res = ::epoll_ctl(getFd(), EPOLL_CTL_DEL, channel->getFd(), &event);
    if (res != 0) {
        throw SystemException("removeChannel failed.");
    }
    mChannelMap.erase(channel->getFd());
    mChannelSet.erase(channel);
}

auto Epoller::poll() -> std::vector<Channel *> {
    TRACE();
    std::vector<Channel *> result;

    std::array<epoll_event, EPOLL_MAX_WAIT_NUM> rEventArray;
    auto count = ::epoll_wait(getFd(), rEventArray.data(), rEventArray.size(), EPOLL_MAX_WAIT_TIMEOUT);
    if (count < 0) {
        throw SystemException("poll failed.");
    }

    for (size_t i = 0; i != static_cast<size_t>(count); ++i) {
        assertTrue(mChannelMap.count(rEventArray[i].data.fd) != 0, "poll : fd not existed.");
        auto* channel = mChannelMap[rEventArray[i].data.fd];
        channel->setRevents(rEventArray[i].events);
        LOG_INFO("{}: fd {}, result event {}"
                , __FUNCTION__, channel->getFd(), static_cast<uint32_t>(rEventArray[i].events));
        result.push_back(channel);
    }
    if (result.size() > 1) {
        std::sort(result.begin(), result.end(), [] (auto&& left, auto&& right) {
            // Sort active channels by priority.
            return (left->getPriority() > right->getPriority());
        });
    }
    return result;
}

} // namespace utils
