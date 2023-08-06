#include "base/Log.h"
#include "base/Jthread.h"
#include "tcp/TcpClient.h"
#include <chrono>
#include <iostream>
#include <string>
#include <string_view>

static constexpr std::string_view TAG = "EchoClient";

using namespace simpletcp;
using namespace simpletcp::net;
using namespace simpletcp::tcp;
using namespace std;


const SocketAddr serverAddr {
    .mIpAddr = "127.0.0.1",
    .mIpProtocol = IP_PROTOCOL::IPv4,
    .mPort = 8848
};

TcpConnectionPtr clientConn;

static void onMessage(const TcpConnectionPtr& conn) {
    auto message = conn->extractAll();
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
            clientConn = nullptr;
            std::cout << "[EchoClient] Connection [" << client.getId() << "] is destroy!" << std::endl;
        }
    });
    client.setMessageCallback(onMessage);
    client.connect();
    LOG_INFO("start server");

    auto inputThread = simpletcp::utils::Jthread([&] {
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
