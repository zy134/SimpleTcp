#include "base/Log.h"
#include "net/EventLoop.h"
#include "tcp/TcpConnection.h"
#include "tcp/TcpServer.h"
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_set>
#include "ChatCommon.h"

static constexpr std::string_view TAG = "ChatServer";

using namespace simpletcp;
using namespace simpletcp::net;
using namespace simpletcp::tcp;
using namespace std;


constexpr auto MAX_LISTEN_QUEUE = 100;

class ChatServer {
public:
    ChatServer(EventLoop* loop): mServ( { loop, chatServerAddr, MAX_LISTEN_QUEUE, 0 } ) {
        mServ.setConnectionCallback([this] (const TcpConnectionPtr& conn) {
            onConnection(conn);
        });
        mServ.setMessageCallback([this] (const TcpConnectionPtr& conn) {
            onMessage(conn);
        });
    }

    void start() {
        LOG_INFO("{}: {}", __FUNCTION__, mServ.getId());
        mServ.start();
    }

    ~ChatServer() = default;

private:
    void onConnection(const TcpConnectionPtr& conn) {
        TRACE();
        if (conn->isConnected()) {
            LOG_INFO("{}: new conn is establish!", __FUNCTION__);
            mClients.insert({ conn, ""});
        } else {
            LOG_INFO("{} conn is destroied.", __FUNCTION__);
            mClients.erase(conn);
        }
    }
    void onMessage(const TcpConnectionPtr& conn) {
        TRACE();
        LOG_DEBUG("{}: current buffer size: {}", __FUNCTION__, conn->getBufferSize());
        if (conn->getBufferSize() >= sizeof(RequestHdr)) {
            // Decode request.
            RequestHdr requestHdr {};
            auto hdrBuf = conn->read(sizeof(requestHdr));
            assertTrue(sizeof(requestHdr) == hdrBuf.size(), "[ChatServer] Bad request header!");
            std::memcpy(reinterpret_cast<char *>(&requestHdr), hdrBuf.data(), sizeof(RequestHdr));
            LOG_DEBUG("{}: request length {}, type {}", __FUNCTION__
                    , static_cast<uint64_t>(requestHdr.mReqLength), static_cast<uint64_t>(requestHdr.mReqType));
            if (conn->getBufferSize() >= requestHdr.mReqLength) {
                auto request = conn->extractString(requestHdr.mReqLength);
                assertTrue(requestHdr.mReqLength == request.size(), "[ChatServer] Bad request!");
                std::string_view requestData { request.data() + sizeof(requestHdr), request.size() - sizeof(requestHdr) };
                switch (requestHdr.mReqType) {
                    case RequestType::Message:
                        LOG_INFO("{}: Message request, message: {}", __FUNCTION__, requestData);
                        for (const auto& client : mClients) {
                            const auto& clientName = mClients.at(conn);
                            auto message = simpletcp::format("[{}] {}", clientName, requestData);
                            client.first->sendString(message);
                        }
                        return;
                    case RequestType::Register:
                        LOG_INFO("{}: Register request, new client :{}", __FUNCTION__, requestData);
                        assertTrue(mClients.count(conn) != 0, "[ChatServer] bad Connection!");
                        mClients.at(conn) = std::string(requestData.data());
                        return;
                    default:
                        LOG_ERR("{}: Bad request type!", __FUNCTION__);
                }
            }
        }
    }

    TcpServer mServ;
    std::unordered_map<TcpConnectionPtr, std::string> mClients;
};

EventLoop loop;

int main() try {
    ChatServer serv {&loop};
    serv.start();
    loop.startLoop();
} catch (std::exception &e) {
    LOG_ERR("{}: {}", __FUNCTION__, e.what());
    throw;
}
