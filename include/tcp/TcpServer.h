#pragma once

#include "base/Utils.h"
#include "net/Channel.h"
#include "net/Socket.h"
#include "net/EventFd.h"
#include "net/EventLoop.h"
#include "tcp/TcpConnection.h"

#include <functional>
#include <unordered_set>

namespace net::tcp {

class TcpServer {
public:
    DISABLE_COPY(TcpServer);
    DISABLE_MOVE(TcpServer);

    TcpServer(net::EventLoop* loop, net::SocketAddr serverAddr, int maxListenQueue);
    ~TcpServer() noexcept;

    /**
     * @brief start: Start listen for client.
     */
    void start();

    /**
     * @brief setConnectionCallback: User interface.
     *
     * @param cb: connection callback which would be invoked when a connection is created or removed.
     */
    void setConnectionCallback(TcpConnectionCallback&& cb) noexcept;

    /**
     * @brief setMessageCallback: User interface
     *
     * @param cb: messsage callback which would be invoked when a messsage is received from client.
     */
    void setMessageCallback(TcpMessageCallback&& cb) noexcept;

    /**
     * @brief setWriteCompleteCallback: User interface
     *
     * @param cb: write complete callback which would be invoked after all write operation done.
     */
    void setWriteCompleteCallback(TcpWriteCompleteCallback&& cb) noexcept;

private:
    net::EventLoop*     mpEventLoop;
    net::SocketPtr      mpListenSocket;
    net::ChannelPtr     mpListenChannel;

    std::unordered_set<net::tcp::TcpConnectionPtr> mConnectionSet;
    static constexpr auto MAX_CONNECTION_NUMS = 256;

    TcpConnectionCallback       mConnectionCb;
    TcpMessageCallback          mMessageCb;
    TcpWriteCompleteCallback    mWriteCompleteCb;

    void createNewConnection();
};

} // namespace net::tcp
