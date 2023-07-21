#include "base/Error.h"
#include "base/Expected.h"
#include "base/Utils.h"
#include "base/Log.h"
#include "net/Socket.h"
#include <algorithm>
#include <asm-generic/socket.h>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <variant>

extern "C" {
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
}

using namespace utils;

static constexpr std::string_view TAG = "Socket";

namespace net {

Socket::~Socket() {
    LOG_INFO("{} ", __FUNCTION__);
    ::close(getFd());
}

inline static int createNonblockingSocket(IP_PROTOCOL protocol) {
    int socketFd = ::socket(
            protocol == IP_PROTOCOL::IPv4 ? AF_INET : AF_INET6
            , SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK
            , IPPROTO_TCP);
    if (socketFd < 0) {
        throw SystemException("failed to create socket.");
    }
    LOG_INFO("{}: success create new fd {}", __FUNCTION__, socketFd);
    return socketFd;
}

inline static std::string transAddrToString(const sockaddr_in& addr) {
    std::string result;
    // addr.sin_addr is a 32 bytes struct.
    auto start = (uint8_t *)(&addr.sin_addr.s_addr);
    constexpr auto maxLen = sizeof(addr.sin_addr);
    for (int i = 0; i != maxLen; ++i) {
        result.append(std::to_string(start[i]));
        if (i != maxLen - 1) {
            result.push_back('.');
        }
    }
    return result;
}

inline static std::string transAddrToString(const sockaddr_in6& addr) {
    std::string result;
    auto start = addr.sin6_addr.s6_addr;
    constexpr auto maxLen = sizeof(addr.sin6_addr);
    for (int i = 0; i != maxLen; ++i) {
        result.push_back(start[i]);
        if (((i + 1) % 4) == 0) {
            result.append("::");
        }
    }
    return result;
}

SocketResult Socket::createTcpClientSocket(SocketAddr&& serverSocketAddr) {
    std::unique_ptr<Socket> result;

    constexpr auto failedResult = [] (int err) {
        return err != 0 &&
            err != EINPROGRESS && err != EINTR && err != EISCONN;
    };

    if (serverSocketAddr.mIpProtocol == IP_PROTOCOL::IPv4) {
        LOG_DEBUG("{}: IPv4", __FUNCTION__);
        sockaddr_in serverAddr = {};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(serverSocketAddr.mPort);
        // inet_pton return 1 indicate success.
        if (auto res = ::inet_pton(AF_INET, serverSocketAddr.mIpAddr.data(), &serverAddr.sin_addr); res != 1) {
            LOG_ERR("{}: error IP addr {}, res {}", __FUNCTION__, serverSocketAddr.mIpAddr.data(), res);
            return tl::unexpected(errno);
        }
        auto socketFd = createNonblockingSocket(IP_PROTOCOL::IPv4);
        if (auto res = ::connect(socketFd, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr))
                ; res != 0 && failedResult(errno)) {
            LOG_ERR("{}: connect failed, addr:{} port:{}", __FUNCTION__
                    , serverSocketAddr.mIpAddr.data(), serverSocketAddr.mPort);
            return tl::unexpected(errno);
        } else if (res != 0) {
            LOG_INFO("{}: connect not done for {}", __FUNCTION__, serverSocketAddr.mIpAddr.data());
        }
        result.reset(new Socket(socketFd));
    } else {
        LOG_DEBUG("{}: IPv6", __FUNCTION__);
        sockaddr_in6 serverAddr = {};
        serverAddr.sin6_family = AF_INET6;
        serverAddr.sin6_port = htons(serverSocketAddr.mPort);
        if (auto res = ::inet_pton(AF_INET6, serverSocketAddr.mIpAddr.data(), &serverAddr.sin6_addr); res != 1) {
            LOG_ERR("{}: connect failed, addr:{} port:{}", __FUNCTION__
                    , serverSocketAddr.mIpAddr.data(), serverSocketAddr.mPort);
            return tl::unexpected(errno);
        }
        auto socketFd = createNonblockingSocket(IP_PROTOCOL::IPv6);
        if (auto res = ::connect(socketFd, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr))
                ; res != 0 && failedResult(errno)) {
            LOG_ERR("{}: failed to connect {}", __FUNCTION__, serverSocketAddr.mIpAddr.data());
            return tl::unexpected(errno);
        } else if (res != 0) {
            LOG_INFO("{}: connect not done for {}", __FUNCTION__, serverSocketAddr.mIpAddr.data());
        }
        result.reset(new Socket(socketFd));
    }
    result->setLocalAddr(serverSocketAddr.mIpProtocol);
    result->setPeerAddr(std::move(serverSocketAddr));
    return result;
}

