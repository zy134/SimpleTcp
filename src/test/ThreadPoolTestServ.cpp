#include "base/Log.h"
#include "net/EventLoop.h"
#include "tcp/TcpServer.h"
#include <exception>
#include <iostream>
#include <unistd.h>

static constexpr std::string_view TAG = "ThreadPoolTestServ";

using namespace simpletcp;
using namespace simpletcp::net;
using namespace simpletcp::tcp;
using namespace std;
using namespace std::chrono_literals;

const SocketAddr serverAddr {
    .mIpAddr = "127.0.0.1",
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
}

static void onMessage(const TcpConnectionPtr& conn) {
    auto message = conn->extractAll();
    std::cerr << "read from client [" << message << "]" << std::endl;
    conn->send(message);
    conn->dumpSocketInfo();
}

int main() try {
    TcpServer server(&loop, serverAddr, MAX_LISTEN_QUEUE, 4);
    server.setConnectionCallback(onConnection);
    server.setMessageCallback(onMessage);
    server.start();
    LOG_INFO("start server");
    std::cerr << "[ThreadPoolTestServ] start in process " << getpid() << std::endl;
    loop.startLoop();
} catch (std::exception &e) {
    LOG_ERR("{}: {}", __FUNCTION__, e.what());
    throw;
}
