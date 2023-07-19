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

#if 0
int main() {
    auto connectFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (connectFd < 0) {
        LOG_ERR("%s: failed to create socket.", __FUNCTION__);
        return 0;
    }

    sockaddr_in serverAddr {};
    serverAddr.sin_port = ::htons(8848);
    serverAddr.sin_family = AF_INET;
    if (auto res = ::inet_pton(AF_INET, "127.0.0.0", &serverAddr.sin_addr); res <= 0) {
        LOG_ERR("%s: error addr, code(%d) message(%d)", __FUNCTION__, errno, strerror(errno));
        return 0;
    }

    if (auto res = ::connect(connectFd, (sockaddr*)&serverAddr, sizeof(serverAddr)); res != 0) {
        LOG_ERR("%s: connect failed, code(%d) message(%d)", __FUNCTION__, errno, strerror(errno));
        return 0;
    }
    LOG_ERR("%s: connect Success!", __FUNCTION__);
    constexpr std::string_view message = "hello world";
    while (true) {
        ::write(connectFd, message.data(), message.size());
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}

#else

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
    LOG_ERR("%s: %s", __FUNCTION__, e.what());
    throw;
}
#endif
