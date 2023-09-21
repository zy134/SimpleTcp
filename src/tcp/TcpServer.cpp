#include <tcp/TcpServer.h>
#include <fmt/format.h>
#include <net/Channel.h>
#include <net/Socket.h>
#include <tcp/TcpBuffer.h>
#include <tcp/TcpConnection.h>
#include <base/Utils.h>
#include <base/Log.h>
#include <base/Error.h>
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

TcpServer::TcpServer(TcpServerArgs args)
    : mpEventLoop(args.loop), mEventLoopPool(args.loop, args.maxThreadNum) {
    assertTrue(mpEventLoop != nullptr, "[TcpServer] loop must not be none!");
    assertTrue(args.maxListenQueue > 0, "[TcpServer] maxListenQueue must bigger than 0");
    mpEventLoop->assertInLoopThread();
    LOG_INFO("{}: E", __FUNCTION__);
    LOG_INFO("{}: owner loop :{}", __FUNCTION__, static_cast<void *>(mpEventLoop));
    // create listen socket.
    mpListenSocket = Socket::createTcpListenSocket(std::move(args.serverAddr), args.maxListenQueue);
    mpListenSocket->setReuseAddr(true);
    mpListenSocket->setReusePort(true);

    // create listen channel.
    mpListenChannel = Channel::createChannel(mpListenSocket->getFd(), args.loop);

    auto channelInfo = fmt::format("Server listen in port {}", mpListenSocket->getPort());
    mpListenChannel->setChannelInfo(channelInfo);
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
    mIdentification = fmt::format("{:016d}_{:05d}_{}_{}"
        , std::chrono::steady_clock::now().time_since_epoch().count()
        , gettid()
        , mpListenSocket->getLocalAddr().mPort
        , mpListenSocket->getLocalAddr().mIpAddr
    );
    LOG_INFO("{}: New Sever {}", __FUNCTION__, mIdentification);
    LOG_INFO("{}: X", __FUNCTION__);
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
    getLoop()->assertInLoopThread();
    if (mEventLoopPool.getLoopNums() == 0) {
        createNewConnectionInSubLoop(mpEventLoop);
    } else {
        // Get a loop from loop pool and create new connection in new loop.
        auto newLoop = mEventLoopPool.acquireLoop();
        // Execute task asynchronously and wait for it to complete.
        // Because listen fd is available now, no need to use queueInLoop to delay the task.
        newLoop->runInLoop([this, newLoop] {
            createNewConnectionInSubLoop(newLoop);
        });
    }
}

void TcpServer::createNewConnectionInSubLoop(EventLoop* newLoop) {
    LOG_INFO("{}: E", __FUNCTION__);
    newLoop->assertInLoopThread();
    mpEventLoop->assertInLoopThread();
    SocketPtr clientSocket;
    try {
        clientSocket = mpListenSocket->accept();
    } catch (const NetworkException& e) {
        LOG_ERR("{}: Ignore exception: {}", __FUNCTION__, e.what());
        throw;
    } catch (const std::exception& e) {
        LOG_ERR("{}: {}", __FUNCTION__, e.what());
        throw;
    }
    if (mConnectionSet.size() == MAX_CONNECTION_NUMS) {
        LOG_ERR("{}: refuse connect because connection set is full.", __FUNCTION__);
        // Socket would be close when leave this scope.
        return ;
    }
    clientSocket->dumpSocketInfo();
    // Assign a loop for new connection.
    auto newConn = TcpConnection::createTcpConnection(std::move(clientSocket), newLoop);

    // Copy user callback function to new connection.
    newConn->setConnectionCallback(mConnectionCb);
    newConn->setMessageCallback(mMessageCb);
    newConn->setWriteCompleteCallback(mWriteCompleteCb);
    newConn->setHighWaterMarkCallback(mHighWaterMarkCb);

    // Must remove connection in loop thread of TcpServer.
    // EventLoop::poll()
    //
    //      => handleEvent()
    //          => Channel::mCloseCb()
    //              => TcpConnection::handleClose()
    //                  => TcpConnection::mCloseCb()
    //                      => EventLoop::queueInLoop()
    //      => destroyConnection()
    //          => ~Channel()
    //              => EventLoop::removeChannel()
    //                  => ~Socket()
    //                      => ~TcpConnection()
    newConn->setCloseCallback([this] (const TcpConnectionPtr& conn) {
            // destroy connection in sub loop.
            auto guard = conn;
            conn->getLoop()->queueInLoop([guard, this] {
                guard->destroyConnection();
                // remove connection.
                {
                    std::lock_guard lock { mConnMutex };
                    mConnectionSet.erase(guard);
                }
            });
    });
    newConn->establishConnect();

    {
        std::lock_guard lock { mConnMutex };
        mConnectionSet.insert(std::move(newConn));
    }
    LOG_INFO("{}: X", __FUNCTION__);
}

} // namespace net::tcp
