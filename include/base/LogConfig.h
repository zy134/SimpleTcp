#pragma once

namespace simpletcp {

enum class LogLevel {
    Version = 0,
    Debug,
    Info,
    Warning,
    Error,
    Fatal,
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

// Default log file size. default 256Mb
#ifndef LOG_MAX_FILE_SIZE
#define LOG_MAX_FILE_SIZE (1 << 28)
#endif

#ifndef DEFAULT_LOG_LEVEL
#define DEFAULT_LOG_LEVEL 1
#endif

// TODO: equals to page size
constexpr unsigned long LOG_BUFFER_SIZE = 4096ul * 4;

} // namespace simpletcp
