#pragma once
#include <string_view>

namespace simpletcp::http {

enum class HttpRequestType {
    GET,
    POST,
    PUT,
    DELETE,
    HEAD,
    UNKNOWN,
};

enum class HttpVersion {
    HTTP1_0,
    HTTP1_1,
    HTTP3_0,
    UNKNOWN_VER,
};

enum class HttpStatusCode {
    OK = 200,
    MOVED_PERMANENTLY = 301,
    BAD_REQUEST = 400,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    INTERNAL_SERVER_ERROR = 500,
    METHOD_NOT_IMPLEMENTED = 501,
};

inline constexpr uint32_t to_unsigned(HttpStatusCode code) {
    return static_cast<uint32_t>(code);
}

inline constexpr std::string_view to_string_view(HttpStatusCode code) {
    switch (code) {
        case HttpStatusCode::BAD_REQUEST: return "Bad Request";
        case HttpStatusCode::MOVED_PERMANENTLY: return "Moved Permanently";
        case HttpStatusCode::NOT_FOUND: return "Not Found";
        case HttpStatusCode::OK: return "OK";
        case HttpStatusCode::INTERNAL_SERVER_ERROR: return "Internal Server Error";
        case HttpStatusCode::METHOD_NOT_IMPLEMENTED: return "Method Not Implemented";
        default: return "UNKNOWN";
    }
}

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

inline constexpr std::string_view to_string_view(HttpVersion ver) {
    switch (ver) {
        case HttpVersion::HTTP1_0: return "HTTP/1.0";
        case HttpVersion::HTTP1_1: return "HTTP/1.1";
        default: return "UNKNOWN_VER";
    }
}

} // namespace simpletcp::http

