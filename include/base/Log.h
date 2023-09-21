#pragma once
#include <base/LogConfig.h>
#include <base/LogServer.h>
#include <base/Backtrace.h>
#include <utility>
#include <fmt/format.h>

namespace simpletcp {

void printBacktrace();

#define LOG_VER(str, ...)                                                       \
    if constexpr (DEFAULT_LOG_LEVEL >= static_cast<int>(LogLevel::Version)) {   \
        do {                                                                    \
            std::vector<char> __buffer;                                           \
            fmt::format_to(std::back_inserter(__buffer), str, ##__VA_ARGS__);     \
            std::string_view formatted { __buffer.data(), __buffer.size() };        \
            detail::LogServer::getInstance().write(LogLevel::Version            \
                    , formatted, TAG);                                          \
        } while(0);                                                             \
    }

#define LOG_DEBUG(str, ...)                                                     \
    if constexpr (DEFAULT_LOG_LEVEL >= static_cast<int>(LogLevel::Debug)) {     \
        do {                                                                    \
            std::vector<char> __buffer;                                           \
            fmt::format_to(std::back_inserter(__buffer), str, ##__VA_ARGS__);     \
            std::string_view formatted { __buffer.data(), __buffer.size() };        \
            detail::LogServer::getInstance().write(LogLevel::Debug              \
                    , formatted, TAG);                                          \
        } while(0);                                                             \
    }

#define LOG_INFO(str, ...)                                                      \
    if constexpr (DEFAULT_LOG_LEVEL >= static_cast<int>(LogLevel::Info)) {      \
        do {                                                                    \
            std::vector<char> __buffer;                                           \
            fmt::format_to(std::back_inserter(__buffer), str, ##__VA_ARGS__);     \
            std::string_view formatted { __buffer.data(), __buffer.size() };        \
            detail::LogServer::getInstance().write(LogLevel::Info               \
                    , formatted, TAG);                                          \
        } while(0);                                                             \
    }


#define LOG_WARN(str, ...)                                                      \
    if constexpr (DEFAULT_LOG_LEVEL >= static_cast<int>(LogLevel::Warning)) {   \
        do {                                                                    \
            std::vector<char> __buffer;                                           \
            fmt::format_to(std::back_inserter(__buffer), str, ##__VA_ARGS__);     \
            std::string_view formatted { __buffer.data(), __buffer.size() };        \
            detail::LogServer::getInstance().write(LogLevel::Warning            \
                    , formatted, TAG);                                          \
        } while(0);                                                             \
    }

#define LOG_ERR(str, ...)                                                       \
        do {                                                                    \
            std::vector<char> __buffer;                                           \
            fmt::format_to(std::back_inserter(__buffer), str, ##__VA_ARGS__);     \
            std::string_view formatted { __buffer.data(), __buffer.size() };        \
            detail::LogServer::getInstance().write(LogLevel::Error              \
                    , formatted, TAG);                                          \
            detail::LogServer::getInstance().forceFlush();                      \
        } while(0);                                                             \

#define LOG_FATAL(str, ...)                                                     \
        do {                                                                    \
            std::vector<char> __buffer;                                           \
            fmt::format_to(std::back_inserter(__buffer), str, ##__VA_ARGS__);     \
            std::string_view formatted { __buffer.data(), __buffer.size() };        \
            detail::LogServer::getInstance().write(LogLevel::Fatal              \
                    , formatted, TAG);                                          \
            printBacktrace();                                                   \
            detail::LogServer::getInstance().forceDestroy();                    \
            std::terminate();                                                   \
        } while(0);                                                             \

class ScopeTracer {
public:
    ScopeTracer(std::string_view tag, std::string_view funcName)
        : mTag(tag), mFuncName(funcName)
    {
        if constexpr (DEFAULT_LOG_LEVEL >= static_cast<int>(LogLevel::Debug)) {
            auto msg = fmt::format("{}: E", mFuncName);
            detail::LogServer::getInstance().write(LogLevel::Debug, msg, mTag);
        }
    }
    ~ScopeTracer() noexcept {
        if constexpr (DEFAULT_LOG_LEVEL >= static_cast<int>(LogLevel::Debug)) {
            auto msg = fmt::format("{}: X", mFuncName);
            detail::LogServer::getInstance().write(LogLevel::Debug, msg, mTag);
        }
    }
private:
    std::string_view mTag;
    std::string_view mFuncName;
};

#define TRACE() ScopeTracer __scoper_tracer__{ TAG, __FUNCTION__ };

#ifdef DEBUG_BUILD
#define assertTrue(cond, msg)                       \
if (!(cond)) {                                      \
    LOG_FATAL("[ASSERT] assert error!", __FUNCTION__); \
}
#else
#define assertTrue(cond, msg)                       \
if (!(cond)) {                                      \
    LOG_ERR("[ASSERT] assert error!", __FUNCTION__); \
}
#endif

}// namespace utils
