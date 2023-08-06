#include "http/HttpServer.h"
#include "base/Error.h"
#include "base/Log.h"
#include "http/HttpError.h"
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"
#include "tcp/TcpConnection.h"
#include "tcp/TcpServer.h"
#include <iostream>
#include <stdexcept>
#include <type_traits>

inline static constexpr std::string_view TAG = "HttpServer";

namespace simpletcp::http {

static constexpr std::string_view CRLF = "\r\n";

const static HttpResponse HTTP_DEFAULT_RESPONSE {
    .mStatus = HttpStatusCode::NOT_FOUND,
    .mStatusLine = "HTTP/1.1 400 Bad Request",
    .mHeaders {},
    .mBody {},
};

void appendResponseToConnection(const HttpResponse& response, const tcp::TcpConnectionPtr& conn) {
    TRACE();
    if (response.mStatusLine.empty()) {
        LOG_WARN("{}: bad response!", __FUNCTION__);
        throw NetworkException {"[HttpServer] bad response", 0};
    }
    std::string buffer;
    buffer.append(response.mStatusLine);
    buffer.append(CRLF);
    if (response.mHeaders.empty()) {
        conn->send(std::move(buffer));
        LOG_DEBUG("{}: response", __FUNCTION__);
        return ;
    }

    for (auto&& header : response.mHeaders) {
        buffer.append(header.first).append(": ").append(header.second).append(CRLF);
    }
    if (response.mBody.empty()) {
        conn->send(std::move(buffer));
        LOG_DEBUG("{}: response", __FUNCTION__);
        return ;
    }

    buffer.append(response.mBody);
    buffer.append(CRLF);
    conn->send(std::move(buffer));
    LOG_DEBUG("{}: response", __FUNCTION__);
    return ;
} 

HttpServer::HttpServer(HttpServerArgs args): mLoop(), mTcpServer({
        .loop = &mLoop,
        .serverAddr = std::move(args.serverAddr),
        .maxListenQueue = args.maxListenQueue,
        .maxThreadNum = args.maxThreadNum
        }) {
    LOG_INFO("{}", __FUNCTION__);
}

void HttpServer::start() {
    LOG_INFO("{}", __FUNCTION__);
    mTcpServer.setConnectionCallback(std::move(mConnectionCb));
    mTcpServer.setMessageCallback([this] (const tcp::TcpConnectionPtr& conn) mutable {
        this->onMessage(conn);
    });
    mTcpServer.start();
    mLoop.startLoop();
}

void HttpServer::onMessage(const tcp::TcpConnectionPtr& conn [[maybe_unused]]) {
    TRACE();
    auto rawHttpPacket = conn->readAll();
    try {
        auto request = parseHttpRequest(rawHttpPacket);
        request.mRawRequest = conn->extract(request.mRequestSize);
        dumpHttpRequest(request);
        appendResponseToConnection(HTTP_DEFAULT_RESPONSE, conn);
        conn->shutdownConnection();
        // HttpResponse response;
        // if (mRequestCb) {
        //     mRequestCb(request, response);
        // } else {}
    } catch (const std::exception& e) {
        LOG_FATAL("{}: Http error happen. {}", __FUNCTION__, e.what());
    }
}

} // namespace simpletcp::http
