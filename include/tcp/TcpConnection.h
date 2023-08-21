#pragma once

#include "base/Utils.h"
#include "base/Mutex.h"
#include "net/Channel.h"
#include "net/EventLoop.h"
#include "net/Socket.h"
#include "tcp/TcpBuffer.h"
#include <cstddef>
#include <cstdint>
#include <fmt/core.h>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <span>
#include <vector>

namespace simpletcp::tcp {

/*
 * TcpConnection
 * Wrapper of connected socket, TcpConnection manage the life cycle of socket by RAII tech.
 *
 *
 * */

class TcpConnection;


// poll Channels => invoke Channel callbacks => invoke Connection callbacks
// All callbacks is run in loop thread.
// TODO:
// Use std::move_only_function or folly:Function instead of std::function
using TcpConnectionPtr          = std::shared_ptr<TcpConnection>;
using TcpConnectionCallback     = std::function<void (const TcpConnectionPtr&)>;
using TcpMessageCallback        = std::function<void (const TcpConnectionPtr&)>;
using TcpCloseCallback          = std::function<void (const TcpConnectionPtr&)>;
using TcpWriteCompleteCallback  = std::function<void (const TcpConnectionPtr&)>;
using TcpHighWaterMarkCallback  = std::function<void (const TcpConnectionPtr&)>;


class TcpConnection final : public std::enable_shared_from_this<TcpConnection> {
    // The state of Tcp connection.
    enum class ConnState {
        Connected,
        HalfClosed,
        DisConnected
    };
    // State machine:
    // 1. If shutdown connection actively:
    //      DisConnected -> Connected -> HalfClosed -> DisConnected
    // 2. If shutdown connection passively:
    //      DisConnected -> Connected -> DisConnected

public:

    using span_type = TcpBuffer::span_type;         // equals to std::span<const uint8_t>
    using buffer_type = TcpBuffer::buffer_type;     // equals to std::vector<uint8_t>

    static TcpConnectionPtr createTcpConnection(net::SocketPtr&& socket, net::EventLoop* loop);

    // User interface.
    void setMessageCallback(const TcpMessageCallback& cb) noexcept { mMessageCb = cb; }

    // User interface.
    void setConnectionCallback(const TcpConnectionCallback& cb) noexcept { mConnectionCb = cb; }

    // User interface.
    void setWriteCompleteCallback(const TcpWriteCompleteCallback& cb) noexcept { mWriteCompleteCb = cb; }

    // User interface.
    void setHighWaterMarkCallback(const TcpHighWaterMarkCallback& cb) noexcept { mHighWaterMarkCb = cb; }

    // Internal interface.
    // Close callback is used by TcpServer/TcpClient to notify them erase TcpConnection from collection.
    void setCloseCallback(TcpCloseCallback&& cb) noexcept { mCloseCb = std::move(cb); }

    /**
     * @brief send : User interface. Send message to server.
     *               Thread-safety.
     *
     * @param data:
     */
    void send(span_type data);

    void send(buffer_type&& data);

    void sendString(std::string_view message);

    void sendString(std::string&& message);

    /**
     * @brief read : Return a string which read from TcpBuffer, but not extract data from TcpBuffer.
     *               The size of result string may less then input size, please check it!
     *               Thread-safety
     * @param size: the size of data user would read.
     *
     * @return result string.
     */
    span_type read(size_t size) noexcept EXCLUDES(mRecvMutex);

    /**
     * @brief readAll : Return a string which read from TcpBuffer, but not extract data from TcpBuffer.
     *                  The size of result string may less then input size, please check it!
     *                  Thread-safety
     * @return result string.
     */
    span_type readAll() noexcept EXCLUDES(mRecvMutex);

    /**
     * @brief extract : Return a string which read from TcpBuffer, and extract data from TcpBuffer.
     *                  The size of result string may less then input size, please check it!
     *                  Thread-safety
     * @param size: the size of data user would extract.
     *
     * @return result string.
     */
    buffer_type extract(size_t size) noexcept EXCLUDES(mRecvMutex);

    /**
     * @brief extract : Return a string which read from TcpBuffer, and extract data from TcpBuffer.
     *                  The size of result string may less then input size, please check it!
     *                  Thread-safety
     * @param size: the size of data user would extract.
     *
     * @return result string.
     */
    buffer_type extractAll() noexcept EXCLUDES(mRecvMutex);

    // Read function return string.
    std::string_view    readString(size_t size) noexcept EXCLUDES(mRecvMutex);

    std::string_view    readStringAll() noexcept EXCLUDES(mRecvMutex);

    std::string         extractString(size_t size) noexcept EXCLUDES(mRecvMutex);

    std::string         extractStringAll() noexcept EXCLUDES(mRecvMutex);

    /**
     * @brief getBufferSize : Get the size of bytes store in mRecvBuffer.
     *
     * @return
     */
    auto getBufferSize() noexcept EXCLUDES(mRecvMutex) {
        std::lock_guard lock { mRecvMutex };
        return mRecvBuffer.size();
    }

    /**
     * @brief establishConnect :Internal interface.
     *                          Call by TcpServer/TcpClient to make connection readable.
     */
    void establishConnect();

    /**
     * @brief destroyConnection : Internal interface.
     *                            Call by TcpServer/TcpClient to destroy connection when the peer has been shutdown
     *                            , and then release the Channel and Socket.
     */
    void destroyConnection() noexcept;

    /**
     * @brief shutdownConnection : Internal interface.
     *                             Call by TcpServer/TcpClient to shutdown write port of socket
     *                             , but not release Channel and Socket.
     */
    void shutdownConnection() noexcept;

    ~TcpConnection() noexcept;

    [[nodiscard]]
    inline bool isConnected() const noexcept { return mState == ConnState::Connected; }

    [[nodiscard]]
    inline bool isDisconnected() const noexcept { return mState == ConnState::DisConnected; }

    inline void setState(ConnState state) noexcept { mState = state; }

    inline net::SocketAddr getLocalAddr() const noexcept { return mLocalAddr; }

    inline net::SocketAddr getPeerAddr() const noexcept { return mPeerAddr; }

    void dumpSocketInfo() const noexcept { mpSocket->dumpSocketInfo(); }

    /**
     * @brief getLoop : Get owner loop for TcpConnection.
     *
     * @return
     */
    [[nodiscard]]
    net::EventLoop* getLoop() const noexcept { return mpEventLoop; }

private:
    net::EventLoop*             mpEventLoop;
    net::SocketPtr              mpSocket;
    net::ChannelPtr             mpChannel;
    net::SocketAddr             mLocalAddr;
    net::SocketAddr             mPeerAddr;
    std::string                 mIdentification;

    TcpCloseCallback            mCloseCb;
    TcpConnectionCallback       mConnectionCb;
    TcpMessageCallback          mMessageCb;
    TcpWriteCompleteCallback    mWriteCompleteCb;
    TcpHighWaterMarkCallback    mHighWaterMarkCb;

    ConnState                   mState;

    std::mutex                  mRecvMutex;
    TcpBuffer                   mRecvBuffer GUARDED_BY(mRecvMutex);
    TcpBuffer                   mSendBuffer;

    TcpConnection(net::SocketPtr&& socket, net::EventLoop* loop);

    void handleRead() EXCLUDES(mRecvMutex);

    void handleWrite() EXCLUDES(mRecvMutex);

    void handleError() EXCLUDES(mRecvMutex);

    void handleClose() EXCLUDES(mRecvMutex);

    void sendInLoop(span_type data);
};


} // namespace net::tcp
