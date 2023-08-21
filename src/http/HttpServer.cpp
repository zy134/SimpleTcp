#include "http/HttpServer.h"
#include "base/Error.h"
#include "base/Log.h"
#include "base/Format.h"
#include "http/HttpCommon.h"
#include "http/HttpError.h"
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"
#include "tcp/TcpConnection.h"
#include "tcp/TcpServer.h"
#include <iostream>
#include <stdexcept>
#include <type_traits>

inline static constexpr std::string_view TAG = "HttpServer";

// TODO: add SSL
namespace simpletcp::http {

[[maybe_unused]]
static constexpr std::string_view CRLF = "\r\n";

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
    auto rawHttpPacket = conn->readStringAll();
    HttpRequest request {};
    HttpResponse response {};

    // TODO:
    // Need not found page?
    try {
        // Parse HTTP request.
        request = parseHttpRequest(rawHttpPacket);
        request.mRawRequest = conn->extractString(request.mRequestSize);
        // Handle the received request.
        if (mRequestHandle) {
            LOG_INFO("{}: send response.", __FUNCTION__);
            response.setAcceptEncodings(request.mAcceptEncodings);
            mRequestHandle(request, response);
        } else {
            LOG_INFO("{}: response not found", __FUNCTION__);
            response.setStatus(StatusCode::BAD_REQUEST);
            response.setKeepAlive(false);
        }
        conn->sendString(response.generateResponse());
        if (!response.mIsKeepAlive) {
            conn->shutdownConnection();
        }
    } catch (const RequestError& e) {
        if (e.getErrorType() == RequestErrorType::PartialPacket) {
            LOG_INFO("{}: PartialPacket is received, expect for more data...", __FUNCTION__);
        } else {
            dumpHttpRequest(request);
            LOG_ERR("{}: {}", __FUNCTION__, e.what());
            printBacktrace();
            conn->sendString(HTTP_BAD_REQUEST_RESPONSE);
        }
    } catch (const ResponseError& e) {
        // The Server impl of HTTP must catch the exception of HTTP response.
        LOG_ERR("{}: {}", __FUNCTION__, e.what());
        dumpHttpRequest(request);
        response.dump();
        printBacktrace();
        throw;
    }

}

} // namespace simpletcp::http
