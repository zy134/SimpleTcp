#pragma once

namespace simpletcp {

enum class LogLevel {
    Fatal = 0,
    Error = 1,
    Warning,
    Info,
    Debug,
    Version,
};

// Default log file path.
#ifndef DEFAULT_LOG_PATH
//#define DEFAULT_LOG_PATH "/home/zy134/test/ChatServer/logs"
#error "Undefined DEFAULT_LOG_PATH!"
#endif

// Default log line length.
#ifndef LOG_MAX_LINE_SIZE
#define LOG_MAX_LINE_SIZE 512
#endif

// Using style of libfmt(C++ std::format) or c-style snprintf.
#define LOG_STYLE_FMT true

// Default log file size 256Mb
#ifndef LOG_MAX_FILE_SIZE
#define LOG_MAX_FILE_SIZE (1 << 28)
#endif

#ifndef DEFAULT_LOG_LEVEL
#define DEFAULT_LOG_LEVEL 1
#endif

// Check is DEFAULT_LOG_LEVEL a valid value.
static_assert(DEFAULT_LOG_LEVEL <= static_cast<int>(simpletcp::LogLevel::Version)
        , "[Log] DEFAULT_LOG_LEVEL must less equal than LogLevel::Version!");

static_assert(DEFAULT_LOG_LEVEL >= static_cast<int>(simpletcp::LogLevel::Error)
        , "[Log] DEFAULT_LOG_LEVEL must bigger than LogLevel::Error!");


// TODO: equals to multiple of page size
// constexpr unsigned long LOG_BUFFER_SIZE = 1ul << 22; // 4Mb
constexpr unsigned long LOG_BUFFER_SIZE = 1ul << 18; // 256Kb

} // namespace simpletcp
