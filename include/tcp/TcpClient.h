#pragma once

#include "base/Utils.h"
#include "net/Channel.h"
#include "net/Socket.h"
#include "tcp/TcpConnection.h"
#include "net/EventLoop.h"

namespace net::tcp {

class TcpClient {
public:
    DISABLE_COPY(TcpClient);
    DISABLE_MOVE(TcpClient);

    // Run in loop thread.
    TcpClient(EventLoop* loop, SocketAddr serverAddr);

    // Run in loop thread.
    ~TcpClient();

    /**
     * @brief connect : Start TcpClient, connect to TcpServer.
     *                  Run in loop thread.
     */
    void connect();

    /**
     * @brief disconnect : Shutdown the connection.
     *                     Run in loop thread.
     */
    void disconnect();

    // User interface.
    void setConnectionCallback(TcpConnectionCallback&& cb) noexcept { mConnectionCb = std::move(cb); }
    
    // User interface.
    void setMessageCallback(TcpMessageCallback&& cb) noexcept { mMessageCb = std::move(cb); }
    
    // User interface.
    void setWriteCompleteCallback(TcpWriteCompleteCallback&& cb) noexcept { mWriteCompleteCb = std::move(cb); }

private:
    void createNewConnection();

    net::EventLoop*     mpEventLoop;
    TcpConnectionPtr    mConnection;

    // Temporary variant use for connecting to remote.
    SocketPtr           mConnSocket;
    ChannelPtr          mConnChannel;

    TcpConnectionCallback       mConnectionCb;
    TcpMessageCallback          mMessageCb;
    TcpWriteCompleteCallback    mWriteCompleteCb;

};

} // namespace net::tcp
