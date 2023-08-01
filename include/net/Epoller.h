#pragma once

#include "base/Utils.h"
#include "base/Error.h"
#include "net/Channel.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace simpletcp::net {

class Channel;

// The RAII wrappper of linux epoll.
// It's not thread-safe, thread-safety is guaranteed by the instance of EventLoop.
class Epoller final {
public:
    using EpollerPtr = std::unique_ptr<Epoller>;
    static EpollerPtr createEpoller();
    ~Epoller();

    [[nodiscard]]
    int getFd() const noexcept { return mFd; }

    [[nodiscard]]
    bool hasChannel(Channel *) const noexcept;
    void addChannel(Channel *);
    void updateChannel(Channel *);
    void removeChannel(Channel *);

    auto poll() -> std::vector<Channel *>;

private:
    Epoller(int fd) noexcept : mFd(fd) {}
    int mFd;

    std::unordered_map<int, Channel *> mChannelMap;
    std::unordered_set<Channel *> mChannelSet;
};

using EpollerPtr = Epoller::EpollerPtr;

} // namespace utils
