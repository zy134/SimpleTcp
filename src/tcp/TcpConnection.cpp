#include "base/Error.h"
#include "base/Log.h"
#include "base/Utils.h"
#include "net/Channel.h"
#include "net/EventLoop.h"
#include "net/Socket.h"
#include "tcp/TcpConnection.h"
#include <cstddef>
#include <cstring>
#include <exception>
#include <memory>
#include <mutex>
#include <string_view>
#include <system_error>

static constexpr std::string_view TAG = "TcpConnection";

constexpr auto TCP_HIGH_WATER_MARK = 65535;

using namespace utils;
using namespace std;

namespace net::tcp {

TcpConnectionPtr TcpConnection::createTcpConnection(net::SocketPtr&& socket, net::EventLoop* loop) {
    LOG_INFO("{}", __FUNCTION__);
    return std::shared_ptr<TcpConnection>(new TcpConnection(std::move(socket), loop));
}

TcpConnection::TcpConnection(SocketPtr&& socket, net::EventLoop* loop)
        : mpEventLoop(loop), mpSocket(std::move(socket)), mState(TcpState::DisConnected) {
    LOG_INFO("{}: E", __FUNCTION__);

    mLocalAddr = mpSocket->getLocalAddr();
    mPeerAddr = mpSocket->getPeerAddr();

    mpChannel = net::Channel::createChannel(mpSocket->getFd(), loop);
    mpChannel->setWriteCallback([this] () {
        handleWrite();
    });
    mpChannel->setReadCallback([this] () {
        handleRead();
    });
    mpChannel->setErrorCallback([this] () {
        handleError();
    });
    mpChannel->setCloseCallback([this] () {
        handleClose();
    });
    LOG_INFO("{}: X", __FUNCTION__);
}

TcpConnection::~TcpConnection() noexcept {
    LOG_INFO("{}: E", __FUNCTION__);
    if (mState == TcpState::Connected || mState == TcpState::HalfClosed) {
        destroyConnection();
        mState = TcpState::DisConnected;
        LOG_INFO("{}: The connection is not shutdown, but dtor has invoked...", __FUNCTION__);
    }
    LOG_INFO("{}: X", __FUNCTION__);
}

// Read data from socket to receive buffer
void TcpConnection::handleRead() {
    TRACE();
    mpEventLoop->assertInLoopThread();
    auto scopeGuard = shared_from_this();
    try {
        std::lock_guard lock { mRecvMutex };
        assertTrue(mState == TcpState::Connected || mState == TcpState::HalfClosed
                , "[TcpConnection] invoke handleRead in a bad connection!");
        mRecvBuffer.readFromSocket(mpSocket);
        if (mMessageCb) {
            mMessageCb(scopeGuard, mRecvBuffer);
        }
    } catch (utils::NetworkException& e) {
        if (e.getNetErr() == 0) {
            LOG_INFO("{} remote socket is shutdown.", __FUNCTION__);
            handleClose();
        } else {
            LOG_ERR("{} error happen", __FUNCTION__);
            handleError();
        }
    }
}

void TcpConnection::handleWrite() {
    TRACE();
    mpEventLoop->assertInLoopThread();
    auto scopeGuard = shared_from_this();
    std::lock_guard lock { mSendMutex };
    assertTrue(mState == TcpState::Connected, "[TcpConnection] invoke handleWrite in a bad connection!");
    if (mSendBuffer.size() != 0) {
        mSendBuffer.writeToSocket(mpSocket);
    }
    if (mSendBuffer.size() == 0) {
        // TODO
        // Delay callback function in loop thread.
        if (mWriteCompleteCb) {
            mWriteCompleteCb(scopeGuard);
        }
        mpChannel->disableWrite();
    }
}

// Call by Channel, must run in loop.
void TcpConnection::handleClose() {
    TRACE();
    auto errCode = mpSocket->getSocketError();
    LOG_ERR("{}: code({}) message({})", __FUNCTION__, errCode, strerror(errCode));
    mpEventLoop->assertInLoopThread();

    std::scoped_lock lock { mRecvMutex, mSendMutex };
    mState = TcpState::DisConnected;
    auto scopeGuard = shared_from_this();
    if (mConnectionCb) {
        mConnectionCb(scopeGuard);
    }
    if (mCloseCb) {
        // destroyConnection()
        mCloseCb(scopeGuard);
    }
}

// Don't need to close connection when error happen. Because the socket change to be readable latter and
// 0 bytes would be read from socket, then the connection be close.
void TcpConnection::handleError() {
    TRACE();
    auto errCode = mpSocket->getSocketError();
    LOG_ERR("{}: code({}) message({})", __FUNCTION__, errCode, strerror(errCode));
    mpEventLoop->assertInLoopThread();
}

// We use std::string but not const std::string& or string_view, because we should
// hold the string in different thread. So make problem simpler, we need a copy.
void TcpConnection::send(std::string_view message) {
    TRACE();
    std::lock_guard lock { mSendMutex };
    if (!isConnected()) {
        LOG_ERR("{}: remote connection is shutdown!", __FUNCTION__);
        return ;
    }
    mSendBuffer.appendToBuffer(message);
    if (mSendBuffer.size() > TCP_HIGH_WATER_MARK && mHighWaterMarkCb) {
        mpEventLoop->queueInLoop([&, guard = shared_from_this()] {
            mHighWaterMarkCb(guard);
        });
    }
    if (!mpChannel->isWriting()) {
        if (mpEventLoop->isInLoopThread()) {
            mpChannel->enableWrite();
        } else {
            mpEventLoop->queueInLoop([&] {
                mpChannel->enableWrite();
            });
        }
    }
}

std::string TcpConnection::read(size_t size) noexcept {
    TRACE();
    std::lock_guard lock { mRecvMutex };
    return mRecvBuffer.read(size);
}

std::string TcpConnection::readAll() noexcept {
    TRACE();
    std::lock_guard lock { mRecvMutex };
    return mRecvBuffer.readAll();
}

std::string TcpConnection::extract(size_t size) noexcept {
    TRACE();
    std::lock_guard lock { mRecvMutex };
    return mRecvBuffer.extract(size);
}

// in loop thread.
void TcpConnection::establishConnect() {
    LOG_INFO("{}", __FUNCTION__);
    mpEventLoop->assertInLoopThread();
    mpChannel->enableRead();
    mState = TcpState::Connected;
    if (mConnectionCb) {
        mConnectionCb(shared_from_this());
    }
}

// in loop thread.
void TcpConnection::destroyConnection() noexcept {
    LOG_INFO("{}", __FUNCTION__);
    mpEventLoop->assertInLoopThread();
    try {
        // remove Channel from EventLoop
        if (mpChannel) {
            mpChannel->disableAll();
            mpChannel = nullptr;
        } else {
            LOG_WARN("{}: destroy on a bad channel!", __FUNCTION__);
        }
        // shutdown file descriptor
        mpSocket = nullptr;
    } catch (const std::exception& e) {
        LOG_ERR("{}: {}", __FUNCTION__, e.what());
    }
}

// Internal interface call by TcpClient, must run in loop.
void TcpConnection::shutdownConnection() noexcept {
    LOG_INFO("{}", __FUNCTION__);
    mpEventLoop->assertInLoopThread();
    try {
        std::scoped_lock lock { mRecvMutex, mSendMutex };
        mState = TcpState::HalfClosed;
        mpChannel->disableWrite();
        mpSocket->shutdown();
    } catch (const std::exception& e) {
        LOG_ERR("{}: {}", __FUNCTION__, e.what());
    }
}

} // namespace net::tcp