SocketPtr Socket::createTcpListenSocket(SocketAddr&& serverSocketAddr, int maxListenQueue) {
    std::unique_ptr<Socket> result;
    if (serverSocketAddr.mIpProtocol == IP_PROTOCOL::IPv4) {
        LOG_DEBUG("{}: IPv4", __FUNCTION__);
        auto socketFd = createNonblockingSocket(IP_PROTOCOL::IPv4);
        result.reset(new Socket(socketFd));
    } else {
        LOG_DEBUG("{}: IPv6", __FUNCTION__);
        auto socketFd = createNonblockingSocket(IP_PROTOCOL::IPv6);
        result.reset(new Socket(socketFd));
    }
    result->mIsListenSocket = true;
    result->mMaxListenQueue = maxListenQueue;
    result->mLocalAddr.mPort = serverSocketAddr.mPort;
    result->mLocalAddr.mIpProtocol = serverSocketAddr.mIpProtocol;
    return result;
}

void Socket::listen() {
    LOG_INFO("{}: start", __FUNCTION__);
    assertTrue(mIsListenSocket, "[Socket] listen just can be invoked by listen socket!");
    if (mLocalAddr.mIpProtocol == IP_PROTOCOL::IPv4) {
        LOG_DEBUG("{}: IPv4", __FUNCTION__);
        sockaddr_in serverAddr = {};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = ::htons(mLocalAddr.mPort);
        if (auto res = ::bind(getFd(), reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr)); res != 0) {
            LOG_FATAL("{}: bind error because {}.", __FUNCTION__, strerror(errno));
        }
        if (auto res = ::listen(getFd(), mMaxListenQueue); res != 0) {
            LOG_FATAL("{}: listen error because {}.", __FUNCTION__, strerror(errno));
        }
    } else {
        LOG_DEBUG("{}: IPv6", __FUNCTION__);
        sockaddr_in6 serverAddr = {};
        serverAddr.sin6_family = AF_INET6;
        serverAddr.sin6_port = ::htons(mLocalAddr.mPort);
        serverAddr.sin6_addr = IN6ADDR_ANY_INIT;
        if (auto res = ::bind(getFd(), reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr)); res != 0) {
            LOG_FATAL("{}: bind error because {}.", __FUNCTION__, strerror(errno));
        }
        if (auto res = ::listen(getFd(), mMaxListenQueue); res != 0) {
            LOG_FATAL("{}: listen error because {}.", __FUNCTION__, strerror(errno));
        }
    }
    LOG_INFO("{}: end", __FUNCTION__);
    setLocalAddr(mLocalAddr.mIpProtocol);
}

SocketResult Socket::accept() {
    LOG_INFO("{}: start", __FUNCTION__);
    // Accept connection.
    assertTrue(mIsListenSocket, "[Socket] accept just can be invoked by listen socket!");
    std::variant<sockaddr_in, sockaddr_in6> clientAddr;
    socklen_t addrLen = (getIpProtocol() == IP_PROTOCOL::IPv4)
            ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);
    auto connectedFd = ::accept4(getFd()
            , reinterpret_cast<sockaddr *>(&clientAddr), &addrLen
            , SOCK_CLOEXEC | SOCK_NONBLOCK);
    if (connectedFd < 0) {
        LOG_ERR("{}: failed to accept, {}", __FUNCTION__, gai_strerror(errno));
        return tl::unexpected(errno);
    }

    // Translate IP address.
    auto result = std::unique_ptr<Socket>(new Socket(connectedFd));
    SocketAddr peerAddr {};
    if (getIpProtocol() == IP_PROTOCOL::IPv4) {
        peerAddr.mIpAddr = transAddrToString(std::get<sockaddr_in>(clientAddr));
        peerAddr.mIpProtocol = IP_PROTOCOL::IPv4;
        peerAddr.mPort = ::ntohs(std::get<sockaddr_in>(clientAddr).sin_port);
    } else {
        peerAddr.mIpAddr = transAddrToString(std::get<sockaddr_in6>(clientAddr));
        peerAddr.mIpProtocol = IP_PROTOCOL::IPv6;
        peerAddr.mPort = ::ntohs(std::get<sockaddr_in6>(clientAddr).sin6_port);
    }
    result->setPeerAddr(std::move(peerAddr));
    result->setLocalAddr(getIpProtocol());
    // Return new connected socket..
    LOG_INFO("{}: end", __FUNCTION__);
    return result;
}

void Socket::shutdown() {
    LOG_INFO("{}", __FUNCTION__);
    auto res = ::shutdown(getFd(), SHUT_WR);
    if (res != 0) {
        LOG_ERR("{}: code({}) message({})", __FUNCTION__, errno, strerror(errno));
    }
}

