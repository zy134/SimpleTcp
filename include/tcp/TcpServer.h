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

class TcpServer final {
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

    /**
     * @brief setHighWaterMarkCallback: User interface
     *
     * @param cb: The callback function would be invoked when
     *            the size of received buffer bigger then TCP_HIGH_WATER_MARK.
     */
    void setHighWaterMarkCallback(TcpHighWaterMarkCallback&& cb) noexcept;

    /**
     * @brief getId : Get the idenfication of current server.
     *
     * @return 
     */
    [[nodiscard]]
    std::string_view getId() const noexcept { return mIdentification; }

private:
    net::EventLoop*     mpEventLoop;
    net::SocketPtr      mpListenSocket;
    net::ChannelPtr     mpListenChannel;

    // The idenfication of server port.
    // Id is a string like: [timestamp_tid_port_ip]
    std::string         mIdentification;

    std::unordered_set<net::tcp::TcpConnectionPtr> mConnectionSet;
    static constexpr auto MAX_CONNECTION_NUMS = 256;

    TcpConnectionCallback       mConnectionCb;
    TcpMessageCallback          mMessageCb;
    TcpWriteCompleteCallback    mWriteCompleteCb;
    TcpHighWaterMarkCallback    mHighWaterMarkCb;

    void createNewConnection();
};

} // namespace net::tcp
