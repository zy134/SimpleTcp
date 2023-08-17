#pragma once

#include "http/HttpCommon.h"
#include "http/HttpRequest.h"
#include <stdexcept>

namespace simpletcp::http {

enum class RequestErrorType {
    BadRequest,
    UnsupportMethod,
    PartialPacket,
};

enum class ResponseErrorType {
    BadResponse,
    BadContentType,
    BadContent,
    BadVersion,
    BadStatus,
};

inline constexpr std::string_view to_string_view(RequestErrorType type) {
    switch (type) {
        case RequestErrorType::BadRequest: return "BadRequest";
        case RequestErrorType::UnsupportMethod: return "UnsupportMethod";
        case RequestErrorType::PartialPacket: return "PartialPacket";
    }
}

inline constexpr std::string_view to_string_view(ResponseErrorType type) {
    switch (type) {
        case ResponseErrorType::BadResponse: return "BadResponse";
        case ResponseErrorType::BadContentType: return "BadContentType";
        case ResponseErrorType::BadContent: return "BadContent";
        case ResponseErrorType::BadVersion: return "BadVersion";
        case ResponseErrorType::BadStatus: return "BadStatus";
    }
}

class RequestError : public std::runtime_error {
public:
    RequestError(const std::string& msg, RequestErrorType type) : std::runtime_error(msg), mErrorType(type) {}

    [[nodiscard]]
    auto getErrorType() const noexcept { return mErrorType; }
private:
    RequestErrorType mErrorType;
};

class ResponseError : public std::runtime_error {
public:
    ResponseError(const std::string& msg, ResponseErrorType type) : std::runtime_error(msg), mErrorType(type) {}

    [[nodiscard]]
    auto getErrorType() const noexcept { return mErrorType; }
private:
    ResponseErrorType mErrorType;
};

} // namespace simpletcp::http
