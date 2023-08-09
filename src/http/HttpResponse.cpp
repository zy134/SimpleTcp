#include "http/HttpResponse.h"
#include "base/Log.h"
#include <chrono>
#include <ctime>
#include <exception>
#include <filesystem>
#include <fstream>
#include <ios>
#include <utility>

using namespace std;
using namespace std::filesystem;
using namespace std::chrono;

inline static constexpr std::string_view TAG = "HttpResponse";

[[maybe_unused]] 
inline static constexpr std::string_view CRLF = "\r\n";

namespace simpletcp::http {

void HttpResponse::setKeepAlive(bool enable) {
    mIsKeepAlive = enable;
}

void HttpResponse::setStatus(HttpStatusCode status) {
    mStatus = status;
}

void HttpResponse::setVersion(HttpVersion version) {
    mVersion = version;
}

void HttpResponse::setBody(std::string &&body) {
    mBody = std::move(body);
}

void HttpResponse::addHeader(std::string_view key, std::string_view value) {
    if (mHeaders.count(key.data()) != 0) {
        LOG_INFO("{}: key:{}, old value({})->new value({})", __FUNCTION__, key, mHeaders.at(key.data()), value);
        mHeaders.at(key.data()) = value.data();
    } else {
        LOG_INFO("{}: key:{} value:{}", __FUNCTION__, key, value);
        mHeaders.insert({ key.data(), value.data() });
    }
}

void HttpResponse::setHttpContextFile(const std::filesystem::path& httpFilePath) {
    TRACE();
    try {
        auto file = std::fstream { httpFilePath, std::ios::in };
        auto fileSize = filesystem::file_size(httpFilePath);
        std::string body;
        body.resize(fileSize);
        file.read(body.data(), static_cast<streamsize>(fileSize));
        mBody = std::move(body);
        LOG_INFO("{}: http file: {}, size {} bytes", __FUNCTION__, httpFilePath.c_str(), fileSize);
    } catch (const std::exception& e) {
        LOG_ERR("{}: exception happen ! {}", __FUNCTION__, e.what());
        printBacktrace();
        mStatus = HttpStatusCode::NOT_FOUND;
        mVersion = HttpVersion::HTTP1_1;
        mStatusLine = "HTTP/1.1 404 Not Found";
        mHeaders = {
            { "Content-Type", "text/html" },
        };
        mBody = 
            "<HTML><TITLE>Not Found</TITLE>\r\n"\
            "<BODY><P>The server could not fulfill\r\n"\
            "your request because the resource specified\r\n"\
            "is unavailable or nonexistent.\r\n"\
            "</BODY></HTML>\r\n";
    }
}

} // namespace simpletcp::http
