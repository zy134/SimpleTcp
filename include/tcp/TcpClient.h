#pragma once

#include "base/Utils.h"
#include "net/Channel.h"
#include "net/Socket.h"
#include "tcp/TcpConnection.h"
#include "net/EventLoop.h"
#include <chrono>
#include <ratio>

namespace net::tcp {

class TcpClient {
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

    net::EventLoop*     mpEventLoop;
    SocketAddr          mServerAddr;
    ClientState         mState; // State can only change in loop thread.

    TcpConnectionPtr    mConnection;

    // Temporary variant use for connecting to remote.
    SocketPtr           mConnSocket;
    ChannelPtr          mConnChannel;
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

};

} // namespace net::tcp
