#pragma once
#include <string_view>
#include <unordered_map>

namespace simpletcp::http {

// Supported HTTP reqeust method type.
enum class RequestType {
    UNKNOWN,
    GET,
    POST,
    PUT,
    DELETE,
};

inline constexpr auto to_cstr(RequestType type) {
    switch (type) {
        case RequestType::GET: return "GET";
        case RequestType::POST: return "POST";
        case RequestType::PUT: return "PUT";
        case RequestType::DELETE: return "DELETE";
        default: return "UNKNOWN";
    }
}

// Supported HTTP version
enum class Version {
    UNKNOWN,
    HTTP1_0,
    HTTP1_1,
    HTTP3_0,
};

inline constexpr auto to_cstr(Version ver) {
    switch (ver) {
        case Version::HTTP1_0: return "HTTP/1.0";
        case Version::HTTP1_1: return "HTTP/1.1";
        default: return "UNKNOWN";
    }
}

// Supported status code of HTTP response.
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

inline constexpr auto to_unsigned(StatusCode code) {
    return static_cast<unsigned>(code);
}

inline constexpr auto to_cstr(StatusCode code) {
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

// Supported content-type of HTTP response.
enum class ContentType {
    UNKNOWN,
    BINARY,
    PLAIN,
    HTML,
    CSS,
    JAVASCRIPT,
    XML,
    JSON,
    JPEG,
    PNG,
    GIF,
    ICON,
    MP4,
    MPG,
};

inline constexpr auto to_cstr(ContentType type) {
    switch (type) {
        case ContentType::BINARY: return "application/octet-stream";
        case ContentType::PLAIN: return "text/plain";
        case ContentType::HTML: return "text/html";
        case ContentType::CSS: return "text/css";
        case ContentType::JAVASCRIPT: return "application/x-javascript";
        case ContentType::XML: return "application/xml";
        case ContentType::JSON: return "application/json";
        case ContentType::JPEG: return "image/jpeg";
        case ContentType::PNG: return "image/png";
        case ContentType::GIF: return "image/gif";
        case ContentType::ICON: return "application/x-ico";
        case ContentType::MP4: return "video/mpeg4";
        case ContentType::MPG: return "video/mpg";
        default: return "UNKNOWN";
    }
}

const static inline std::unordered_map<std::string_view, ContentType> Str2ContentType {
    { ".html", ContentType::HTML },
    { ".jpg", ContentType::JPEG },
    { ".png", ContentType::PNG },
    { ".txt", ContentType::PLAIN },
    { ".js", ContentType::JAVASCRIPT },
    { ".json", ContentType::JSON },
    { ".xml", ContentType::XML },
    { ".mp4", ContentType::MP4 },
    { ".ico", ContentType::ICON },
    { ".css", ContentType::CSS },
    { ".mpeg", ContentType::MPG },
    { ".mpg", ContentType::MPG },
};

// Supported charset of HTTP response.
enum class CharSet {
    UNKNOWN,
    UTF_8,
    UTF_16,
    UTF_32,
};

inline constexpr auto to_cstr(CharSet type) {
    switch (type) {
        case CharSet::UTF_8: return "charset=UTF-8";
        case CharSet::UTF_16: return "charset=UTF-16";
        case CharSet::UTF_32: return "charset=UTF-32";
        default: return "UNKNOWN";
    }
}

// Supported encoding type of HTTP response.
// TODO: add br support.
enum class EncodingType {
    NO_ENCODING,
    GZIP,
    COMPRESS,
    DEFLATE,
    BR,
};

inline constexpr auto to_cstr(EncodingType type) {
    switch (type) {
        case EncodingType::BR: return "br";
        case EncodingType::GZIP: return "gzip";
        case EncodingType::COMPRESS: return "compress";
        case EncodingType::DEFLATE: return "deflate";
        default: return "No encoding";
    }
}

} // namespace simpletcp::http

