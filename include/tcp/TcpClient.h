#pragma once

#include "base/Utils.h"
#include "net/Channel.h"
#include "net/Socket.h"
#include "tcp/TcpConnection.h"
#include "net/EventLoop.h"
#include <chrono>
#include <ratio>

namespace simpletcp::tcp {

class TcpClient final {
    /*
     * ClientState:
     *
     *                    --------> DISCONNECT
     *                    |             |
     *                    |             |
     *                    |             |
     *                    |             |connect()
     *                    |             |
     *                    |             |
     *                    |             v         disconnect()
     *      handleClose() |         CONNECTING ---------------> FORCEDISCONNECTED
     *                    |             |                             ^
     *                    |             |                             |
     *                    |             |                             |
     *                    |             |handleWrite()                | disconnect()
     *                    |             |                             |
     *                    |             |                             |
     *                    |             |                             |
     *                    |             v                             |
     *                    ----------CONNECTED--------------------------
     *
     *
     * */

    enum class ClientState {
        Disconnect,
        Connecting,
        Connected,
        ForceDisconnected,
    };
    static inline std::string to_string(ClientState state) {
        switch (state) {
            case ClientState::Disconnect: return "Disconnect";
            case ClientState::Connected: return "Connected";
            case ClientState::Connecting: return "Connecting";
            case ClientState::ForceDisconnected: return "ForceDisconnected";
        }
        return "Unknown";
    }
public:
    DISABLE_COPY(TcpClient);
    DISABLE_MOVE(TcpClient);

    // Run in loop thread.
    TcpClient(net::EventLoop* loop, net::SocketAddr serverAddr);

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

    /**
     * @brief setConnectionCallback: User interface.
     *
     * @param cb: connection callback which would be invoked when a connection is created or removed.
     */
    void setConnectionCallback(TcpConnectionCallback&& cb) noexcept { mConnectionCb = std::move(cb); }

    /**
     * @brief setMessageCallback: User interface
     *
     * @param cb: messsage callback which would be invoked when a messsage is received from server.
     */
    void setMessageCallback(TcpMessageCallback&& cb) noexcept { mMessageCb = std::move(cb); }

    /**
     * @brief setWriteCompleteCallback: User interface
     *
     * @param cb: write complete callback which would be invoked after all write operation done.
     */
    void setWriteCompleteCallback(TcpWriteCompleteCallback&& cb) noexcept { mWriteCompleteCb = std::move(cb); }

    /**
     * @brief setHighWaterMarkCallback: User interface
     *
     * @param cb: The callback function would be invoked when
     *            the size of received buffer bigger then TCP_HIGH_WATER_MARK.
     */
    void setHighWaterMarkCallback(TcpHighWaterMarkCallback&& cb) noexcept { mHighWaterMarkCb = std::move(cb); }

    /**
     * @brief getId : Get the idenfication of current server.
     *
     * @return 
     */
    [[nodiscard]]
    std::string_view getId() const noexcept { return mIdentification; }

private:
    // State machine function.
    void transStateInLoop(ClientState newState);

    void doConnecting();

    void doDisconnected();

    void doConnected();

    void doForceDisconnected();

    // Help function for retry connection, it would throw exception when retry timeout.
    void retry();

    net::EventLoop*          mpEventLoop;
    net::SocketAddr          mServerAddr;
    ClientState         mState; // State can only change in loop thread.

    TcpConnectionPtr    mConnection;

    // Temporary variant use for connecting to remote.
    net::SocketPtr           mConnSocket;
    net::ChannelPtr          mConnChannel;
    std::chrono::milliseconds   mConnRetryDelay;
    std::chrono::milliseconds   mConnRandomDelay;
    static constexpr auto TCP_MAX_RETRY_TIMEOUT = std::chrono::minutes(5);
    static constexpr auto TCP_INIT_RETRY_TIMEOUT = std::chrono::milliseconds(100);

    // The idenfication of client port.
    // Id is a string like: [timestamp_tid_port_ip]
    std::string         mIdentification;

    TcpConnectionCallback       mConnectionCb;
    TcpMessageCallback          mMessageCb;
    TcpWriteCompleteCallback    mWriteCompleteCb;
    TcpHighWaterMarkCallback    mHighWaterMarkCb;

};

} // namespace net::tcp
