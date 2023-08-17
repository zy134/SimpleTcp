#pragma once
#include <string_view>

namespace simpletcp::http {

enum class RequestType {
    UNKNOWN,
    GET,
    POST,
    PUT,
    DELETE,
};

enum class Version {
    UNKNOWN,
    HTTP1_0,
    HTTP1_1,
    HTTP3_0,
};

enum class StatusCode {
    UNKNOWN,
    OK = 200,
    PARTIAL_CONTENT = 206,
    MOVED_PERMANENTLY = 301,
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    INTERNAL_SERVER_ERROR = 500,
    METHOD_NOT_IMPLEMENTED = 501,
    HTTP_VERSION_NO_SUPPORT = 505,
};

enum class ContentType {
    UNKNOWN,
    BINARY,
    PLAIN,
    HTML,
    JPEG,
    PNG,
    GIF,
    JAVASCRIPT,
    XML,
    JSON,
    MP4,
    OTHER,
};

enum class CharSet {
    UNKNOWN,
    UTF_8,
    UTF_16,
    UTF_32,
};

enum class EncodingType {
    NO_ENCODING,
    GZIP,
    COMPRESS,
    DEFLATE,
    BR,
};

inline constexpr uint32_t to_unsigned(StatusCode code) {
    return static_cast<uint32_t>(code);
}

inline constexpr std::string_view to_string_view(StatusCode code) {
    switch (code) {
        case StatusCode::OK: return "200 OK";
        case StatusCode::PARTIAL_CONTENT: return "206 Partial Content";
        case StatusCode::MOVED_PERMANENTLY: return "301 Moved Permanently";
        case StatusCode::BAD_REQUEST: return "400 Bad Request";
        case StatusCode::FORBIDDEN: return "403 Forbindden";
        case StatusCode::NOT_FOUND: return "404 Not Found";
        case StatusCode::INTERNAL_SERVER_ERROR: return "500 Internal Server Error";
        case StatusCode::METHOD_NOT_IMPLEMENTED: return "501 Method Not Implemented";
        default: return "UNKNOWN";
    }
}

inline constexpr std::string_view to_string_view(RequestType type) {
    switch (type) {
        case RequestType::GET: return "GET";
        case RequestType::POST: return "POST";
        case RequestType::PUT: return "PUT";
        case RequestType::DELETE: return "DELETE";
        default: return "UNKNOWN";
    }
}

inline constexpr std::string_view to_string_view(Version ver) {
    switch (ver) {
        case Version::HTTP1_0: return "HTTP/1.0";
        case Version::HTTP1_1: return "HTTP/1.1";
        default: return "UNKNOWN";
    }
}

inline constexpr std::string_view to_string_view(ContentType type) {
    switch (type) {
        case ContentType::BINARY: return "application/octet-stream";
        case ContentType::HTML: return "text/html";
        case ContentType::JPEG: return "image/jpeg";
        case ContentType::PNG: return "image/png";
        case ContentType::GIF: return "image/gif";
        case ContentType::XML: return "application/xml";
        case ContentType::JSON: return "application/json";
        case ContentType::MP4: return "video/mpeg4";
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

inline constexpr std::string_view to_string_view(EncodingType type) {
    switch (type) {
        case EncodingType::BR: return "br";
        case EncodingType::GZIP: return "gzip";
        case EncodingType::COMPRESS: return "compress";
        case EncodingType::DEFLATE: return "deflate";
        default: return "No encoding";
    }
}

} // namespace simpletcp::http

