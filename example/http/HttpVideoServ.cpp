#include "base/Log.h"
#include "http/HttpCommon.h"
#include "http/HttpError.h"
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"
#include "net/EventLoop.h"
#include "tcp/TcpConnection.h"
#include "tcp/TcpServer.h"
#include "http/HttpServer.h"
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <string_view>
#include <string>
#include <tuple>

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
        mFileSize = std::filesystem::file_size(path);

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
            std::cout << "[HttpVideoServ] client acquire for " << path << std::endl;
            response.setContentByFilePath(path);
        } else if (request.mUrl == "video.mp4") {
            path.append(request.mUrl);
            std::cout << "[HttpVideoServ] client acquire for " << path << std::endl;

            // Select transfer range
            RangeType range;
            if (request.mRanges.empty()) {
                throw ResponseError {"[HttpVideoServ] bad range arguments.", ResponseErrorType::BadResponse};
            }

            const auto& received_range = request.mRanges.front();
            range.start = received_range.first;
            range.end = (received_range.second > 0) ?
                    received_range.second : static_cast<int64_t>(mChunkSize) + range.start;
            if (range.end > static_cast<int64_t>(mFileSize)) {
                range.end = static_cast<int64_t>(mFileSize);
            }
            range.total = static_cast<int64_t>(mFileSize);

            // Read file of range.
            mFile.seekg(range.start);
            mFile.read(mChunkBuffer.data(), static_cast<std::streamsize>(range.end - range.start));
            std::string_view data { mChunkBuffer.data(), static_cast<size_t>(range.end - range.start) };
            if (range.end < static_cast<int64_t>(mFileSize)) {
                response.setStatus(StatusCode::PARTIAL_CONTENT);
            } else {
                response.setStatus(StatusCode::OK);
            }
            for (auto&& p : request.mRanges) {
                std::cout << "[HttpVideoServ] range [" << p.first << ", " << p.second << "]" << std::endl;
            }
            response.setBody(data);
            response.setContentLength(static_cast<size_t>(range.end - range.start));
            response.setContentType(ContentType::MP4);
            response.setContentRange(range);
            // dumpHttpRequest(request);
        }
    }

    void start() {
        mServ.start();
    }

private:
    HttpServer      mServ;
    std::fstream    mFile;
    size_t          mFileSize;
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
