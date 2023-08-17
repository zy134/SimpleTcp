#include "base/Log.h"
#include "http/HttpCommon.h"
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"
#include "net/EventLoop.h"
#include "tcp/TcpConnection.h"
#include "tcp/TcpServer.h"
#include "http/HttpServer.h"
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>
#include <string>

using namespace simpletcp;
using namespace simpletcp::net;
using namespace simpletcp::tcp;
using namespace simpletcp::http;
using namespace std::string_literals;

inline static const SocketAddr serverAddr {
    .mIpAddr = "127.0.0.1",
    .mIpProtocol = IP_PROTOCOL::IPv4,
    .mPort = 8848
};

inline static constexpr auto MAX_LISTEN_QUEUE = 1000;

inline static constexpr std::string_view TAG = "HttpServ";

void onConnection(const TcpConnectionPtr& conn) {
    TRACE();
    if (conn->isConnected()) {
        std::cout << "Connected" << std::endl;
    } else {
        std::cout << "Disconnected" << std::endl;
    }
}

void handleRequest(const HttpRequest& request, HttpResponse& response) {
    response.setVersion(Version::HTTP1_1);
    response.setStatus(StatusCode::OK);
    response.setDate();
    response.setKeepAlive(true);
    std::filesystem::path path = HTTP_RESOURCE_PATH;
    if (request.mUrl.empty()) {
        path.append("index.html");
        std::cout << "[HttpServ] client acquire for " << path << std::endl;
        response.setContentByFilePath(path);
    } else if (request.mUrl == "img.jpg") {
        path.append(request.mUrl);
        std::cout << "[HttpServ] client acquire for " << path << std::endl;
        response.setContentByFilePath(path);
    }
}

HttpServerArgs initArgs {
    .serverAddr = serverAddr,
    .maxListenQueue = MAX_LISTEN_QUEUE,
    .maxThreadNum = 0,
};

int main() try {
    HttpServer server(std::move(initArgs));
    server.setConnectionCallback(onConnection);
    server.setRequestHandle(handleRequest);
    server.start();
} catch (...) {
    LOG_ERR("{}", __FUNCTION__);
    throw;
}
