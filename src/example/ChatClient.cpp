#include "base/Log.h"
#include "net/EventLoop.h"
#include "net/Socket.h"
#include "tcp/TcpClient.h"
#include "tcp/TcpConnection.h"
#include <chrono>
#include <iostream>
#include <string_view>
#include <thread>

#ifdef TAG
#undef TAG
#endif
static constexpr std::string_view TAG = "ChatClient";

using namespace utils;
using namespace std;
using namespace net;
using namespace net::tcp;

const SocketAddr serverAddr {
    .mIpAddr = "127.0.0.0",
    .mIpProtocol = IP_PROTOCOL::IPv4,
    .mPort = 8848
};

TcpConnectionPtr clientConn;

static void onConnection(const TcpConnectionPtr& conn) {
    if (conn->isConnected()) {
        LOG_INFO("Connection success!");
        clientConn = conn;
    } else {
        LOG_INFO("Disconnected!");
        clientConn = nullptr;
    }
}

int main() try {
    // create loop
    EventLoop loop;
    TcpClient client(&loop, serverAddr);
    client.setConnectionCallback(onConnection);
    client.connect();
    LOG_INFO("start server");

    auto inputThread = std::jthread([&] {
        std::string input;
        while (std::cin >> input) {
            auto guard = clientConn;
            if (guard != nullptr) {
                guard->send(std::move(input));
                std::cout << "input: " << input << std::endl;
            }
        }
    });

    loop.startLoop();
} catch (std::exception &e) {
    LOG_ERR("{}: {}", __FUNCTION__, e.what());
    throw;
}
