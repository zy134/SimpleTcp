#include "tcp/TcpClient.h"
#include "base/Error.h"
#include "net/EventLoop.h"
#include "net/Channel.h"
#include "base/Utils.h"
#include "base/Log.h"
#include "net/Socket.h"
#include "tcp/TcpConnection.h"

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <fmt/format.h>
#include <mutex>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <chrono>
#include <random>

extern "C" {
#include <unistd.h>
}

using namespace simpletcp;
using namespace simpletcp::net;
using namespace std;
using namespace std::chrono;
using namespace chrono_literals;

static constexpr std::string_view UNKNOWN_CLIENT_ID = "UNKNOWN_CLIENT_ID";
static constexpr std::string_view TAG = "TcpClient";

namespace simpletcp::tcp {

TcpClient::TcpClient(EventLoop* loop, SocketAddr serverAddr)
    : mpEventLoop(loop), mServerAddr(std::move(serverAddr))
      , mState(ClientState::Disconnect), mConnRetryDelay(TCP_INIT_RETRY_TIMEOUT), mIdentification(UNKNOWN_CLIENT_ID)
{
    LOG_INFO("{}: E", __FUNCTION__);
    mpEventLoop->assertInLoopThread();

    // Every client has a random delay prefix.
    std::default_random_engine e(
        static_cast<unsigned long>(chrono::system_clock::now().time_since_epoch().count())
    );
    std::uniform_int_distribution<uint32_t> u { 0, 500 };
    mConnRandomDelay = chrono::milliseconds{ u(e) };

    LOG_INFO("{}: X", __FUNCTION__);
}

// The life time of TcpClient must be longer then connection!
TcpClient::~TcpClient() {
    LOG_INFO("{}: E", __FUNCTION__);
    mpEventLoop->assertInLoopThread();
    if (mConnection && !mConnection.unique()) {
        LOG_ERR("{}: Why there has more then one reference to this connection?", __FUNCTION__);
    }
    LOG_INFO("{}: X", __FUNCTION__);
}

void TcpClient::connect() {
    LOG_INFO("{}", __FUNCTION__);
    mpEventLoop->assertInLoopThread();
    transStateInLoop(ClientState::Connecting);
}

void TcpClient::disconnect() {
    LOG_INFO("{}", __FUNCTION__);
    if (!mConnection) {
        LOG_WARN("{}: shutdown in a bad connection!", __FUNCTION__);
        return ;
    }
    mpEventLoop->queueInLoop([this] {
        transStateInLoop(ClientState::ForceDisconnected);
    });
}

void TcpClient::transStateInLoop(ClientState newState) {
    TRACE();
    mpEventLoop->assertInLoopThread();
    auto oldState = mState;
    LOG_INFO("{}: (old->new) ({} -> {})", __FUNCTION__, to_string(oldState), to_string(newState));
    switch (newState) {
        case ClientState::Disconnect:
        // We don't allow transform state from Connecting to Disconnect, if retry Connecting failed it will
        // throw a exception.
            if (oldState != ClientState::Connected) {
                LOG_ERR("{}: Bad state! Connection is invalid.", __FUNCTION__);
                break;
            }
            mState = newState;
            doDisconnected();
            break;
        case ClientState::Connecting:
            if (oldState == ClientState::Connected) {
                LOG_ERR("{}: Bad state! Repeated connect...", __FUNCTION__);
                break;
            }
            if (oldState == ClientState::Connecting) {
                LOG_DEBUG("{}: retry connect, timeout: {}ms", __FUNCTION__, mConnRetryDelay.count());
            }
            mState = newState;
            doConnecting();
            break;
        case ClientState::Connected:
            if (oldState != ClientState::Connecting) {
                LOG_ERR("{}: Bad state! Connected must trans from Connecting...", __FUNCTION__);
                break;
            }
            mState = newState;
            doConnected();
            break;
        case ClientState::ForceDisconnected:
            if (oldState == ClientState::Disconnect || oldState == ClientState::ForceDisconnected) {
                LOG_WARN("{}: Bad state! Connection is already closed.", __FUNCTION__);
                break;
            }
            mState = newState;
            doForceDisconnected();
            break;
    }
}

void TcpClient::doConnecting() {
    LOG_INFO("{}: E", __FUNCTION__);
    mConnChannel = nullptr;
    mConnSocket = nullptr;
    auto serverAddr = mServerAddr;
    // Get non-block socket.
    mConnSocket = Socket::createTcpClientSocket(std::move(serverAddr));
    if (mConnSocket == nullptr) {
        retry();
        LOG_INFO("{}: X", __FUNCTION__);
        return ;
    }
    LOG_INFO("{} : create socket success.", __FUNCTION__);

    // Check the state of socket.
    auto errCode = mConnSocket->getSocketError();
    LOG_INFO("{} : socket state {}", __FUNCTION__, errCode);
    if (errCode == EINPROGRESS || errCode == EINTR || errCode == EISCONN) {
        // Connect is not done, use a channel to monitor the state of socket.
        LOG_INFO("{} : connect in progress.", __FUNCTION__);
        mConnChannel = Channel::createChannel(mConnSocket->getFd(), mpEventLoop);
        mConnChannel->setErrorCallback([this] {
            auto err = mConnSocket->getSocketError();
            LOG_ERR("{} : connect error({}) message({})", __FUNCTION__, err, strerror(err));
            // If connect failed, retry.
            retry();
        });
        mConnChannel->setWriteCallback([this] {
            // TODO:
            // Need check the state of socket.
            LOG_INFO("{} : connect has done.", __FUNCTION__);
            mpEventLoop->queueInLoop([this] {
                transStateInLoop(ClientState::Connected);
            });
        });
        mConnChannel->enableWrite();
    } else if (errCode == 0) {
        // Connect is done, create a new TcpConnection.
        mpEventLoop->queueInLoop([this] {
            transStateInLoop(ClientState::Connected);
        });
    } else {
        // Error happen, retry connect.
        LOG_ERR("{} : connect error({}) message({})", __FUNCTION__, errCode, strerror(errCode));
        retry();
    }
    LOG_INFO("{}: X", __FUNCTION__);
}

void TcpClient::doConnected() {
    LOG_INFO("{}: E", __FUNCTION__);
    assertTrue(mConnSocket != nullptr, "[TcpClient] bad socket pointer!");
    assertTrue(mConnSocket->getSocketError() == 0, "[TcpClient] bad socket!");
    mConnSocket->dumpSocketInfo();

    // This channel is useless now.
    mConnChannel = nullptr;
    // Reset retry delay when connect done.
    mConnRetryDelay = TCP_INIT_RETRY_TIMEOUT;

    // Create new connection.
    mConnection = TcpConnection::createTcpConnection(std::move(mConnSocket), mpEventLoop);
    mConnection->setMessageCallback(mMessageCb);
    mConnection->setWriteCompleteCallback(mWriteCompleteCb);
    mConnection->setConnectionCallback(mConnectionCb);
    mConnection->setHighWaterMarkCallback(mHighWaterMarkCb);
    mConnection->setCloseCallback([this] (const TcpConnectionPtr&) {
        mpEventLoop->queueInLoop([this] {
            transStateInLoop(ClientState::Disconnect);
        });
    });
    // Create new idenfication for TcpClient.
    mIdentification = simpletcp::format("{:016d}_{:05d}_{}_{}"
        , steady_clock::now().time_since_epoch().count()
        , gettid()
        , mConnection->getLocalAddr().mPort
        , mConnection->getLocalAddr().mIpAddr
    );
    LOG_INFO("{}: New Identification {}", __FUNCTION__, mIdentification);

    mConnection->establishConnect();

    LOG_INFO("{}: X", __FUNCTION__);
}

void TcpClient::doDisconnected() {
    LOG_INFO("{} E", __FUNCTION__);
    assertTrue(mConnection != nullptr, "[TcpClient] destroy on a bad connection!");
    // First, delete old Connection.
    mIdentification = UNKNOWN_CLIENT_ID;
    mConnection->destroyConnection();
    mConnection = nullptr;
    // Second, if remote is shutdown, reset delay and retry connect.
    mConnRetryDelay = TCP_INIT_RETRY_TIMEOUT;
    retry();
    LOG_INFO("{} X", __FUNCTION__);
}

void TcpClient::doForceDisconnected() {
    LOG_INFO("{} E", __FUNCTION__);
    mpEventLoop->queueInLoop([this] {
        mConnection->shutdownConnection();
    });
    LOG_INFO("{} X", __FUNCTION__);
}

void TcpClient::retry() {
    mpEventLoop->assertInLoopThread();
    LOG_INFO("{}: curretn delay: {}", __FUNCTION__, mConnRetryDelay.count());
    if (mConnRetryDelay < TCP_MAX_RETRY_TIMEOUT) {
        mpEventLoop->runAfter([this] {
            transStateInLoop(ClientState::Connecting);
        }, mConnRetryDelay + mConnRandomDelay);
        mConnRetryDelay *= 2;
    } else {
        throw NetworkException("[TcpClient] Connect time out!", errno);
    }

}

} // namespace net::tcp
