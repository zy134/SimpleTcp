#include "base/Log.h"
#include "base/Jthread.h"
#include "tcp/TcpClient.h"
#include <chrono>
#include <iostream>
#include <string>
#include <string_view>
#include "ChatCommon.h"

static constexpr std::string_view TAG = "ChatClient";

using namespace simpletcp;
using namespace simpletcp::net;
using namespace simpletcp::tcp;
using namespace std;


TcpConnectionPtr clientConn;

static void onMessage(const TcpConnectionPtr& conn) {
    TRACE();
    auto message = conn->extractAll();
    std::cout << message << std::endl;
}

int main(int argc, char** argv) try {
    if (argc < 2) {
        std::cerr << "Invalid parameters" << std::endl;
        return 0;
    }
    std::string clientName = argv[1];

    // create loop
    EventLoop loop;
    TcpClient client(&loop, chatServerAddr);
    client.setConnectionCallback([&] (const TcpConnectionPtr& conn) {
        if (conn->isConnected()) {
            std::cout << "[EchoClient] Connection [" << client.getId() << "] is build!" << std::endl;
            clientConn = conn;
            auto msg = registerRequest(clientName);
            conn->send(msg);
        } else {
            clientConn = nullptr;
            std::cout << "[EchoClient] Connection [" << client.getId() << "] is destroy!" << std::endl;
        }
    });

    client.setMessageCallback(onMessage);
    client.connect();
    LOG_INFO("start client");

    auto inputThread = simpletcp::jthread([&] {
        std::string input;
        while (std::getline(std::cin, input)) {
            auto guard = clientConn;
            if (guard != nullptr) {
                LOG_INFO("{}: input: {}", __FUNCTION__, input);
                auto msg = normalRequest(input);
                guard->send(msg);
            }
        }
    });

    loop.startLoop();
} catch (const std::exception &e) {
    LOG_ERR("{}: {}", __FUNCTION__, e.what());
    throw;
}
