#pragma once

#include "base/Utils.h"
#include "net/Channel.h"
#include "net/EventLoop.h"
#include "net/Socket.h"
#include "tcp/TcpBuffer.h"
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace net::tcp {

/*
 * TcpConnection
 * Wrapper of connected socket, TcpConnection manage the life cycle of socket by RAII tech.
 *
 *
 * */

class TcpConnection;


// poll Channels => invoke Channel callbacks => invoke Connection callbacks
// All callbacks is run in loop thread.
using TcpConnectionPtr          = std::shared_ptr<TcpConnection>;
using TcpConnectionCallback     = std::function<void (const TcpConnectionPtr&)>;
using TcpMessageCallback        = std::function<void (const TcpConnectionPtr&)>;
using TcpCloseCallback          = std::function<void (const TcpConnectionPtr&)>;
using TcpWriteCompleteCallback  = std::function<void (const TcpConnectionPtr&)>;
using TcpHighWaterMarkCallback  = std::function<void (const TcpConnectionPtr&)>;


class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
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
     * @param message:
     */
    void send(std::string_view message);

    /**
     * @brief read : Return a string which read from TcpBuffer, but not extract data from TcpBuffer.
     *               The size of result string may less then input size, please check it!
     *               Thread-safety
     * @param size: the size of data user would read.
     *
     * @return result string.
     */
    std::string read(size_t size) noexcept;

    /**
     * @brief readAll : Return a string which read from TcpBuffer, but not extract data from TcpBuffer.
     *                  The size of result string may less then input size, please check it!
     *                  Thread-safety
     * @return result string.
     */
    std::string readAll() noexcept;

    /**
     * @brief extract : Return a string which read from TcpBuffer, and extract data from TcpBuffer.
     *                  The size of result string may less then input size, please check it!
     *                  Thread-safety
     * @param size: the size of data user would extract.
     *
     * @return result string.
     */
    std::string extract(size_t size) noexcept;

    /**
     * @brief extract : Return a string which read from TcpBuffer, and extract data from TcpBuffer.
     *                  The size of result string may less then input size, please check it!
     *                  Thread-safety
     * @param size: the size of data user would extract.
     *
     * @return result string.
     */
    std::string extractAll() noexcept;

    /**
     * @brief getBufferSize : Get the size of bytes store in mRecvBuffer.
     *
     * @return
     */
    auto getBufferSize() {
        std::lock_guard lock { mRecvMutex };
        return mRecvBuffer.size();
    }

    /**
     * @brief establishConnect :Internal interface.
     *                          Call by TcpServer/TcpClient to enable connection readable.
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

    inline SocketAddr getLocalAddr() const noexcept { return mLocalAddr; }

    inline SocketAddr getPeerAddr() const noexcept { return mPeerAddr; }

    void dumpSocketInfo() const noexcept { mpSocket->dumpSocketInfo(); }

private:
    net::EventLoop*             mpEventLoop;
    net::SocketPtr              mpSocket;
    net::ChannelPtr             mpChannel;
    net::SocketAddr             mLocalAddr;
    net::SocketAddr             mPeerAddr;

    TcpCloseCallback            mCloseCb;
    TcpConnectionCallback       mConnectionCb;
    TcpMessageCallback          mMessageCb;
    TcpWriteCompleteCallback    mWriteCompleteCb;
    TcpHighWaterMarkCallback    mHighWaterMarkCb;

    ConnState                   mState;

    std::mutex                  mSendMutex;
    std::mutex                  mRecvMutex;
    TcpBuffer                   mRecvBuffer;
    TcpBuffer                   mSendBuffer;

    TcpConnection(net::SocketPtr&& socket, net::EventLoop* loop);

    void handleRead();

    void handleWrite();

    void handleError();

    void handleClose();
};


} // namespace net::tcp
