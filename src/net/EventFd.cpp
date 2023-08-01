#include "base/Error.h"
#include "net/EventFd.h"
#include <cstddef>
#include <cstdint>
#include <memory>

extern "C" {
#include <unistd.h>
#include <sys/eventfd.h>
}

using namespace simpletcp;

namespace simpletcp::net {

EventFdPtr EventFd::createEventFd() {
    auto eventFd = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (eventFd < 0) {
        throw SystemException("[EventFd] createEventFd failed.");
    }
    return std::unique_ptr<EventFd>(new EventFd(eventFd));
}

EventFd::~EventFd() {
    ::close(getFd());
}

void EventFd::handleRead() {
    uint64_t bytes;
    auto res = ::read(getFd(), &bytes, sizeof(bytes));
    if (res < 0) {
        throw SystemException("[EventFd] handleRead failed.");
    }
}

void EventFd::wakeup() {
    uint64_t bytes = 1;
    auto res = ::write(getFd(), &bytes, sizeof(bytes));
    if (res < 0) {
        throw SystemException("[EventFd] handleRead failed.");
    }
}

} // namespace utils
