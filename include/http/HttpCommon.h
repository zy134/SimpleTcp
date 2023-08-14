#pragma once
#include <string_view>

namespace simpletcp::http {

enum class HttpRequestType {
    UNKNOWN,
    GET,
    POST,
    PUT,
    DELETE,
};

enum class HttpVersion {
    UNKNOWN,
    HTTP1_0,
    HTTP1_1,
    HTTP3_0,
};

enum class HttpStatusCode {
    UNKNOWN,
    OK = 200,
    MOVED_PERMANENTLY = 301,
    BAD_REQUEST = 400,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    INTERNAL_SERVER_ERROR = 500,
    METHOD_NOT_IMPLEMENTED = 501,
};

enum class HttpContentType {
    UNKNOWN,
    PLAIN,
    HTML,
    JPEG,
    PNG,
    GIF,
    JAVASCRIPT,
    XML,
    JSON,
    OTHER,
};

enum class CharSet {
    UNKNOWN,
    UTF_8,
    UTF_16,
    UTF_32,
};

inline constexpr uint32_t to_unsigned(HttpStatusCode code) {
    return static_cast<uint32_t>(code);
}

inline constexpr std::string_view to_string_view(HttpStatusCode code) {
    switch (code) {
        case HttpStatusCode::BAD_REQUEST: return "400 Bad Request";
        case HttpStatusCode::MOVED_PERMANENTLY: return "301 Moved Permanently";
        case HttpStatusCode::NOT_FOUND: return "404 Not Found";
        case HttpStatusCode::OK: return "200 OK";
        case HttpStatusCode::INTERNAL_SERVER_ERROR: return "500 Internal Server Error";
        case HttpStatusCode::METHOD_NOT_IMPLEMENTED: return "501 Method Not Implemented";
        default: return "UNKNOWN";
    }
}

inline constexpr std::string_view to_string_view(HttpRequestType type) {
    switch (type) {
        case HttpRequestType::GET: return "GET";
        case HttpRequestType::POST: return "POST";
        case HttpRequestType::PUT: return "PUT";
        case HttpRequestType::DELETE: return "DELETE";
        default: return "UNKNOWN";
    }
}

inline constexpr std::string_view to_string_view(HttpVersion ver) {
    switch (ver) {
        case HttpVersion::HTTP1_0: return "HTTP/1.0";
        case HttpVersion::HTTP1_1: return "HTTP/1.1";
        default: return "UNKNOWN";
    }
}

inline constexpr std::string_view to_string_view(HttpContentType type) {
    switch (type) {
        case HttpContentType::HTML: return "text/html";
        case HttpContentType::JPEG: return "image/jpeg";
        case HttpContentType::PNG: return "image/png";
        case HttpContentType::GIF: return "image/gif";
        case HttpContentType::XML: return "application/xml";
        case HttpContentType::JSON: return "application/json";
        default: return "UNKNOWN";
    }
}

inline constexpr std::string_view to_string_view(CharSet type) {
    switch (type) {
        case CharSet::UTF_8: return "charset=UTF-8";
        case CharSet::UTF_16: return "charset=UTF-16";
        case CharSet::UTF_32: return "charset=UTF-32";
        default: return "UNKNOWN";
    }
}

} // namespace simpletcp::http

