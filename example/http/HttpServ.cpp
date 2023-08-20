#include "base/Log.h"
#include "http/HttpServer.h"

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

inline static constexpr std::string_view TAG = "HttpServ";
static std::string resource_path = HTTP_RESOURCE_PATH;


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
    std::filesystem::path path = resource_path;
    if (request.mUrl.empty()) {
        path.append("index.html");
        std::cout << "[HttpServ] client acquire for " << path << std::endl;
        response.setContentByFilePath(path);
    } else {
        path.append(request.mUrl);
        std::cout << "[HttpServ] client acquire for " << path << std::endl;
        response.setContentByFilePath(path);
    }
}

inline static const SocketAddr serverAddr {
    .mIpAddr = "127.0.0.1",
    .mIpProtocol = IP_PROTOCOL::IPv4,
    .mPort = 8000
};

HttpServerArgs initArgs {
    .serverAddr = serverAddr,
    .maxListenQueue = 1000,
    .maxThreadNum = 0,
};

int main(int argc, char** argv) try {
    if (argc == 2) {
        resource_path = argv[1];
        std::cout << "[HttpServ] set resource path in: " << resource_path << std::endl;
    }
    HttpServer server(std::move(initArgs));
    server.setConnectionCallback(onConnection);
    server.setRequestHandle(handleRequest);
    server.start();
} catch (...) {
    LOG_ERR("{}", __FUNCTION__);
    throw;
}
