#include "base/Error.h"
#include "base/Log.h"
#include "base/Utils.h"
#include "net/Channel.h"
#include "net/EventLoop.h"
#include "net/Socket.h"
#include "tcp/TcpConnection.h"
#include <cstring>
#include <exception>
#include <memory>
#include <mutex>
#include <string_view>
#include <system_error>

#ifdef TAG
#undef TAG
#endif
static constexpr std::string_view TAG = "TcpConnection";

using namespace utils;
using namespace std;

class ScopeTracer {
public:
    ScopeTracer(std::string_view funcName) : mFuncName(funcName) { LOG_DEBUG("%s +", funcName.data()); }
    ~ScopeTracer() { LOG_DEBUG("%s -", mFuncName.data()); }
private:
    std::string_view mFuncName;
};
#define TRACE() ScopeTracer __scoper_tracer__(__FUNCTION__);

namespace net::tcp {

TcpConnectionPtr TcpConnection::createTcpConnection(net::SocketPtr&& socket, net::EventLoop* loop) {
    LOG_INFO("%s", __FUNCTION__);
    return std::shared_ptr<TcpConnection>(new TcpConnection(std::move(socket), loop));
}

TcpConnection::TcpConnection(SocketPtr&& socket, net::EventLoop* loop)
        : mpEventLoop(loop), mpSocket(std::move(socket)), mState(TcpState::DisConnected) {
    LOG_INFO("%s +", __FUNCTION__);

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
    LOG_INFO("%s -", __FUNCTION__);
}

TcpConnection::~TcpConnection() {
    LOG_INFO("%s", __FUNCTION__);
    // destroyConnection();
    // if (mCloseCb)
    //     mCloseCb(shared_from_this());
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
            LOG_INFO("%s remote socket is shutdown.", __FUNCTION__);
            handleClose();
        } else {
            LOG_ERR("%s error happen", __FUNCTION__);
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

void TcpConnection::handleClose() {
    TRACE();
    auto errCode = mpSocket->getSocketError();
    LOG_ERR("%s: code(%d) message(%s)", __FUNCTION__, errCode, strerror(errCode));
    mpEventLoop->assertInLoopThread();

    std::scoped_lock lock { mRecvMutex, mSendMutex };
    mState = TcpState::DisConnected;
    auto scopeGuard = shared_from_this();
    if (mConnectionCb) {
        mConnectionCb(scopeGuard); // invoke destroyConnection
    }
    if (mCloseCb) {
        mCloseCb(scopeGuard);
    }
}

// Don't need to close connection when error happen. Because the socket change to be readable latter and
// 0 bytes would be read from socket, then the connection be close.
void TcpConnection::handleError() {
    TRACE();
    auto errCode = mpSocket->getSocketError();
    LOG_ERR("%s: code(%d) message(%d)", __FUNCTION__, errCode, strerror(errCode));
    mpEventLoop->assertInLoopThread();
}

// We use std::string but not const std::string& or string_view, because we should
// hold the string in different thread. So make problem simpler, we need a copy.
void TcpConnection::send(std::string_view message) {
    TRACE();
    std::lock_guard lock { mSendMutex };
    if (!isConnected()) {
        LOG_ERR("%s: remote connection is shutdown!", __FUNCTION__);
        return ;
    }
    mSendBuffer.appendToBuffer(std::move(message));
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
    LOG_INFO("%s", __FUNCTION__);
    mpEventLoop->assertInLoopThread();
    mpChannel->enableRead();
    mState = TcpState::Connected;
    if (mConnectionCb) {
        mConnectionCb(shared_from_this());
    }
}

// in loop thread.
void TcpConnection::destroyConnection() noexcept {
    LOG_INFO("%s", __FUNCTION__);
    mpEventLoop->assertInLoopThread();
    try {
        // remove Channel from EventLoop
        mpChannel->disableAll();
        mpChannel = nullptr;
        // shutdown file descriptor
        mpSocket = nullptr;
    } catch (const std::exception& e) {
        LOG_ERR("%s: %s", __FUNCTION__, e.what());
    }
}

void TcpConnection::shutdownConnection() noexcept {
    LOG_INFO("%s", __FUNCTION__);
    try {
        std::scoped_lock lock { mRecvMutex, mSendMutex };
        mState = TcpState::HalfClosed;
        mpChannel->disableWrite();
        mpSocket->shutdown();
    } catch (const std::exception& e) {
        LOG_ERR("%s: %s", __FUNCTION__, e.what());
    }
}

} // namespace net::tcp