void Socket::setNonBlock(bool enable) {
    int flags = 0;
    if (flags = ::fcntl(getFd(), F_GETFL, 0); flags != 0) {
        throw utils::SystemException("failed to setNonBlock");
    }
    if (enable) {
        flags |= O_NONBLOCK;
    } else {
        flags = (flags & (~O_NONBLOCK));
    }
    if (auto res = ::fcntl(getFd(), F_SETFL, flags); res != 0) {
        throw utils::SystemException("failed to setNonBlock");
    }
}

void Socket::setNonDelay(bool enable) {
    int flag = enable ? 1 : 0;
    if (auto res = ::setsockopt(getFd(), SOL_SOCKET, TCP_NODELAY, &flag, sizeof(flag)); res != 0) {
        throw utils::NetworkException("failed to setNonDelay.", res);
    }
}

void Socket::setKeepAlive(bool enable) {
    int flag = enable ? 1 : 0;
    if (auto res = ::setsockopt(getFd(), SOL_SOCKET, SO_KEEPALIVE, &flag, sizeof(flag)); res != 0) {
        throw utils::NetworkException("failed to setKeepAlive", res);
    }
}

void Socket::setReuseAddr(bool enable) {
    int flag = enable ? 1 : 0;
    if (auto res = ::setsockopt(getFd(), SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)); res != 0) {
        throw utils::NetworkException("failed to setReuseAddr", res);
    }
}

void Socket::setReusePort(bool enable) {
    int flag = enable ? 1 : 0;
    if (auto res = ::setsockopt(getFd(), SOL_SOCKET, SO_REUSEPORT, &flag, sizeof(flag)); res != 0) {
        throw utils::NetworkException("failed to setReusePort", res);
    }
}


void Socket::setLocalAddr(IP_PROTOCOL protocol) {
    if (protocol == IP_PROTOCOL::IPv4) {
        sockaddr_in addr;
        socklen_t addrLen = sizeof(addr);
        auto res = ::getsockname(getFd(), reinterpret_cast<sockaddr *>(&addr), &addrLen);
        if (res == 0) {
            SocketAddr localAddr {
                .mIpAddr = transAddrToString(addr),
                .mIpProtocol = IP_PROTOCOL::IPv4,
                .mPort = ::ntohs(addr.sin_port)
            };
            mLocalAddr = std::move(localAddr);
        } else {
            LOG_ERR("{}: failed to get local addr.", __FUNCTION__);
        }
    } else {
        sockaddr_in6 addr;
        socklen_t addrLen = sizeof(addr);
        auto res = ::getsockname(getFd(), reinterpret_cast<sockaddr *>(&addr), &addrLen);
        if (res == 0) {
            SocketAddr localAddr {
                .mIpAddr = transAddrToString(addr),
                .mIpProtocol = IP_PROTOCOL::IPv6,
                .mPort = ::ntohs(addr.sin6_port)
            };
            mLocalAddr = std::move(localAddr);
        } else {
            LOG_ERR("{}: failed to get local addr.", __FUNCTION__);
        }
    }
}

int Socket::getSocketError() const noexcept {
    int optval;
    socklen_t optlen = sizeof(optval);
    if (::getsockopt(getFd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
        return errno;
    } else {
        return optval;
    }
}

void Socket::dumpSocketInfo() const noexcept {
    tcp_info tcpInfo {};
    socklen_t len = sizeof(tcpInfo);
    if (::getsockopt(getFd(), SOL_SOCKET, TCP_INFO, &tcpInfo, &len) == 0) {
        LOG_DEBUG("{}: local addr {}, port {}", __FUNCTION__, mLocalAddr.mIpAddr, mLocalAddr.mPort);
        LOG_DEBUG("{}: peer  addr {}, port {}", __FUNCTION__, mPeerAddr.mIpAddr, mPeerAddr.mPort);
        LOG_DEBUG("{}: pmtu={}, state={}", __FUNCTION__, tcpInfo.tcpi_pmtu, tcpInfo.tcpi_state);
        LOG_DEBUG("{}: rtt={}, trrval={}, rto={}", __FUNCTION__
                , tcpInfo.tcpi_rtt, tcpInfo.tcpi_rttvar, tcpInfo.tcpi_rto);
        LOG_DEBUG("{}: send cwnd={}, mss={}, ssthresh={}, wscale={}", __FUNCTION__
                , tcpInfo.tcpi_snd_cwnd, tcpInfo.tcpi_snd_mss, tcpInfo.tcpi_snd_ssthresh, (int)tcpInfo.tcpi_snd_wscale);
        LOG_DEBUG("{}: recv space={}, mss={}, ssthresh={}, wscale={}", __FUNCTION__
                , tcpInfo.tcpi_rcv_space, tcpInfo.tcpi_rcv_mss, tcpInfo.tcpi_rcv_ssthresh, (int)tcpInfo.tcpi_rcv_wscale);
    } else {
        LOG_WARN("{}: failed to get tcp info!");
    }
}

} // namespace net
