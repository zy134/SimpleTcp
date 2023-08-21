#include "base/Log.h"
#include "net/EventLoop.h"
#include "tcp/TcpServer.h"
#include <exception>
#include <iostream>

static constexpr std::string_view TAG = "EchoServer";

using namespace simpletcp;
using namespace simpletcp::net;
using namespace simpletcp::tcp;
using namespace std;
using namespace std::chrono_literals;

const SocketAddr serverAddr {
    .mIpAddr = "127.0.0.1",
    .mIpProtocol = IP_PROTOCOL::IPv4,
    .mPort = 8000
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
    auto message = conn->extractStringAll();
    std::cerr << "read from client [" << message << "]" << std::endl;
    conn->sendString(message);
    conn->dumpSocketInfo();
}

int main() try {
    TcpServerArgs initArgs {
        .loop = &loop,
        .serverAddr = serverAddr,
        .maxListenQueue = MAX_LISTEN_QUEUE,
        .maxThreadNum = 0
    };
    TcpServer server(initArgs);
    server.setConnectionCallback(onConnection);
    server.setMessageCallback(onMessage);
    server.start();
    LOG_INFO("start server");
    loop.startLoop();
} catch (std::exception &e) {
    LOG_ERR("{}: {}", __FUNCTION__, e.what());
    throw;
}
