#pragma once

#include "base/Utils.h"
#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>

namespace net {

using ChannelCbType = std::function<void ()>;

// The lifetime of EventLoop must be longer than Channel.
// But EventLoop not handle the instance of Channel, it just has the weak reference of Channel.
// So Channel must add/remove to EventLoop itself when it has been create/delete
class EventLoop;

// The Channl not hold the file descriptor, it just record it. So you need manage the lifetime
// of file descriptor yourself!
class Channel {
public:
    DISABLE_COPY(Channel);
    DISABLE_MOVE(Channel);

    static auto createChannel(const int fd, EventLoop* loop) {
        return std::unique_ptr<Channel>(new Channel(fd, loop));
    }

    ~Channel();

    void handleEvent();

    void setWriteCallback(ChannelCbType&& cb) noexcept { mWriteCb = std::move(cb); }

    void setReadCallback(ChannelCbType&& cb) noexcept { mReadCb = std::move(cb); }

    void setErrorCallback(ChannelCbType&& cb) noexcept { mErrorCb = std::move(cb); }

    void setCloseCallback(ChannelCbType&& cb) noexcept { mCloseCb = std::move(cb); }

    // Not thread safe, but in loop thread.
    void enableRead();
    void enableWrite();
    void disableRead();
    void disableWrite();
    void disableAll();

    [[nodiscard]]
    bool isWriting() const noexcept;
    [[nodiscard]]
    bool isReading() const noexcept;

    [[nodiscard]]
    int getFd() const noexcept { return mFd; }

    [[nodiscard]]
    uint32_t getEvent() const noexcept { return mEvent; }

    void setRevents(uint32_t revent) { mRevent = revent; }

private:
    Channel(int fd, EventLoop* loop);

    ChannelCbType   mReadCb;
    ChannelCbType   mWriteCb;
    ChannelCbType   mErrorCb;
    ChannelCbType   mCloseCb;

    EventLoop*      mpEventLoop;

    uint32_t        mEvent;
    uint32_t        mRevent;
    int             mFd;

};

using ChannelPtr = std::unique_ptr<Channel>;

} // namespace net
