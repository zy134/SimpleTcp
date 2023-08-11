#pragma once

#include <type_traits>
#include <ctime>
#if defined(__cpp_lib_format)
#include <format>
#elif __has_include(<fmt/format.h>)
// Using header-only libfmt
#define FMT_HEADER_ONLY
#include <fmt/format.h>
#include <fmt/core.h>
#else
#error "Not support std::format or fmt::format!"
#endif

namespace simpletcp {

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

template <StringType S, Formatable... Args>
inline std::string format(S&& format_str, Args&&... args) {
#ifdef __cpp_lib_format
    return std::format(std::forward<S>(format_str), std::forward<Args>(args)...);
#else
    return fmt::format(std::forward<S>(format_str), std::forward<Args>(args)...);
#endif
}


template <typename Container, StringType S, Formatable... Args>
auto format_to(std::back_insert_iterator<Container> out, S&& format_str, Args&&... args)
    -> std::back_insert_iterator<Container> {
#ifdef __cpp_lib_format
    return std::format_to(out, std::forward<S>(format_str), std::forward<Args>(args)...);
#else
    return fmt::format_to(out, std::forward<S>(format_str), std::forward<Args>(args)...);
#endif
}

}
