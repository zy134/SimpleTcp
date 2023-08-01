#pragma once
#include "base/Utils.h"
#include <memory>
#include <string_view>

namespace simpletcp::net {

class EventFd final {
public:
    using EventFdPtr = std::unique_ptr<EventFd>;
    static EventFdPtr createEventFd();

    DISABLE_COPY(EventFd);
    DISABLE_MOVE(EventFd);
    ~EventFd();

    [[nodiscard]]
    int getFd() const noexcept { return mFd; }

    void wakeup();

    void handleRead();

private:
    EventFd(int fd) : mFd(fd) {}

    int mFd;
};

using EventFdPtr = EventFd::EventFdPtr;

inline auto operator<=>(const EventFdPtr& lhs, const int fd) noexcept {
    return lhs->getFd()<=>fd;
};

inline auto operator<=>(const EventFdPtr& lhs, const EventFdPtr& rhs) noexcept {
    return lhs->getFd()<=>rhs->getFd();
};

} // namespace net
