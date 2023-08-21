#pragma once
#include "base/Utils.h"
#include "http/HttpCommon.h"
#include <cstdint>
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
    HttpResponse()
        : mStatus(StatusCode::UNKNOWN)
        , mVersion(Version::UNKNOWN)
        , mContentType(ContentType::UNKNOWN)
        , mContentLength(-1)
        , mContentRange { .start = 0, .end = 0, .total = 0 }
        , mIsKeepAlive(false)
        , mCharSet(CharSet::UNKNOWN)
        , mAvailEncodings()
    {}
    // Enable move, disable copy.
    HttpResponse(HttpResponse&&) = default;
    DISABLE_COPY(HttpResponse);

    /**
     * @brief setStatus : Set status code of HTTP response. This method must be called for every response.
     *
     * @param status:
     *
     * @throw: throw ResponseError if status not set.
     */
    void setStatus(StatusCode status) noexcept { mStatus = status; }

    /**
     * @brief setVersion : Set version of HTTP response. This method must be called for every response.
     *
     * @param version:
     *
     * @throw: throw ResponseError if version not set.
     */
    void setVersion(Version version) noexcept { mVersion = version; }

    /**
     * @brief setKeepAlive : Set should the connection be keep-alive.
     *
     * @param enable: true: keep-alive, false: disconnect
     */
    void setKeepAlive(bool enable) noexcept { mIsKeepAlive = enable; }

    /**
     * @brief setCharSet : Set char set of response.
     *
     * @param charSet:
     */
    void setCharSet(CharSet charSet) { mCharSet = charSet; }

    /**
     * @brief setDate : Add GMT date in the header of HTTP response.
     */
    void setDate();

    /**
     * @brief setContentByFilePath : Set body, type, length of HTTP response by file path, if the path is invalid,
     *      this method would throw a std::runtime_error.
     *
     * @param path: The path of file which would be write to HTTP response.
     *
     * @ throw : std::runtime_error
     */
    void setContentByFilePath(const std::filesystem::path& path);

    /**
     * @brief setBody : Original method of HTTP response, you'd better not use directly.
     *      Set the content of HTTP response.
     *
     * @param body:
     */
    void setBody(std::string&& body) noexcept { mBody = std::move(body); }
    void setBody(std::string_view body) noexcept { mBody = std::string { body.data(), body.size() }; }

    /**
     * @brief setContentType : Original method of HTTP response, you'd better not use directly.
     *      Set content type of HTTP response.
     *
     * @param contentType:
     */
    void setContentType(ContentType contentType) noexcept { mContentType = contentType; }

    /**
     * @brief setContentLength : Original method of HTTP response, you'd better not use directly.
     *      Set content length of HTTP response. If the content range has been set, the invariant
     *      (mContentLength == mContentRange.start - mContentRange.start) must be hold.
     * @param contentLength:
     */
    void setContentLength(int64_t contentLength) { mContentLength = contentLength; }

    /**
     * @brief setContentRange : Set content range of HTTP response.
     *
     * @param contentRange: RangeType {start, end, total}, start means the start position of the byte stream
     *      in content, end means the end position of byte stream int content, total means the total length
     *      of byte stream that Server would send to Client. If the content range has been set, the invariant
     *      (mContentLength == mContentRange.start - mContentRange.start) must be hold.
     */
    void setContentRange(auto contentRange) { mContentRange = contentRange; }

    /**
     * @brief setProperty : Add a new property in the header of HTTP response
     *
     * @param key:
     * @param value:
     */
    void setProperty(std::string_view key, std::string_view value);
    /**
     * @brief getProperty : Get the value of property in HTTP response.
     *
     * @param key:
     *
     * @return
     */
    auto getProperty(std::string_view key) -> std::optional<std::string_view>;

private:
    // Internal method. Invoked by HttpServer.
    EncodingType selectEncodeType(const std::filesystem::path& filePath) const;
    void setAcceptEncodings(const auto& encodings) { mAvailEncodings = encodings; }
    void dump() const;
    void validateResponse();
    std::string generateResponse();

    StatusCode                  mStatus;
    Version                     mVersion;
    ContentType                 mContentType;
    int64_t                     mContentLength;
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
