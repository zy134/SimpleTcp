#include "base/Log.h"
#include "http/HttpCommon.h"
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"
#include "net/EventLoop.h"
#include "tcp/TcpConnection.h"
#include "tcp/TcpServer.h"
#include "http/HttpServer.h"
#include <iostream>
#include <string_view>

using namespace simpletcp;
using namespace simpletcp::net;
using namespace simpletcp::tcp;
using namespace simpletcp::http;

inline static const SocketAddr serverAddr {
    .mIpAddr = "127.0.0.1",
    .mIpProtocol = IP_PROTOCOL::IPv4,
    .mPort = 8848
};

inline static constexpr auto MAX_LISTEN_QUEUE = 1000;

inline static constexpr std::string_view TAG = "HttpServ";

void onConnection(const TcpConnectionPtr& conn) {
    if (conn->isConnected()) {
        std::cout << "Connected" << std::endl;
    } else {
        std::cout << "Disconnected" << std::endl;
    }
}

void handleRequest(const HttpRequest& request, HttpResponse& response) {
    response.setVersion(HttpVersion::HTTP1_1);
    response.setStatus(HttpStatusCode::OK);
    response.setDate();
    if (request.mUrl.empty()) {
        response.setKeepAlive(true);
        response.setContentByFilePath("index.html", HttpContentType::HTML);
    } else {
        response.setKeepAlive(false);
        response.setContentByFilePath("img.jpg", HttpContentType::JPEG);
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
    server.setHttpRequestCallback(handleRequest);
    server.start();
} catch (...) {
    LOG_ERR("{}", __FUNCTION__);
    throw;
}
