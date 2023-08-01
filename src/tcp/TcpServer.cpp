#include "tcp/TcpServer.h"
#include "net/Channel.h"
#include "net/Socket.h"
#include "tcp/TcpBuffer.h"
#include "tcp/TcpConnection.h"
#include "base/Utils.h"
#include "base/Log.h"
#include "base/Error.h"
#include <exception>
#include <string_view>

extern "C" {
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
}

static constexpr std::string_view TAG = "TcpServer";

using namespace simpletcp;
using namespace simpletcp::net;


namespace simpletcp::tcp {

TcpServer::TcpServer(net::EventLoop* loop, net::SocketAddr serverAddr, int maxListenQueue) : mpEventLoop(loop) {
    mpEventLoop->assertInLoopThread();
    LOG_INFO("{} +", __FUNCTION__);
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

    // When error happen in listen socket, exit tcp server.
    mpListenChannel->setCloseCallback([&] {
        auto errCode = mpListenSocket->getSocketError();
        LOG_FATAL("{}: Error happen for server! Error code:{}, {}", __FUNCTION__
                , errCode, gai_strerror(errCode));
    });

    mpListenChannel->setErrorCallback([&] {
        auto errCode = mpListenSocket->getSocketError();
        LOG_FATAL("{}: Error happen for server! Error code:{}, {}", __FUNCTION__
                , errCode, gai_strerror(errCode));
    });

    // When new connection is established, create new idenfication for TcpClient.
    mIdentification = "";
    mIdentification = simpletcp::format("{:016d}_{:05d}_{}_{}"
        , std::chrono::steady_clock::now().time_since_epoch().count()
        , gettid()
        , mpListenSocket->getLocalAddr().mPort
        , mpListenSocket->getLocalAddr().mIpAddr
    );
    LOG_INFO("{}: New Sever {}", __FUNCTION__, mIdentification);
    LOG_INFO("{} -", __FUNCTION__);
}

TcpServer::~TcpServer() noexcept {
    LOG_INFO("{}", __FUNCTION__);
    mpEventLoop->assertInLoopThread();
    mConnectionSet.clear();
    mpListenChannel = nullptr;
    mpListenSocket = nullptr;
}

// Must run in loop.
void TcpServer::start() {
    LOG_INFO("{}", __FUNCTION__);
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

void TcpServer::setHighWaterMarkCallback(TcpHighWaterMarkCallback &&cb) noexcept {
    mHighWaterMarkCb = std::move(cb);
}

// Callback for Channel::handleEvent(), so it is run in loop thread.
void TcpServer::createNewConnection() {
    LOG_INFO("{}", __FUNCTION__);
    mpEventLoop->assertInLoopThread();
    SocketPtr clientSocket;
    try {
        clientSocket = mpListenSocket->accept();
    } catch (const NetworkException& e) {
        LOG_ERR("{}: Ignore exception: {}", e.what());
    } catch (const std::exception& e) {
        LOG_ERR("{}: {}", e.what());
        throw;
    }
    if (mConnectionSet.size() == MAX_CONNECTION_NUMS) {
        LOG_ERR("{}: refuse connect because connection set is full.", __FUNCTION__);
        return ;
    }
    clientSocket->dumpSocketInfo();
    auto newConn = TcpConnection::createTcpConnection(std::move(clientSocket), mpEventLoop);

    // Copy user callback function to new connection.
    newConn->setConnectionCallback(mConnectionCb);
    newConn->setMessageCallback(mMessageCb);
    newConn->setWriteCompleteCallback(mWriteCompleteCb);
    newConn->setHighWaterMarkCallback(mHighWaterMarkCb);

    // Must remove connection in loop thread.
    // EventLoop::poll()
    //      => handleEvent()
    //          => Channel::mCloseCb()
    //              => TcpConnection::handleClose()
    //                  => TcpConnection::mCloseCb()
    //                      => EventLoop::queueInLoop()
    //
    //      => ~Channel()
    //          => EventLoop::removeChannel()
    //              => ~Socket()
    //                  => ~TcpConnection()
    newConn->setCloseCallback([&] (const TcpConnectionPtr& conn) {
        mpEventLoop->queueInLoop([&, guard = conn] {
            guard->destroyConnection();
            mConnectionSet.erase(guard);
        });
    });
    // TODO: check weather a run-condition occurs.
    newConn->establishConnect();
    mConnectionSet.insert(std::move(newConn));
}

} // namespace net::tcp
