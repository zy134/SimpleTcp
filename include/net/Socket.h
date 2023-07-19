#pragma once

#include "base/Utils.h"
#include "base/Expected.h"
#include <algorithm>
#include <cstdint>
#include <memory>
#include <netdb.h>
#include <sys/socket.h>

namespace net {

enum class IP_PROTOCOL {
    IPv4,
    IPv6,
};

struct SocketAddr {
    std::string mIpAddr;
    IP_PROTOCOL mIpProtocol;
    uint32_t    mPort;
};


/**
 * @brief : The wrapper of socket file descriptor.
 */
class Socket final {
public:
    using SocketPtr = std::unique_ptr<Socket>;
    using SocketResult = tl::expected<std::unique_ptr<Socket>, int>;
    DISABLE_COPY(Socket);
    DISABLE_MOVE(Socket);
    ~Socket();

    static SocketResult createTcpClientSocket(SocketAddr&& serverAddr);

    static SocketPtr createTcpListenSocket(SocketAddr&& serverAddr, int maxListenQueue);
    
    void listen();

    SocketResult accept();

    void shutdown();

    void setNonDelay(bool enable);
    
    void setNonBlock(bool enable);
    
    void setKeepAlive(bool enable);

    void setReuseAddr(bool enable);

    void setReusePort(bool enable);

    [[nodiscard]]
    int getFd() const noexcept { return mFd; }

    [[nodiscard]]
    IP_PROTOCOL getIpProtocol() const noexcept { return mLocalAddr.mIpProtocol; }

    [[nodiscard]]
    uint32_t getPort() const noexcept { return mLocalAddr.mPort; }

    [[nodiscard]]
    SocketAddr getLocalAddr() const noexcept { return mLocalAddr; }

    [[nodiscard]]
    SocketAddr getPeerAddr() const noexcept { return mPeerAddr; }
    
    [[nodiscard]]
    int getSocketError() const noexcept;
    
    void dumpSocketInfo() const noexcept;

private:
    Socket(int fd)
        : mFd(fd), mIsListenSocket(false), mMaxListenQueue(0) {}

    void setLocalAddr(IP_PROTOCOL ipType);

    void setPeerAddr(SocketAddr&& peerAddr) noexcept { mPeerAddr = std::move(peerAddr); }

    int         mFd;
    bool        mIsListenSocket;
    int         mMaxListenQueue;
    IP_PROTOCOL mIpProtocol;
    uint32_t    mPort;
    SocketAddr  mLocalAddr;
    SocketAddr  mPeerAddr;

};


using SocketPtr = Socket::SocketPtr;

using SocketResult = Socket::SocketResult;

inline auto operator<=>(const SocketPtr& lhs, const int fd) noexcept {
    return lhs->getFd()<=>fd;
};

inline auto operator<=>(const SocketPtr& lhs, const SocketPtr& rhs) noexcept {
    return lhs->getFd()<=>rhs->getFd();
};

} // namespace net
