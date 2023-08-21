#include "net/EventLoop.h"
#include "net/Socket.h"
#include "tcp/TcpConnection.h"
#include "tcp/TcpServer.h"
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"
#include "http/HttpError.h"

namespace simpletcp::http {

struct HttpServerArgs {
    net::SocketAddr serverAddr;
    int maxListenQueue;
    int maxThreadNum;
};

class HttpServer final {
public:
    /**
     * @brief : The callback type. This callback function is used to handle HTTP request
     *      and generate appropriate HTTP response. The handle procedure may throw exceptions,
     *      and uses must handle the exception(with type ResponseError) themselives.
     */
    using RequestHandle = std::function<void (const HttpRequest&, HttpResponse&)>;

    HttpServer(HttpServerArgs args);

    /**
     * @brief setConnectionCallback : Set callback function which would be invoked when
     *      remote client is connected or disconnected.
     *
     * @param cb:
     */
    void setConnectionCallback(tcp::TcpConnectionCallback&& cb) noexcept { mConnectionCb = std::move(cb); }

    /**
     * @brief setRequestHandle : Set callback function which would be invoked when remote
     *      HTTP request is arrived.
     *
     * @param cb:
     */
    void setRequestHandle(RequestHandle&& cb) noexcept { mRequestHandle = std::move(cb); }

    /**
     * @brief start : Start loop and server.
     */
    void start();
private:
    net::EventLoop              mLoop;
    tcp::TcpServer              mTcpServer;
    tcp::TcpConnectionCallback  mConnectionCb;
    RequestHandle               mRequestHandle;

    void onMessage(const tcp::TcpConnectionPtr& conn);
};

} // namespace simpletcp::http
