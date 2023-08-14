#pragma once
#include "base/Utils.h"
#include "http/HttpCommon.h"
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <filesystem>

namespace simpletcp::http {

class HttpResponse {
public:
    HttpResponse()
        : mStatus(HttpStatusCode::UNKNOWN)
        , mVersion(HttpVersion::UNKNOWN)
        , mContentType(HttpContentType::UNKNOWN)
        , mIsKeepAlive(false)
        , mCharSet(CharSet::UNKNOWN)
    {}

    DISABLE_COPY(HttpResponse);

    void setKeepAlive(bool enable) noexcept { mIsKeepAlive = enable; }
    void setStatus(HttpStatusCode status) noexcept { mStatus = status; }
    void setVersion(HttpVersion version) noexcept { mVersion = version; }
    void setCharSet(CharSet charSet) { mCharSet = charSet; }
    void setDate();

    void setContentByFilePath(const std::filesystem::path& path, HttpContentType type);

    void setContent(std::string&& content, HttpContentType type) {
        setBody(std::move(content));
        setContentLenght(content.size());
        setContentType(type);
    }

    bool isKeepAlive() const noexcept { return mIsKeepAlive; }

    void dump() const;

    // Internal method. Invoked by HttpServer.
    std::string generateResponse();
private:
    // Not offer overload function of copy version. Because the body may be so big.
    void setBody(std::string&& body) noexcept { mBody = std::move(body); }
    void setContentType(HttpContentType contentType) noexcept { mContentType = contentType; }
    void setContentLenght(size_t contentLength) { addHeader("Content-Length", std::to_string(contentLength)); }
    void addHeader(std::string_view key, std::string_view value);

    HttpStatusCode              mStatus;
    HttpVersion                 mVersion;
    HttpContentType             mContentType;
    bool                        mIsKeepAlive;
    CharSet                     mCharSet;
    std::unordered_map<std::string, std::string>
                                mHeaders;
    std::string                 mBody;
};

inline constexpr std::string_view HTTP_NOT_FOUND_RESPONSE =
    "HTTP/1.1 404 Not Found\r\n" \
    "Content-Type: text/html\r\n" \
    "<HTML><TITLE>Not Found</TITLE>\r\n"\
    "<BODY><P>The server could not fulfill\r\n"\
    "your request because the resource specified\r\n"\
    "is unavailable or nonexistent.\r\n"\
    "</BODY></HTML>\r\n";

inline constexpr std::string_view HTTP_BAD_REQUEST_RESPONSE =
    "HTTP/1.1 400 Bad Request\r\n" \
    "Content-Type: text/html\r\n" \
    "<P>Your browser sent a bad request, such as a POST without a Content-Length.\r\n";

inline constexpr std::string_view HTTP_UNSUPPORT_METHOD_RESPONSE =
    "HTTP/1.1 501 Method Not Implemented\r\n" \
    "Content-Type: text/html\r\n" \
    "<P>Your browser sent a request with unsupport method.\r\n";

} // namespace simpletcp::http
