#include "tcp/TcpServer.h"
#include "net/Channel.h"
#include "net/Socket.h"
#include "tcp/TcpBuffer.h"
#include "tcp/TcpConnection.h"
#include "base/Utils.h"
#include "base/Log.h"
#include <string_view>

extern "C" {
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
}

#ifdef TAG
#undef TAG
#endif
static constexpr std::string_view TAG = "TcpServer";

using namespace utils;
using namespace net;


namespace net::tcp {

TcpServer::TcpServer(net::EventLoop* loop, net::SocketAddr serverAddr, int maxListenQueue) : mpEventLoop(loop) {
    mpEventLoop->assertInLoopThread();
    LOG_INFO("%s +", __FUNCTION__);
    // create listen socket.
    mpListenSocket = Socket::createTcpListenSocket(std::move(serverAddr), maxListenQueue);
    mpListenSocket->setReuseAddr(true);
    mpListenSocket->setReusePort(true);

    // create listen channel.
    mpListenChannel = Channel::createChannel(mpListenSocket->getFd(), loop);

    mpListenChannel->setReadCallback([&] {
        LOG_INFO("[ReadCallback] new client is arrived.");
        createNewConnection();
    });

    mpListenChannel->setCloseCallback([&] {
        auto errCode = mpListenSocket->getSocketError();
        LOG_FATAL("%s: Error happen for server! Error code:%d, %s", __FUNCTION__
                , errCode, gai_strerror(errCode));
    });

    mpListenChannel->setErrorCallback([&] {
        auto errCode = mpListenSocket->getSocketError();
        LOG_ERR("%s: Error happen for server! Error code:%d, %s", __FUNCTION__
                , errCode, gai_strerror(errCode));
    });
    LOG_INFO("%s -", __FUNCTION__);
}

TcpServer::~TcpServer() {
    LOG_INFO("%s", __FUNCTION__);
    mpEventLoop->assertInLoopThread();
    mConnectionSet.clear();
    mpListenChannel = nullptr;
    mpListenSocket = nullptr;
}

// Must run in loop.
void TcpServer::start() {
    LOG_INFO("%s", __FUNCTION__);
    mpEventLoop->assertInLoopThread();
    mpListenSocket->listen();
    mpListenChannel->enableRead();
}

void TcpServer::setConnectionCallback(TcpConnectionCallback &&cb) noexcept {
    mConnectionCb = std::move(cb);
}

void TcpServer::setMessageCallback(TcpMessageCallback &&cb) noexcept {
    mMessageCb = std::move(cb);
}

void TcpServer::setWriteCompleteCallback(TcpWriteCompleteCallback &&cb) noexcept {
    mWriteCompleteCb = std::move(cb);
}

// Callback for Channel::handleEvent(), so it is run in loop thread.
void TcpServer::createNewConnection() {
    LOG_INFO("%s", __FUNCTION__);
    mpEventLoop->assertInLoopThread();
    auto clientSocket = mpListenSocket->accept();
    if (!clientSocket) {
        LOG_ERR("%s: accept error %s", __FUNCTION__, gai_strerror(clientSocket.error()));
        return ;
    }
    clientSocket.value()->dumpSocketInfo();
    auto newConn = TcpConnection::createTcpConnection(std::move(clientSocket.value()), mpEventLoop);

    // Copy user callback function to new connection.
    newConn->setConnectionCallback(mConnectionCb);
    newConn->setMessageCallback(mMessageCb);
    newConn->setWriteCompleteCallback(mWriteCompleteCb);

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
    newConn->setCloseCallback([&] (const TcpConnectionPtr& conn) {
            mpEventLoop->assertInLoopThread();
            auto guard = conn;
            conn->destroyConnection();
            mConnectionSet.erase(guard);
        }
    );
    // TODO: check weather a run-condition occurs.
    newConn->establishConnect();
    mConnectionSet.insert(std::move(newConn));
}

} // namespace net::tcp
