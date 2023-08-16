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

namespace simpletcp::http {

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
    try {
        auto request = parseHttpRequest(rawHttpPacket);
        request.mRawRequest = conn->extractString(request.mRequestSize);
        if (mRequestHandle) {
            LOG_INFO("{}: send response.", __FUNCTION__);
            HttpResponse response { request.mVersion, request.mAcceptEncodings };
            mRequestHandle(request, response);
            conn->sendString(response.generateResponse());
            response.dump();
            dumpHttpRequest(request);
            if (!response.isKeepAlive()) {
                conn->shutdownConnection();
            }
        } else {
            LOG_INFO("{}: response not found", __FUNCTION__);
            conn->sendString(HTTP_BAD_REQUEST_RESPONSE);
        }
    } catch (const RequestError& e) {
        LOG_ERR("{}: Http request error happen. {}", __FUNCTION__, e.what());
        printBacktrace();
        conn->sendString(HTTP_BAD_REQUEST_RESPONSE);
    } catch (const ResponseError& e) {
        LOG_ERR("{}: Http response error happen. {}", __FUNCTION__, e.what());
        printBacktrace();
        conn->sendString(HTTP_BAD_REQUEST_RESPONSE);
    } catch (const std::exception& e) {
        LOG_ERR("{}: Http error happen. {}", __FUNCTION__, e.what());
        throw;
    }
}

} // namespace simpletcp::http
