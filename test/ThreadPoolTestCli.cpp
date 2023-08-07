#include "base/Log.h"
#include "base/Jthread.h"
#include "tcp/TcpClient.h"
#include "tcp/TcpConnection.h"
#include <chrono>
#include <iostream>
#include <string>
#include <string_view>
#include <map>
#include <thread>

static constexpr std::string_view TAG = "ThreadPoolTestCli";

using namespace simpletcp;
using namespace simpletcp::net;
using namespace simpletcp::tcp;
using namespace std;


const SocketAddr serverAddr {
    .mIpAddr = "127.0.0.1",
    .mIpProtocol = IP_PROTOCOL::IPv4,
    .mPort = 8848
};

static void onMessage(const TcpConnectionPtr& conn) {
    auto message = conn->extractStringAll();
    std::cout << "From server:[" << message << "]" << std::endl;
}

int main() try {
    // create loop
    std::vector<simpletcp::utils::Jthread> clients;
    std::map<simpletcp::tcp::TcpConnectionPtr, std::string> connections;
    std::mutex mu;

    for (int i = 0; i != 8; ++i) {
        clients.emplace_back([&] {
            EventLoop loop;
            TcpClient client(&loop, serverAddr);
            client.setConnectionCallback([&] (const TcpConnectionPtr& conn) {
                std::lock_guard lock { mu };
                if (conn->isConnected()) {
                    std::cout << "[EchoClient] Connection [" << client.getId() << "] is build!" << std::endl;
                    std::string str = "Client ID:";
                    str.append(client.getId());
                    connections.insert({ conn, str });
                } else {
                    std::cout << "[EchoClient] Connection [" << client.getId() << "] is destroy!" << std::endl;
                    connections.erase(conn);
                }
            });
            client.setMessageCallback(onMessage);
            client.connect();
            LOG_INFO("start server");
            loop.startLoop();
        });
    }

    while (true) {
        std::this_thread::sleep_for(3s);
        std::lock_guard lock { mu };
        for (auto&& conn : connections) {
            conn.first->sendString(conn.second);
        }
    }

} catch (std::exception &e) {
    LOG_ERR("{}: {}", __FUNCTION__, e.what());
    throw;
}
