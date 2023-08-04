#pragma once

#include "base/Utils.h"
#include <algorithm>
#include <functional>
#include <memory>

namespace simpletcp::net {

using ChannelCbType = std::function<void ()>;

// The lifetime of EventLoop must be longer than Channel.
// But EventLoop not handle the instance of Channel, it just has the weak reference of Channel.
// So Channel must add/remove to EventLoop itself when it has been create/delete
class EventLoop;

// Indicate the priority of event.
enum ChannelPriority {
    Normal,
    High
};

// The Channl not hold the file descriptor, it just record it. So you need manage the lifetime
// of file descriptor yourself!
class Channel final {
public:
    DISABLE_COPY(Channel);
    DISABLE_MOVE(Channel);

    static auto createChannel(const int fd, EventLoop* loop, ChannelPriority priority = ChannelPriority::Normal) {
        return std::unique_ptr<Channel>(new Channel(fd, loop, priority));
    }

    ~Channel();

    void handleEvent();

    void setChannelInfo(std::string s) { mChannelInfo = std::move(s); }

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

    [[nodiscard]]
    auto getPriority() const noexcept { return mPriority; }

    [[nodiscard]]
    auto getInfo() const noexcept { return mChannelInfo; }
private:
    Channel(int fd, EventLoop* loop, ChannelPriority priority);

    ChannelCbType   mReadCb;
    ChannelCbType   mWriteCb;
    ChannelCbType   mErrorCb;
    ChannelCbType   mCloseCb;

    EventLoop*      mpEventLoop;

    uint32_t        mEvent;
    uint32_t        mRevent;
    int             mFd;
    ChannelPriority mPriority;
    std::string     mChannelInfo;
};

using ChannelPtr = std::unique_ptr<Channel>;

} // namespace net
