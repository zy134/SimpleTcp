#include "tcp/TcpClient.h"
#include "base/Error.h"
#include "net/EventLoop.h"
#include "net/Channel.h"
#include "base/Utils.h"
#include "base/Log.h"
#include "net/Socket.h"
#include "tcp/TcpConnection.h"

#include <cstring>
#include <system_error>
#include <thread>
#include <chrono>

using namespace net;
using namespace utils;
using namespace std;
using namespace chrono_literals;

#ifdef TAG
#undef TAG
#endif
static constexpr std::string_view TAG = "TcpClient";

namespace net::tcp {

static SocketPtr connectToServer(SocketAddr&& serverAddr) {
    // Retry three times.
    for (int i = 0; i != 3; ++i) {
        auto result = Socket::createTcpClientSocket(std::move(serverAddr));
        if (result.has_value()) {
            LOG_INFO("%s : create socket success.", __FUNCTION__);
            return std::move(result.value());
        } else {
            switch (result.error()) {
                case EAGAIN:
                case EADDRINUSE:
                case EADDRNOTAVAIL:
                case ECONNREFUSED:
                case ENETUNREACH:
                    LOG_INFO("%s : retry connect.", __FUNCTION__);
                    std::this_thread::sleep_for(i * 1000ms);
                    // goto retry;
                    break;

                case EACCES:
                case EPERM:
                case EAFNOSUPPORT:
                case EALREADY:
                case EBADF:
                case EFAULT:
                case ENOTSOCK:
                default:
                    LOG_ERR("%s: error(%d), message(%s)", __FUNCTION__, result.error(), strerror(result.error()));
                    throw NetworkException("[TcpClient] connect error", result.error());
            }
        }
    }
    throw NetworkException("[TcpClient] retry connect error", errno);
}

TcpClient::TcpClient(EventLoop* loop, SocketAddr serverAddr) :mpEventLoop(loop) {
    LOG_INFO("%s +", __FUNCTION__);
    mpEventLoop->assertInLoopThread();
    // Create socket which connect to remote server.
    mConnSocket = connectToServer(std::move(serverAddr));
    LOG_INFO("%s -", __FUNCTION__);
}

void TcpClient::connect() {
    LOG_INFO("%s +", __FUNCTION__);
    mpEventLoop->assertInLoopThread();
    auto errCode = mConnSocket->getSocketError();
    LOG_INFO("%s socket errCode %d", __FUNCTION__, errCode);
    // Connect is not done, so use a temporary channel to wait for connection enable.
    if (errCode == EINPROGRESS || errCode == EINTR || errCode == EISCONN) {
        LOG_INFO("%s : connect in progress.", __FUNCTION__);
        mConnChannel = Channel::createChannel(mConnSocket->getFd(), mpEventLoop);
        mConnChannel->setErrorCallback([&] {
            auto errCode = mConnSocket->getSocketError();
            LOG_ERR("%s : connect error(%d) message(%s)", __FUNCTION__, errCode, strerror(errCode));
            throw NetworkException("[TcpClient] connect error", errCode);
        });
        mConnChannel->setWriteCallback([&] {
            LOG_INFO("%s : connect has done.", __FUNCTION__);
            mpEventLoop->queueInLoop([&] {
                createNewConnection();
            });
            // It's safe because there has no need to access mConnChannel again.
            mConnChannel = nullptr;
        });
        mConnChannel->enableWrite();
    } else if (errCode == 0) {
    // Connect is done, create a new TcpConnection.
        createNewConnection();
    } else {
        throw NetworkException("[TcpClient] connect error.", errCode);
    }
    LOG_INFO("%s -", __FUNCTION__);
}

void TcpClient::disconnect() {
    LOG_INFO("%s", __FUNCTION__);
    assertTrue(mConnection != nullptr, "[TcpClient] mConnection is empty!");
    if (mpEventLoop->isInLoopThread()) {
        mConnection->shutdownConnection();
    } else {
        mpEventLoop->queueInLoop([&] {
            mConnection->shutdownConnection();
        });
    }
}

// The life time of mConnection must be longer then connection!
TcpClient::~TcpClient() {
    LOG_INFO("%s +", __FUNCTION__);
    mpEventLoop->assertInLoopThread();
    if (!mConnection.unique()) {
        LOG_ERR("%s: Why there has more then one reference to this connection?", __FUNCTION__);
    }
    if (mConnection) {
        mConnection->destroyConnection();
        mConnection = nullptr;
    }
    LOG_INFO("%s -", __FUNCTION__);
}

void TcpClient::createNewConnection() {
    LOG_INFO("%s +", __FUNCTION__);
    mpEventLoop->assertInLoopThread();
    assertTrue(mConnSocket->getSocketError() == 0, "[TcpClient] bad socket!");
    mConnSocket->dumpSocketInfo();

    mConnection = TcpConnection::createTcpConnection(std::move(mConnSocket), mpEventLoop);
    mConnection->setMessageCallback(mMessageCb);
    mConnection->setWriteCompleteCallback(mWriteCompleteCb);
    mConnection->setConnectionCallback(mConnectionCb);
    // Passive close.
    // Must remove connection in loop thread.
    // EventLoop::poll() 
    //      => handleEvent()
    //          => Channel::mCloseCb()
    //              => TcpConnection::handleClose()
    //                  => TcpConnection::mCloseCb()
    //                      => TcpConnection::destroyConnection()
    //                          => Socket::~Socket()
    //                              => Channel::~Channel()
    //                                  => EventLoop::removeChannel()
    mConnection->setCloseCallback([&] (const TcpConnectionPtr& conn) {
        mpEventLoop->assertInLoopThread();
        auto guard = conn;
        conn->destroyConnection();
        mConnection = nullptr;
    });
    mConnection->establishConnect();
    LOG_INFO("%s -", __FUNCTION__);
}

} // namespace net::tcp
