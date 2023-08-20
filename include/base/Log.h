#pragma once
#include "base/LogConfig.h"
#include "base/LogServer.h"
#include "base/Backtrace.h"
#include <utility>

// Check is DEFAULT_LOG_LEVEL a valid value.
static_assert(DEFAULT_LOG_LEVEL <= static_cast<int>(simpletcp::LogLevel::Version)
        , "[Log] DEFAULT_LOG_LEVEL must less equal than LogLevel::Version!");

static_assert(DEFAULT_LOG_LEVEL >= static_cast<int>(simpletcp::LogLevel::Error)
        , "[Log] DEFAULT_LOG_LEVEL must bigger than LogLevel::Error!");

namespace simpletcp::detail {

void printBacktrace();

template <StringType T, Formatable ...Args>
constexpr void log_ver(std::string_view tag, T&& fmt, Args&& ...args) {
    if constexpr (DEFAULT_LOG_LEVEL >= static_cast<int>(LogLevel::Version)) {
        LogServer::getInstance().format(LogLevel::Version, tag, std::forward<T>(fmt), std::forward<Args>(args)...);
    }
}

template <StringType T, Formatable ...Args>
constexpr void log_debug(std::string_view tag, T&& fmt, Args&& ...args) {
    if constexpr (DEFAULT_LOG_LEVEL >= static_cast<int>(LogLevel::Debug)) {
        LogServer::getInstance().format(LogLevel::Debug, tag, std::forward<T>(fmt), std::forward<Args>(args)...);
    }
}

template <StringType T, Formatable ...Args>
constexpr void log_info(std::string_view tag, T&& fmt, Args&& ...args) {
    if constexpr (DEFAULT_LOG_LEVEL >= static_cast<int>(LogLevel::Info)) {
        LogServer::getInstance().format(LogLevel::Info, tag, std::forward<T>(fmt), std::forward<Args>(args)...);
    }
}

template <StringType T, Formatable ...Args>
constexpr void log_warn(std::string_view tag, T&& fmt, Args&& ...args) {
    if constexpr (DEFAULT_LOG_LEVEL >= static_cast<int>(LogLevel::Warning)) {
        LogServer::getInstance().format(LogLevel::Warning, tag, std::forward<T>(fmt), std::forward<Args>(args)...);
    }
}

template <StringType T, Formatable ...Args>
constexpr void log_err(std::string_view tag, T&& fmt, Args&& ...args) {
    LogServer::getInstance().format(LogLevel::Error, tag, std::forward<T>(fmt), std::forward<Args>(args)...);
    LogServer::getInstance().forceFlush();
}

template <StringType T, Formatable ...Args>
constexpr void log_fatal(std::string_view tag, T&& fmt, Args&& ...args) {
    LogServer::getInstance().format(LogLevel::Fatal, tag, std::forward<T>(fmt), std::forward<Args>(args)...);
    LogServer::getInstance().forceDestroy();
    std::terminate();
}

class ScopeTracer {
public:
    constexpr ScopeTracer(std::string_view tag, std::string_view funcName)
        : mTag(tag), mFuncName(funcName)
    {
        log_debug(mTag, "{}: E", mFuncName);
    }
    ~ScopeTracer() noexcept {
        log_debug(mTag, "{}: X", mFuncName);
    }
private:
    std::string_view mTag;
    std::string_view mFuncName;
};

} // namespace detail


namespace simpletcp {

#define LOG_VER(fmt, ...)                                                       \
    if constexpr (DEFAULT_LOG_LEVEL >= static_cast<int>(LogLevel::Version)) {   \
        do {                                                                    \
            detail::log_ver(TAG, fmt, ##__VA_ARGS__);                           \
        } while(0);                                                             \
    }

#define LOG_DEBUG(fmt, ...)                                                     \
    if constexpr (DEFAULT_LOG_LEVEL >= static_cast<int>(LogLevel::Debug)) {     \
        do {                                                                    \
            detail::log_debug(TAG, fmt, ##__VA_ARGS__);                         \
        } while(0);                                                             \
    }

#define LOG_INFO(fmt, ...)                                                      \
    if constexpr (DEFAULT_LOG_LEVEL >= static_cast<int>(LogLevel::Info)) {      \
        do {                                                                    \
            detail::log_info(TAG, fmt, ##__VA_ARGS__);                          \
        } while(0);                                                             \
    }


#define LOG_WARN(fmt, ...)                                                      \
    if constexpr (DEFAULT_LOG_LEVEL >= static_cast<int>(LogLevel::Warning)) {   \
        do {                                                                    \
            detail::log_warn(TAG, fmt, ##__VA_ARGS__);                          \
        } while(0);                                                             \
    }

#define LOG_ERR(fmt, ...)                                                       \
        do {                                                                    \
            detail::log_err(TAG, fmt, ##__VA_ARGS__);                           \
        } while(0);                                                             \

#define LOG_FATAL(fmt, ...)                                                     \
        do {                                                                    \
            detail::log_fatal(TAG, fmt, ##__VA_ARGS__);                         \
        } while(0);                                                             \

#define TRACE() detail::ScopeTracer __scoper_tracer__{ TAG, __FUNCTION__ };

using detail::printBacktrace;

#ifdef DEBUG_BUILD
#define assertTrue(cond, msg)                       \
if (!(cond)) {                                      \
    simpletcp::detail::log_fatal(TAG, "[ASSERT] assert error: {}", msg); \
}
#else
#define assertTrue(cond, msg)                       \
if (!(cond)) {                                      \
    simpletcp::detail::log_err(TAG, "[ASSERT] assert error: {}", msg); \
}
#endif

}// namespace utils
