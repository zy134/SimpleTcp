#pragma once

#include <iterator>
#include <string_view>
#include <vector>
#include <algorithm>

namespace simpletcp::utils {

inline std::vector<std::string> split(std::string_view str, std::string_view token) {
    std::vector<std::string> result;
    auto start_pos = str.find_first_not_of(token);
    auto end_pos = str.find(token, start_pos);
    while (end_pos != std::string_view::npos) {
        result.emplace_back(str.begin() + start_pos, str.begin() + end_pos);
        start_pos = str.find_first_not_of(token, end_pos);
        end_pos = str.find(token, start_pos);
    }
    return result;
}

inline std::string strip(std::string_view str) {
    [[unlikely]]
    if (str.empty())
        return {};
    auto start_pos = 0ul;
    auto end_pos = str.size();
    while (start_pos < end_pos) {
        if (str[start_pos] != ' ' && str[end_pos - 1] != ' ') {
            break;
        }
        if (str[start_pos] == ' ') { ++start_pos; }
        if (str[end_pos - 1] == ' ') { --end_pos; }
    }
    if (start_pos >= end_pos)
        return {};
    return std::string{ str.begin() + start_pos, end_pos - start_pos };
}

} // namespace simpletcp::utils
