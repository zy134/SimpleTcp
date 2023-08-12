#pragma once
#include "base/LogConfig.h"
#include "base/LogServer.h"
#include "base/Backtrace.h"
#include <utility>

namespace simpletcp::detail {

void printBacktrace();

template <StringType T, Formatable ...Args>
constexpr void log(LogLevel level, std::string_view tag, T&& fmt, Args&& ...args) {
    LogServer::getInstance().format(level, tag, std::forward<T>(fmt), std::forward<Args>(args)...);
    [[unlikely]]
    if (level == LogLevel::Fatal) {
        printBacktrace();
        LogServer::getInstance().forceDestroy();
        std::terminate();
    }
    // For error case, need to flush buffer to log file immediately.
    [[unlikely]]
    if (level == LogLevel::Error) {
        LogServer::getInstance().forceFlush();
    }
}

class ScopeTracer {
public:
    constexpr ScopeTracer(std::string_view tag, std::string_view funcName)
        : mTag(tag), mFuncName(funcName)
    {
        log(LogLevel::Debug, mTag, "{}: E", mFuncName);
    }
    ~ScopeTracer() noexcept {
        log(LogLevel::Debug, mTag, "{}: X", mFuncName);
    }
private:
    std::string_view mTag;
    std::string_view mFuncName;
};

} // namespace detail


namespace simpletcp {

// Check is DEFAULT_LOG_LEVEL a valid value.
static_assert(static_cast<int>(LogLevel::Version) <= DEFAULT_LOG_LEVEL
        , "[Log] DEFAULT_LOG_LEVEL is smaller than LogLevel::Version!");

static_assert(DEFAULT_LOG_LEVEL <= static_cast<int>(LogLevel::Error)
        , "[Log] DEFAULT_LOG_LEVEL is bigger than LogLevel::Error!");

#define LOG_VER(fmt, ...)                                                       \
    if constexpr (static_cast<int>(LogLevel::Version) >= DEFAULT_LOG_LEVEL) {   \
        do {                                                                    \
            detail::log(LogLevel::Version, TAG, fmt, ##__VA_ARGS__);            \
        } while(0);                                                             \
    }

#define LOG_DEBUG(fmt, ...)                                                     \
    if constexpr (static_cast<int>(LogLevel::Debug) >= DEFAULT_LOG_LEVEL) {     \
        do {                                                                    \
            detail::log(LogLevel::Debug, TAG, fmt, ##__VA_ARGS__);              \
        } while(0);                                                             \
    }

#define LOG_INFO(fmt, ...)                                                      \
    if constexpr (static_cast<int>(LogLevel::Info) >= DEFAULT_LOG_LEVEL) {      \
        do {                                                                    \
            detail::log(LogLevel::Info, TAG, fmt, ##__VA_ARGS__);               \
        } while(0);                                                             \
    }


#define LOG_WARN(fmt, ...)                                                      \
    if constexpr (static_cast<int>(LogLevel::Warning) >= DEFAULT_LOG_LEVEL) {   \
        do {                                                                    \
            detail::log(LogLevel::Warning, TAG, fmt, ##__VA_ARGS__);            \
        } while(0);                                                             \
    }

#define LOG_ERR(fmt, ...)                                                       \
    if constexpr (static_cast<int>(LogLevel::Error) >= DEFAULT_LOG_LEVEL) {     \
        do {                                                                    \
            detail::log(LogLevel::Error, TAG, fmt, ##__VA_ARGS__);              \
        } while(0);                                                             \
    }

#define LOG_FATAL(fmt, ...)                                                     \
    if constexpr (static_cast<int>(LogLevel::Fatal) >= DEFAULT_LOG_LEVEL) {     \
        do {                                                                    \
            detail::log(LogLevel::Fatal, TAG, fmt, ##__VA_ARGS__);              \
        } while(0);                                                             \
    }

#define TRACE() detail::ScopeTracer __scoper_tracer__{ TAG, __FUNCTION__ };

using detail::printBacktrace;

#ifdef DEBUG_BUILD
#define assertTrue(cond, msg)                       \
if (!(cond)) {                                      \
    simpletcp::detail::log(simpletcp::LogLevel::Fatal, TAG, "[ASSERT] assert error: {}", msg); \
}
#else
#define assertTrue(cond, msg)                       \
if (!(cond)) {                                      \
    simpletcp::detail::log(simpletcp::LogLevel::Error, TAG, "[ASSERT] assert error: {}", msg); \
}
#endif

}// namespace utils
