#include "base/Log.h"
#include "net/EventLoop.h"
#include "net/Socket.h"
#include "tcp/TcpBuffer.h"
#include "tcp/TcpConnection.h"
#include "tcp/TcpServer.h"
#include <exception>
#include <iostream>
#include <string_view>

#ifdef TAG
#undef TAG
#endif
static constexpr std::string_view TAG = "ChatServer";

using namespace utils;
using namespace std;
using namespace net;
using namespace net::tcp;


const SocketAddr serverAddr {
    .mIpAddr = "127.0.0.0",
    .mIpProtocol = IP_PROTOCOL::IPv4,
    .mPort = 8848
};

constexpr auto MAX_LISTEN_QUEUE = 100;

EventLoop loop;

static void onConnection(const TcpConnectionPtr& conn) {
    if (conn->isConnected()) {
        LOG_INFO("{}: connected!", __FUNCTION__);
    } else {
        LOG_INFO("{}: disconnected!", __FUNCTION__);
    }
}

static void onMessage(const TcpConnectionPtr& conn, TcpBuffer& buffer) {
    auto message = buffer.extract(buffer.size());
    LOG_INFO("{}: read from client [{}]", __FUNCTION__, message);
    std::cout << "read from client:" << message << std::endl;
}

int main() try {
    // create loop
    TcpServer server(&loop, serverAddr, MAX_LISTEN_QUEUE);
    server.setConnectionCallback(onConnection);
    server.setMessageCallback(onMessage);
    server.start();
    LOG_INFO("start server");
    loop.startLoop();
} catch (std::exception &e) {
    LOG_ERR("{}: {}", __FUNCTION__, e.what());
    throw;
}
