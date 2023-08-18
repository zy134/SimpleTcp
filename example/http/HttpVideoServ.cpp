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
#include <ios>
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
    .mPort = 8000
};

inline static constexpr auto MAX_LISTEN_QUEUE = 1000;

inline static constexpr std::string_view TAG = "HttpVideoServ";

class HttpVideoServ {
public:

    HttpVideoServ(const HttpServerArgs& args, const std::filesystem::path& path)
        : mServ(args), mFile(path, std::ios::in) {
        mChunkSize = 1 << 20; // 1Mb
        mChunkBuffer.resize(mChunkSize);
        mSeekPos = 0;
        mTotalSize = std::filesystem::file_size(path);

        mServ.setConnectionCallback([this] (const auto& conn) {
            this->onConnection(conn);
        });

        mServ.setRequestHandle([this] (const auto& request, auto& response) {
            this->handleRequest(request, response);
        });
    }

    void onConnection(const TcpConnectionPtr& conn) {
        TRACE();
        if (conn->isConnected()) {
            std::cout << "Connected" << std::endl;
        } else {
            std::cout << "Disconnected" << std::endl;
        }
    }
    
    void handleRequest(const HttpRequest& request, HttpResponse& response) {
        TRACE();
        response.setVersion(Version::HTTP1_1);
        response.setStatus(StatusCode::OK);
        response.setDate();
        response.setKeepAlive(true);
        std::filesystem::path path = HTTP_RESOURCE_PATH;
        if (request.mUrl.empty()) {
            path.append("video.html");
            std::cout << "[HttpServ] client acquire for " << path << std::endl;
            response.setContentByFilePath(path);
        } else if (request.mUrl == "video.mp4" && mSeekPos < mTotalSize) {
            path.append(request.mUrl);
            std::cout << "[HttpServ] client acquire for " << path << std::endl;
            size_t size = std::min(mChunkSize, mTotalSize - mSeekPos);
            mFile.read(mChunkBuffer.data(), static_cast<std::streamsize>(size));
            std::string_view data { mChunkBuffer.data(), size };
            if (size == mChunkSize) {
                response.setStatus(StatusCode::PARTIAL_CONTENT);
            } else {
                response.setStatus(StatusCode::OK);
            }
            response.setBody(data);
            response.setContentLength(size);
            response.setContentType(ContentType::MP4);
            response.setContentRange(std::tuple { mSeekPos, mSeekPos + size, mTotalSize });
            mSeekPos += size;
        }
    }

    void start() {
        mServ.start();
    }

private:
    HttpServer      mServ;
    std::fstream    mFile;
    size_t          mTotalSize;
    size_t          mSeekPos;
    size_t          mChunkSize;
    std::vector<char>
                    mChunkBuffer;
};

HttpServerArgs initArgs {
    .serverAddr = serverAddr,
    .maxListenQueue = MAX_LISTEN_QUEUE,
    .maxThreadNum = 0,
};

int main() try {
    std::filesystem::path path = HTTP_RESOURCE_PATH;
    path.append("video.mp4");
    HttpVideoServ serv { initArgs, path };
    serv.start();
} catch (...) {
    LOG_ERR("{}", __FUNCTION__);
    throw;
}
