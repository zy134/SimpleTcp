#include "base/Log.h"
#include "base/Jthread.h"
#include "net/EventLoop.h"
#include "tcp/TcpClient.h"
#include "tcp/TcpServer.h"
#include <exception>
#include <string_view>
#include <iostream>

using namespace simpletcp;
using namespace simpletcp::net;
using namespace simpletcp::tcp;

static constexpr std::string_view TAG = "ReconnectServer";

const SocketAddr serverAddr {
    .mIpAddr = "127.0.0.1",
    .mIpProtocol = IP_PROTOCOL::IPv4,
    .mPort = 8848
};

void onConnection(const TcpConnectionPtr& conn) {
    if (conn->isConnected()) {
        LOG_INFO("{}: Connected!", __FUNCTION__);
    } else {
        LOG_INFO("{}: Disconnected!", __FUNCTION__);
    }
}

void onMessage(const TcpConnectionPtr& conn) {
    auto message = conn->extractStringAll();
    LOG_INFO("{}: receive from client [{}]", __FUNCTION__, message);
    conn->sendString(std::move(message));
}


int main() try {
    EventLoop loop;
    TcpServer server ({&loop, serverAddr, 100, 0});
    server.setConnectionCallback(onConnection);
    server.setMessageCallback(onMessage);
    server.start();
    loop.startLoop();
} catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
}
