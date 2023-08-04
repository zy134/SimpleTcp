#pragma once
/*
 *  Error handle headers
 *  throw exception only when:
 *      1. Error case is happened rarely and it can't be handle in current context.
 *      2. Severe error is happen, such as the file descriptor is exhausted.
 *
 *  In other case, you'd better deal with error in current context or use std::option
 *  instead of throw exception.
 *
 * */

#include <cstdint>
extern "C" {
#include <netdb.h>
#include <string.h>
}
#include <exception>
#include <system_error>

namespace simpletcp {

// NetworkException, wrapper of socket error.
class NetworkException : public std::system_error {
public:
    NetworkException(const std::string& errMsg, int errCode)
        : std::system_error(errCode, std::system_category(), errMsg), mErrCode(errCode) {}

    [[nodiscard]]
    int getNetErr() const noexcept { return mErrCode; }
private:
    int mErrCode;
};

// SystemException, wrapper of system call error.
class SystemException : public std::system_error {
public:
    SystemException(const std::string& errMsg)
        : std::system_error(errno, std::generic_category(), errMsg), mErrCode(errno) {}

    [[nodiscard]]
    int getSysErr() const noexcept { return mErrCode; }
private:
    int mErrCode;
};

// If other exception happen, re-throw it and make process abort.

}
