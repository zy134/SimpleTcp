#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>

namespace simpletcp::http {

enum class HttpStatusCode {
    OK = 200,
    MOVED_PERMANENTLY = 301,
    BAD_REQUEST = 400,
    NOT_FOUND = 404,
};

inline constexpr uint32_t to_unsigned(HttpStatusCode code) {
    return static_cast<uint32_t>(code);
}

inline constexpr std::string_view to_string_view(HttpStatusCode code) {
    switch (code) {
        case HttpStatusCode::BAD_REQUEST: return "BAD_REQUEST";
        case HttpStatusCode::MOVED_PERMANENTLY: return "MOVED_PERMANENTLY";
        case HttpStatusCode::NOT_FOUND: return "NOT_FOUND";
        case HttpStatusCode::OK: return "OK";
        default: return "UNKNOWN";
    }
}

struct HttpResponse {
    HttpStatusCode              mStatus;
    std::string                 mStatusLine;
    std::unordered_map<std::string, std::string>
                                mHeaders;
    std::string                 mBody;

};

} // namespace simpletcp::http
