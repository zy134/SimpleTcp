#include "base/Log.h"
#include "net/EventLoop.h"
#include "net/Socket.h"
#include "net/TimerQueue.h"
#include "tcp/TcpBuffer.h"
#include "tcp/TcpConnection.h"
#include "tcp/TcpServer.h"
#include <exception>
#include <iostream>

#ifdef TAG
#undef TAG
#endif
static constexpr std::string_view TAG = "EchoServer";

using namespace utils;
using namespace std;
using namespace net;
using namespace net::tcp;
using namespace std::chrono_literals;

const SocketAddr serverAddr {
    .mIpAddr = "127.0.0.0",
    .mIpProtocol = IP_PROTOCOL::IPv4,
    .mPort = 8848
};

constexpr auto MAX_LISTEN_QUEUE = 100;

EventLoop loop;

TimerId timer = 0;

static void onConnection(const TcpConnectionPtr& conn) {
    if (conn->isConnected()) {
        LOG_INFO("{}: connected!", __FUNCTION__);
    } else {
        LOG_INFO("{}: disconnected!", __FUNCTION__);
    }
    if (conn->isConnected()) {
        timer = loop.runEvery([] {
            LOG_INFO("Test Timer");
        }, 3s);
    } else {
        loop.removeTimer(timer);
    }
}

static void onMessage(const TcpConnectionPtr& conn) {
    auto message = conn->extractAll();
    std::cerr << "read from client [" << message << "]" << std::endl;
    conn->send(message);
    conn->dumpSocketInfo();
}

int main() try {
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
