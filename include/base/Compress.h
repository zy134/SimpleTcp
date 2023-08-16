#pragma once

#include <string_view>
#include <string>

// Compress wrapper of deflate, gzip, br
namespace simpletcp::utils {

std::string compress_deflate(std::string_view source);

std::string uncompress_deflate(std::string_view source, size_t source_len);

std::string compress_gzip(std::string_view source);

std::string uncompress_gzip(std::string_view source, size_t source_len);

} // namespace simpletcp::utils
