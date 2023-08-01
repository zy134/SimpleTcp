#pragma once

#include <type_traits>
#include <ios>
#include <utility>
#if defined (__clang__)
#pragma clang diagnostic ignored "-Wformat-security"
#endif

#if defined (__GNUC__)
#pragma GCC diagnostic ignored "-Wformat-security"
#endif

#include <array>
#include <string_view>
#include <type_traits>
#include <concepts>
#include "base/Format.h"

namespace simpletcp {

enum class LogLevel {
    Version = 0,
    Debug,
    Info,
    Warning,
    Error,
    Fatal,
};

namespace detail {

class LogBuffer;
class LogServer;

// Exception should be deal with in internal module of Log.
void format_log_line(LogLevel level, std::string_view fmt, std::string_view tag) noexcept;

constexpr int TransLogLevelToInt(LogLevel level) {
    return static_cast<int>(level);
}

template <typename T>
concept StringType =
    std::same_as<std::string_view, std::decay_t<T>> ||
    std::same_as<std::string, std::decay_t<T>> ||
    std::same_as<const char *, std::decay_t<T>>
;

template <typename T>
concept Formatable =
    StringType<T> ||
    std::integral<std::decay_t<T>> ||
    std::floating_point<std::decay_t<T>> ||
    std::convertible_to<std::decay_t<T>, void *>;

template <StringType T, Formatable ...Args>
constexpr void log(LogLevel level, std::string_view tag, T&& str, Args&& ...args) {
    auto fmtStr = simpletcp::format(std::forward<T>(str), std::forward<Args>(args)...);
    detail::format_log_line(level, fmtStr, tag);
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


// Default log file path.
#ifndef DEFAULT_LOG_PATH
//#define DEFAULT_LOG_PATH "/home/zy134/test/ChatServer/logs"
#error Undefined DEFAULT_LOG_PATH!
#endif
// Default log line length.
#ifndef LOG_MAX_LINE_SIZE
#define LOG_MAX_LINE_SIZE 512
#endif
// Default log file size.
#ifndef LOG_MAX_FILE_SIZE
#define LOG_MAX_FILE_SIZE (1 << 20)
#endif

#ifndef DEFAULT_LOG_LEVEL
#define DEFAULT_LOG_LEVEL 1
#endif

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

#define TRACE()                                                 \
    detail::ScopeTracer __scoper_tracer__{ TAG, __FUNCTION__ }; \

void assertTrue(bool cond, std::string_view msg);

void printBacktrace();

}// namespace utils
