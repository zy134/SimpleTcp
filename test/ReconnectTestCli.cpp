#include "base/Log.h"
#include "base/Jthread.h"
#include "net/EventLoop.h"
#include "tcp/TcpClient.h"
#include "tcp/TcpConnection.h"
#include <exception>
#include <string_view>
#include <iostream>
#include <thread>

using namespace simpletcp;
using namespace simpletcp::net;
using namespace simpletcp::tcp;
using namespace std::chrono_literals;

static constexpr std::string_view TAG = "ReconnectClient";

const SocketAddr serverAddr {
    .mIpAddr = "127.0.0.1",
    .mIpProtocol = IP_PROTOCOL::IPv4,
    .mPort = 8848
};

static int conn_num = 0;

void onConnection(const TcpConnectionPtr& conn) {
    if (conn->isConnected()) {
        ++conn_num;
        LOG_INFO("{}: Connected!", __FUNCTION__);
    } else {
        LOG_INFO("{}: Disconnected!", __FUNCTION__);
    }
}

int main() try {
    std::vector<simpletcp::utils::Jthread> threads;
    for (int i = 0; i != 100; ++i) {
        threads.emplace_back([&] {
            EventLoop loop;
            TcpClient client { &loop, serverAddr };
            client.connect();
            client.setConnectionCallback(onConnection);
            loop.runAfter([&] {
                loop.quitLoop();
            }, i * 100ms);
            loop.startLoop();
            client.disconnect();
        });
    }
    threads.clear();
    std::cerr << "[ReconnectClient] total conn_num " << conn_num << std::endl;
} catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    LOG_ERR("{}", e.what());
    return 1;
}
