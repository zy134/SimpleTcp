#include "base/Log.h"
#include "net/EventLoop.h"
#include "net/Socket.h"
#include "tcp/TcpClient.h"
#include "tcp/TcpConnection.h"
#include <arpa/inet.h>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <exception>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

#ifdef TAG
#undef TAG
#endif
static constexpr std::string_view TAG = "EchoClient";

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

static void onMessage(const TcpConnectionPtr& conn, TcpBuffer& buffer) {
    auto message = buffer.extract(buffer.size());
    std::cout << "From server:[" << message << "]" << std::endl;
}

int main() try {
    // create loop
    EventLoop loop;
    TcpClient client(&loop, serverAddr);
    client.setConnectionCallback([&] (const TcpConnectionPtr& conn) {
        if (conn->isConnected()) {
            std::cout << "[EchoClient] Connection [" << client.getId() << "] is build!" << std::endl;
            clientConn = conn;
        } else {
            std::cout << "[EchoClient] Connection [" << client.getId() << "] is destroy!" << std::endl;
        }
    });
    client.setMessageCallback(onMessage);
    client.connect();
    LOG_INFO("start server");

    auto inputThread = std::jthread([&] {
        std::string input;
        while (std::getline(std::cin, input)) {
            auto guard = clientConn;
            if (guard != nullptr) {
                LOG_INFO("{}: input: {}", __FUNCTION__, input);
                std::cout << "To server:[" << input << "]" << std::endl;
                guard->send(input);
            }
        }
    });

    loop.startLoop();
} catch (std::exception &e) {
    LOG_ERR("{}: {}", __FUNCTION__, e.what());
    throw;
}
