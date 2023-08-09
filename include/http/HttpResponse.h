#pragma once
#include "http/HttpCommon.h"
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <filesystem>

namespace simpletcp::http {

struct HttpResponse {
    void setKeepAlive(bool enable);
    void setStatus(HttpStatusCode status);
    void setVersion(HttpVersion version);
    void setBody(std::string&& body); // Not offer overload function of copy version. Because the body may be so big.
    void addHeader(std::string_view key, std::string_view value);
    void setHttpContextFile(const std::filesystem::path& httpFilePath);

    HttpStatusCode              mStatus;
    HttpVersion                 mVersion;
    bool                        mIsKeepAlive;
    std::string                 mStatusLine;
    std::unordered_map<std::string, std::string>
                                mHeaders;
    std::string                 mBody;

};

inline const HttpResponse HTTP_NOT_FOUND_RESPONSE {
    .mStatus = HttpStatusCode::NOT_FOUND,
    .mVersion = HttpVersion::HTTP1_1,
    .mIsKeepAlive = false,
    .mStatusLine = "HTTP/1.1 404 Not Found",
    .mHeaders {
        { "Content-Type", "text/html" },
    },
    .mBody {
        "<HTML><TITLE>Not Found</TITLE>\r\n"\
        "<BODY><P>The server could not fulfill\r\n"\
        "your request because the resource specified\r\n"\
        "is unavailable or nonexistent.\r\n"\
        "</BODY></HTML>\r\n"
    },
};

inline const HttpResponse HTTP_BAD_REQUEST_RESPONSE {
    .mStatus = HttpStatusCode::BAD_REQUEST,
    .mVersion = HttpVersion::HTTP1_1,
    .mIsKeepAlive = false,
    .mStatusLine = "HTTP/1.1 400 Bad Request",
    .mHeaders {
        { "Content-Type", "text/html" },
    },
    .mBody {
        "<P>Your browser sent a bad request, such as a POST without a Content-Length.\r\n"
    },
};

} // namespace simpletcp::http
