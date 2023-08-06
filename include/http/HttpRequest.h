#pragma once
#include <cstddef>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <vector>
#include <unordered_map>

namespace simpletcp::http {

enum class HttpRequestType {
    GET,
    POST,
    PUT,
    DELETE,
    HEAD,
    UNKNOWN,
};

enum class HttpRequestVersion {
    HTTP1_0,
    HTTP1_1,
    HTTP3_0,
    UNKNOWN_VER,
};

inline constexpr std::string_view to_string_view(HttpRequestType type) {
    switch (type) {
        case HttpRequestType::GET: return "GET";
        case HttpRequestType::POST: return "POST";
        case HttpRequestType::PUT: return "PUT";
        case HttpRequestType::DELETE: return "DELETE";
        case HttpRequestType::HEAD: return "HEAD";
        default: return "UNKNOWN";
    }
}

inline constexpr std::string_view to_string_view(HttpRequestVersion ver) {
    switch (ver) {
        case HttpRequestVersion::HTTP1_0: return "HTTP1_0";
        case HttpRequestVersion::HTTP1_1: return "HTTP1_1";
        case HttpRequestVersion::HTTP3_0: return "HTTP3_0";
        default: return "UNKNOWN_VER";
    }
}

struct HttpRequest {
    // Parsed request.
    std::string                 mUrl;
    std::string                 mQueryParameters;
    HttpRequestType             mType;
    HttpRequestVersion          mVersion;
    std::unordered_map<std::string, std::string>
                                mHeaders;
    bool                        mIsKeepAlive;
    std::string                 mBody;
    // Raw request data.
    std::string                 mRawRequest;
    size_t                      mRequestSize;
};

HttpRequest parseHttpRequest(std::string_view rawHttpPacket);

void dumpHttpRequest(const HttpRequest& request);

} // namespace simpletcp::http

