#include <base/Error.h>
#include <fmt/format.h>
#include <base/Log.h>
#include <base/Utils.h>
#include <net/Channel.h>
#include <net/EventLoop.h>
#include <net/Socket.h>
#include <tcp/TcpBuffer.h>
#include <tcp/TcpConnection.h>
#include <bits/types/struct_tm.h>
#include <exception>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <string_view>
#include <system_error>

// define TAG as a expression to print idenfication of connection
const static std::string DEF_TAG = "TcpConnection";
#define TAG (mIdentification.empty() ? DEF_TAG : mIdentification)

constexpr auto TCP_HIGH_WATER_MARK = 65535;

using namespace std;
using namespace simpletcp;
using namespace simpletcp::net;

namespace simpletcp::tcp {

TcpConnectionPtr TcpConnection::createTcpConnection(net::SocketPtr&& socket, net::EventLoop* loop) {
    return std::shared_ptr<TcpConnection>(new TcpConnection(std::move(socket), loop));
}

TcpConnection::TcpConnection(SocketPtr&& socket, net::EventLoop* loop)
        : mpEventLoop(loop), mpSocket(std::move(socket)), mState(ConnState::DisConnected) {
    LOG_INFO("{}: E", __FUNCTION__);
    LOG_INFO("{}: owner loop :{}", __FUNCTION__, static_cast<void *>(mpEventLoop));

    mLocalAddr = mpSocket->getLocalAddr();
    mPeerAddr = mpSocket->getPeerAddr();
    mIdentification = fmt::format("ConnId_{}_{}_{}_{}"
        , mpSocket->getLocalAddr().mIpAddr, mpSocket->getLocalAddr().mPort
        , mpSocket->getPeerAddr().mIpAddr, mpSocket->getPeerAddr().mPort
    );

    mpChannel = net::Channel::createChannel(mpSocket->getFd(), loop);
    mpChannel->setChannelInfo(mIdentification);
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
    if (mState == ConnState::Connected || mState == ConnState::HalfClosed) {
        mState = ConnState::DisConnected;
        destroyConnection();
        LOG_ERR("{}: The connection is not shutdown, but dtor has invoked...", __FUNCTION__);
        LOG_ERR("{}: current exceptions {}", __FUNCTION__, std::uncaught_exceptions());
    }
    LOG_INFO("{}: X", __FUNCTION__);
}

// Read data from socket to receive buffer
// Read operation may invoked asynchronously, so need a lock to protect receive buffer.
void TcpConnection::handleRead() {
    TRACE();
    mpEventLoop->assertInLoopThread();
    assertTrue(mState == ConnState::Connected || mState == ConnState::HalfClosed
            , "[TcpConnection] invoke handleRead in a bad connection!");
    auto scopeGuard = shared_from_this();
    try {
        {
        // Avoid dead lock.
            std::lock_guard lock { mRecvMutex };
            mRecvBuffer.readFromSocket(mpSocket);
        }
        // What message callback may doing:
        // 1. send: safe, it always run in loop thread.
        // 2. read: safe, use a mutex to protect receive buffer.
        // 3. shutdownConnection: safe, it always run in loop thread.
        if (mMessageCb) {
            mMessageCb(scopeGuard);
        }
    } catch (const NetworkException& e) {
        if (e.getNetErr() == 0) {
            LOG_INFO("{} remote socket is shutdown.", __FUNCTION__);
            handleClose();
        } else {
            LOG_ERR("{} error happen", __FUNCTION__);
            handleError();
        }
    }
}

// Write operation is always in loop thread, so no need to lock.
void TcpConnection::handleWrite() {
    TRACE();
    mpEventLoop->assertInLoopThread();
    assertTrue(mState == ConnState::Connected, "[TcpConnection] invoke handleWrite in a bad connection!");
    auto scopeGuard = shared_from_this();
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
// If handleClose has been invoked, handleRead and handleWrite would not be invoked later.
void TcpConnection::handleClose() {
    TRACE();
    auto errCode = mpSocket->getSocketError();
    LOG_ERR("{}: code({}) message({})", __FUNCTION__, errCode, strerror(errCode));
    mpEventLoop->assertInLoopThread();
    assertTrue(mState == ConnState::Connected || mState == ConnState::HalfClosed
            , "[TcpConnection] handleClose: destroy a bad connection!");

    mState = ConnState::DisConnected;
    auto scopeGuard = shared_from_this();
    if (mCloseCb) {
        // mCloseCb is a lambda wraper destroyConnection()
        mCloseCb(scopeGuard);
    }
    if (mConnectionCb) {
        mConnectionCb(scopeGuard);
    }
}

// Don't need to close connection when error happen. Because the socket change to be readable latter and
// 0 bytes would be read from socket, then the connection will be closed.
void TcpConnection::handleError() {
    TRACE();
    auto errCode = mpSocket->getSocketError();
    LOG_ERR("{}: code({}) message({})", __FUNCTION__, errCode, strerror(errCode));
    mpEventLoop->assertInLoopThread();
}

// We use std::string but not const std::string& or string_view, because we should
// hold the string in different thread. So make problem simpler, we need a copy.
void TcpConnection::send(span_type data) {
    // In loop thread, write to buffer directly.
    if (mpEventLoop->isInLoopThread()) {
        sendInLoop(data);
    } else {
    // Not in loop thread, copy the input buffer and send it to loop thread.
        std::vector<TcpBuffer::char_type> tmp { data.begin(), data.end() };
        mpEventLoop->queueInLoop([tmp = std::move(tmp), this] () mutable {
            sendInLoop({ tmp.data(), tmp.size() });
        });
    }
}

void TcpConnection::send(buffer_type&& data) {
    // In loop thread, write to buffer directly.
    if (mpEventLoop->isInLoopThread()) {
        sendInLoop(data);
    } else {
    // Not in loop thread, move the input buffer and send it to loop thread.
        mpEventLoop->queueInLoop([data = std::move(data), this] () mutable {
            sendInLoop(data);
        });
    }
}

void TcpConnection::sendString(std::string_view message) {
    // In loop thread, write to buffer directly.
    if (mpEventLoop->isInLoopThread()) {
        sendInLoop({ reinterpret_cast<const TcpBuffer::char_type *>(message.data()), message.size() });
    } else {
    // Not in loop thread, copy the input buffer and send it to loop thread.
        std::string s { message.data(), message.size() };
        mpEventLoop->queueInLoop([s = std::move(s), this] () mutable {
            sendInLoop({ reinterpret_cast<const TcpBuffer::char_type *>(s.data()), s.size() });
        });
    }
}

void TcpConnection::sendString(std::string&& message) {
    // In loop thread, write to buffer directly.
    if (mpEventLoop->isInLoopThread()) {
        sendInLoop({ reinterpret_cast<const TcpBuffer::char_type *>(message.data()), message.size() });
    } else {
    // Not in loop thread, move the input buffer and send it to loop thread.
        mpEventLoop->queueInLoop([message = std::move(message), this] () mutable {
            sendInLoop({ reinterpret_cast<const TcpBuffer::char_type *>(message.data()), message.size() });
        });
    }
}

void TcpConnection::sendInLoop(span_type data) {
    TRACE();
    mpEventLoop->assertInLoopThread();
    if (!isConnected()) {
        LOG_ERR("{}: remote connection is shutdown!", __FUNCTION__);
        return ;
    }
    mSendBuffer.appendToBuffer(data);
    // TODO: slow down throught of socket.
    if (mSendBuffer.size() > TCP_HIGH_WATER_MARK && mHighWaterMarkCb) {
        mHighWaterMarkCb(shared_from_this());
    }
    if (!mpChannel->isWriting()) {
        mpChannel->enableWrite();
    }
}

TcpBuffer::span_type TcpConnection::read(size_t size) noexcept {
    TRACE();
    std::lock_guard lock { mRecvMutex };
    return mRecvBuffer.read(size);
}

TcpBuffer::span_type TcpConnection::readAll() noexcept {
    TRACE();
    std::lock_guard lock { mRecvMutex };
    auto size = mRecvBuffer.size();
    return mRecvBuffer.read(size);
}

std::vector<uint8_t> TcpConnection::extract(size_t size) noexcept {
    TRACE();
    std::lock_guard lock { mRecvMutex };
    return mRecvBuffer.extract(size);
}

std::vector<uint8_t> TcpConnection::extractAll() noexcept {
    TRACE();
    std::lock_guard lock { mRecvMutex };
    auto size = mRecvBuffer.size();
    return mRecvBuffer.extract(size);
}

std::string_view TcpConnection::readString(size_t size) noexcept {
    TRACE();
    std::lock_guard lock { mRecvMutex };
    return mRecvBuffer.readString(size);
}

std::string_view TcpConnection::readStringAll() noexcept {
    TRACE();
    std::lock_guard lock { mRecvMutex };
    auto size = mRecvBuffer.size();
    return mRecvBuffer.readString(size);
}

std::string TcpConnection::extractString(size_t size) noexcept {
    TRACE();
    std::lock_guard lock { mRecvMutex };
    return mRecvBuffer.extractString(size);
}

std::string TcpConnection::extractStringAll() noexcept {
    TRACE();
    std::lock_guard lock { mRecvMutex };
    auto size = mRecvBuffer.size();
    return mRecvBuffer.extractString(size);
}

// in loop thread.
// just invoked by TcpClient/TcpServer
void TcpConnection::establishConnect() {
    LOG_INFO("{}", __FUNCTION__);
    mpEventLoop->assertInLoopThread();
    mpChannel->enableRead();
    mState = ConnState::Connected;
    if (mConnectionCb) {
        mConnectionCb(shared_from_this());
    }
}

// in loop thread.
// just invoked by TcpClient/TcpServer
void TcpConnection::destroyConnection() noexcept {
    LOG_INFO("{}", __FUNCTION__);
    mpEventLoop->assertInLoopThread();
    assertTrue(mState == ConnState::DisConnected, "[TcpConnection] destroyConnection: must set setState as DisConnected!");
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
    mpEventLoop->queueInLoop([this] {
        try {
            LOG_INFO("shutdownConnection in loop thread");
            if (mState == ConnState::DisConnected) {
                LOG_WARN("shutdownConnection: connection is already closed.");
                return ;
            }
            if (mSendBuffer.size() != 0) {
                LOG_INFO("shutdownConnection: write buffer before shutdown");
                mSendBuffer.writeToSocket(mpSocket);
            }
            mState = ConnState::HalfClosed;
            mpChannel->disableWrite();
            mpSocket->shutdown();
        } catch (const std::exception& e) {
            LOG_ERR("shutdownConnection: {}", e.what());
        }
    });
}

} // namespace simpletcp::tcp
