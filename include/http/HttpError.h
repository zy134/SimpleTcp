#pragma once

#include <exception>
#include <stdexcept>

namespace simpletcp::http {

enum class HttpErrorType {
    BadRequest,
    BadResponse,
};

class HttpException : public std::runtime_error {
public:
    HttpException(const std::string& msg, HttpErrorType type) : std::runtime_error(msg), mErrorType(type) {}

    [[nodiscard]]
    auto getErrorType() const noexcept { return mErrorType; }
private:
    HttpErrorType mErrorType;
};

} // namespace simpletcp::http
