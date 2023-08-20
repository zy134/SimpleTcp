#include "base/Compress.h"
#include "base/Log.h"
#include <stdexcept>
#define ZLIB_CONST
#include <zlib.h>

using namespace simpletcp;

constexpr auto TAG = "Compress";

// Compress wrapper of deflate, gzip, br
namespace simpletcp::utils {

std::string compress_deflate(std::string_view src_buffer) {
    std::string dst_buffer;
    dst_buffer.resize(src_buffer.size());
    size_t dst_buffer_len = dst_buffer.size();
    compress(reinterpret_cast<unsigned char *>(dst_buffer.data()), &dst_buffer_len
            , reinterpret_cast<const unsigned char*>(src_buffer.data()), src_buffer.size());
    LOG_INFO("{}: src len:{}, dst len:{}\n", __FUNCTION__, src_buffer.size(), dst_buffer_len);
    dst_buffer.resize(dst_buffer_len);
    return dst_buffer;
}

std::string uncompress_deflate(std::string_view src_buffer, size_t dst_buffer_len) {
    std::string dst_buffer;
    dst_buffer.resize(dst_buffer_len);
    uncompress(reinterpret_cast<unsigned char *>(dst_buffer.data()), &dst_buffer_len
            , reinterpret_cast<const unsigned char*>(src_buffer.data()), src_buffer.size());
    LOG_INFO("{}: src len:{}, dst len:{}\n", __FUNCTION__, src_buffer.size(), dst_buffer_len);
    dst_buffer.resize(dst_buffer_len);
    return dst_buffer;

}

std::string compress_gzip(std::string_view src_buffer) {
    std::string dst_buffer;
    dst_buffer.resize(src_buffer.size());
    size_t dst_buffer_len = dst_buffer.size();
    z_stream stream {};
    stream.zalloc = nullptr;
    stream.zfree = nullptr;
    stream.avail_in = static_cast<unsigned>(src_buffer.size());
    stream.avail_out = static_cast<unsigned>(dst_buffer_len);
    stream.next_in = reinterpret_cast<const unsigned char *>(src_buffer.data());
    stream.next_out = reinterpret_cast<unsigned char *>(dst_buffer.data());

    if (deflateInit2(&stream, Z_BEST_COMPRESSION, Z_DEFLATED, 15 | 16 /* windowBits */, MAX_MEM_LEVEL /* memLevel */,
               Z_DEFAULT_STRATEGY) != Z_OK) {
        throw std::runtime_error("[Compress] deflateInit failed");
    }

    if (deflate(&stream, Z_NO_FLUSH) != Z_OK) {
        throw std::runtime_error("[Compress] deflate Z_NO_FLUSH failed");
    }

    if (deflate(&stream, Z_FINISH) != Z_STREAM_END) {
        throw std::runtime_error("[Compress] deflate Z_FINISH failed");
    }

    if (deflateEnd(&stream) != Z_OK) {
        throw std::runtime_error("[Compress] deflateEnd failed");
    }

    dst_buffer_len = dst_buffer_len - stream.avail_out;
    dst_buffer.resize(dst_buffer_len);
    LOG_INFO("{}: input len {}, output len {}", __FUNCTION__, src_buffer.size(), dst_buffer_len);
    return dst_buffer;
}

std::string uncompress_gzip(std::string_view src_buffer, size_t dst_buffer_len) {
    std::string dst_buffer;
    dst_buffer.resize(dst_buffer_len);
    z_stream stream {};
    stream.zalloc = nullptr;
    stream.zfree = nullptr;
    stream.avail_in = static_cast<unsigned>(src_buffer.size());
    stream.avail_out = static_cast<unsigned>(dst_buffer_len);
    stream.next_in = reinterpret_cast<const unsigned char *>(src_buffer.data());
    stream.next_out = reinterpret_cast<unsigned char *>(dst_buffer.data());

    if (inflateInit2(&stream, 15 | 16) != Z_OK) {
        throw std::runtime_error("[Compress] inflateInit failed");
    }

    if (inflate(&stream, Z_NO_FLUSH) != Z_STREAM_END) {
        throw std::runtime_error("[Compress] inflate Z_NO_FLUSH failed");
    }

    if (inflate(&stream, Z_FINISH) != Z_STREAM_END) {
        throw std::runtime_error("[Compress] inflate Z_FINISH failed");
    }

    if (inflateEnd(&stream) != Z_OK) {
        throw std::runtime_error("[Compress] inflateEnd failed");
    }

    dst_buffer_len = dst_buffer_len - stream.avail_out;
    dst_buffer.resize(dst_buffer_len);
    LOG_INFO("{}: input len {}, output len {}", __FUNCTION__, src_buffer.size(), dst_buffer_len);
    return dst_buffer;
}

// TODO:
// Add br.

} // namespace simpletcp::utils
