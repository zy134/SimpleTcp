#pragma once

#if defined(__cpp_lib_format)
#include <format>
#elif __has_include(<fmt/format.h>)
#define FMT_HEADER_ONLY
#include <fmt/format.h>
#include <fmt/core.h>
#else
#error "Not support std::format or fmt::format!"
#endif

namespace simpletcp {

template <typename S, typename... Args>
inline std::string format(S&& format_str, Args&&... args) {
#ifdef __cpp_lib_format
    return std::format(std::forward<S>(format_str), std::forward<Args>(args)...);
#else
    return fmt::format(std::forward<S>(format_str), std::forward<Args>(args)...);
#endif
}

}
