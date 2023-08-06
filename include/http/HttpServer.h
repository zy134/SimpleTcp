#include "net/EventLoop.h"
#include "net/Socket.h"
#include "tcp/TcpConnection.h"
#include "tcp/TcpServer.h"
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"

namespace simpletcp::http {

struct HttpServerArgs {
    net::SocketAddr serverAddr;
    int maxListenQueue;
    int maxThreadNum;
};

class HttpServer {
public:
    using HttpRequestCallback = std::function<void (const HttpRequest&, HttpResponse&)>;
    HttpServer(HttpServerArgs args);
    void setConnectionCallback(tcp::TcpConnectionCallback&& cb) noexcept { mConnectionCb = std::move(cb); }
    void setHttpRequestCallback(HttpRequestCallback&& cb) noexcept { mRequestCb = std::move(cb); }
    void start();
private:
    net::EventLoop              mLoop;
    tcp::TcpServer              mTcpServer;
    tcp::TcpConnectionCallback  mConnectionCb;
    HttpRequestCallback         mRequestCb;

    void onMessage(const tcp::TcpConnectionPtr& conn);
};

} // namespace simpletcp::http
