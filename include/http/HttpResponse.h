#pragma once
#include "base/Utils.h"
#include "http/HttpCommon.h"
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_set>
#include <vector>
#include <unordered_map>
#include <filesystem>

namespace simpletcp::http {

// Used for Content-Range property.
struct RangeType {
    int64_t start;
    int64_t end;
    int64_t total;
};

class HttpResponse final {
    friend class HttpServer;
public:
    HttpResponse(Version version, std::vector<EncodingType> encodings)
        : mStatus(StatusCode::UNKNOWN)
        , mVersion(version)
        , mContentType(ContentType::UNKNOWN)
        , mContentLength(0)
        , mContentRange { .start = 0, .end = 0, .total = 0 }
        , mIsKeepAlive(false)
        , mCharSet(CharSet::UNKNOWN)
        , mAvailEncodings(std::move(encodings))
    {}
    // Enable move, disable copy.
    HttpResponse(HttpResponse&&) = default;
    DISABLE_COPY(HttpResponse);
    
    void setStatus(StatusCode status) noexcept { mStatus = status; }
    void setVersion(Version version) noexcept { mVersion = version; }
    void setKeepAlive(bool enable) noexcept { mIsKeepAlive = enable; }
    void setCharSet(CharSet charSet) { mCharSet = charSet; }
    void setDate();
    void setContentByFilePath(const std::filesystem::path& path);
    void setBody(std::string&& body) noexcept { mBody = std::move(body); }
    void setBody(std::string_view body) noexcept { mBody = std::string { body.data(), body.size() }; }
    void setContentType(ContentType contentType) noexcept { mContentType = contentType; }
    void setContentLength(size_t contentLength) { mContentLength = contentLength; }
    void setContentRange(auto contentRange) { mContentRange = contentRange; }
    void setProperty(std::string_view key, std::string_view value);
    auto getProperty(std::string_view key) -> std::optional<std::string_view>;

private:
    EncodingType selectEncodeType(const std::filesystem::path& filePath) const;
    void dump() const;
    // Internal method. Invoked by HttpServer.
    std::string generateResponse();

    StatusCode                  mStatus;
    Version                     mVersion;
    ContentType                 mContentType;
    size_t                      mContentLength;
    RangeType                   mContentRange;
    bool                        mIsKeepAlive;
    CharSet                     mCharSet;
    std::vector<EncodingType>
                                mAvailEncodings;
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
