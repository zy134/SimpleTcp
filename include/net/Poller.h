#pragma once

#include "base/Error.h"
#include "base/Utils.h"

/**
 * Poller class, it'a wrapper of linux epoll
 *
 * The instance of Poller hold by EventLoop, and it's has a weak reference of parent EventLoop.
 * Poller not hold strong reference of Channel and EventLoop, it's just has the weak reference
 * of them.
 *
 * Initialize sequence: EventLoop >= Poller > TcpServer > TcpConnection >= Socket > Channel
 *
 */
namespace net {

class EventLoop;
class Channel;

class Poller {
public:
    Poller(EventLoop* loop);
    ~Poller();

    // Not thread-safe but in loop thread.
    void addChannel(Channel*);
    // Not thread-safe but in loop thread.
    void removeChannel(Channel *);
    // Not thread-safe but in loop thread.
    void updateChannel(Channel *);

private:

    
};

} // namespace utils
