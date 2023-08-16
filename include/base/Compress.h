#pragma once

#include <string_view>
#include <string>

// Compress wrapper of deflate, gzip, br
namespace simpletcp::utils {

std::string deflate(std::string_view source);

std::string undeflate(std::string_view source, size_t source_len);

} // namespace simpletcp::utils
