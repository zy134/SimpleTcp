#include "base/Compress.h"
#include "base/Log.h"
#include <zlib.h>

using namespace simpletcp;

constexpr auto TAG = "Compress";

// Compress wrapper of deflate, gzip, br
namespace simpletcp::utils {

std::string deflate(std::string_view src_buffer) {
    std::string dst_buffer;
    dst_buffer.resize(src_buffer.size());
    size_t dst_buffer_len = dst_buffer.size();
    compress(reinterpret_cast<unsigned char *>(dst_buffer.data()), &dst_buffer_len
            , reinterpret_cast<const unsigned char*>(src_buffer.data()), src_buffer.size());
    LOG_INFO("{}: src len:{}, dst len:{}\n", __FUNCTION__, src_buffer.size(), dst_buffer_len);
    dst_buffer.resize(dst_buffer_len);
    return dst_buffer;
}

std::string undeflate(std::string_view src_buffer, size_t src_buffer_len) {
    std::string dst_buffer;
    dst_buffer.resize(src_buffer_len);
    size_t dst_buffer_len = dst_buffer.size();
    uncompress(reinterpret_cast<unsigned char *>(dst_buffer.data()), &dst_buffer_len
            , reinterpret_cast<const unsigned char*>(src_buffer.data()), src_buffer.size());
    LOG_INFO("{}: src len:{}, dst len:{}\n", __FUNCTION__, src_buffer.size(), dst_buffer_len);
    dst_buffer.resize(dst_buffer_len);
    return dst_buffer;

}



} // namespace simpletcp::utils
