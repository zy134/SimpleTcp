#include "tcp/TcpBuffer.h"
#include "base/Utils.h"
#include "base/Log.h"
#include "base/Error.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string_view>
#include <vector>

extern "C" {
#include <unistd.h>
#include <fcntl.h>
}

static constexpr std::string_view TAG = "TcpBuffer";

using namespace simpletcp;
using namespace simpletcp::net;

namespace simpletcp::tcp {

TcpBuffer::TcpBuffer() :mReadPos(0), mWritePos(0) {
    mBuffer.resize(1024);
}

/*
 *  |-------------------|------------------||----------------------------|
 *  |-------------------|------------------||-------writablebytes--------|
 *  |-------------------|------------------||----------------------------|
 *  ^                   ^                   ^             ^              ^
 *  |                   |                   |             |              |
 *  0               mReadPos           mWritePos          |             last
 *                                                        |
 *                                                        |
 *                                          read data form socket to mWritePos
 *
 *
 */
// TODO:
// Use readv to speed up this invocation.
void TcpBuffer::readFromSocket(const SocketPtr &socket) {
    auto res = ::read(socket->getFd(), getWritePos(), writablebytes());
    if (res > 0 && static_cast<size_type>(res) == writablebytes()) {
        // Don't write again. Make write availble in next loop.
        mWritePos += static_cast<size_type>(res);
        mBuffer.resize(mBuffer.size() * 2);
    } else if (res > 0) {
        mWritePos += static_cast<size_type>(res);
    } else {
        throw NetworkException("[TcpBuffer] write error.", socket->getSocketError());
    }
    LOG_DEBUG("{}: read bytes: {}, readablebytes {}, writablebytes {}", __FUNCTION__
            , res, readablebytes(), writablebytes());
}

/*
 *  |-------------------|-------------------------------||-------------------|
 *  |-------------------|--------readablebytes----------||---writablebytes---|
 *  |-------------------|-------------------------------||-------------------|
 *  ^                   ^        |                       ^                   ^
 *  |                   |        |                       |                   |
 *  0               mReadPos     |                      mWritePos           last
 *                      |--------|
 *                       write to|
 *                       socket  |
 *  |----------------------------|----------------------||-------------------|
 *  |----------------------------|---readablebytes------||---writablebytes---|
 *  |----------------------------|----------------------||-------------------|
 *  ^                            ^                       ^                   ^
 *  |                            |                       |                   |
 *  0                         mReadPos                   mWritePos           last
 *
 *
 */
void TcpBuffer::writeToSocket(const SocketPtr &socket) {
    auto res = ::write(socket->getFd(), getReadPos(), readablebytes());
    if (res > 0) {
        LOG_DEBUG("{}: write done, write bytes: {}", __FUNCTION__, res);
        // Don't write again. Make write availble in next loop.
        updateReadPos(static_cast<size_type>(res));
    } else {
        throw NetworkException("[TcpBuffer] write error.", socket->getSocketError());
    }
}

void TcpBuffer::appendToBuffer(span_type data) {
    LOG_DEBUG("{}: start, message size:{}, current buffer size:{}", __FUNCTION__, data.size(), mBuffer.size());
    while (writablebytes() < data.size()) {
        mBuffer.resize(data.size() * 2);
    }
    ::memcpy(getWritePos(), data.data(), data.size());
    mWritePos += data.size();
    LOG_DEBUG("{}: end, message size:{}, current buffer size:{}", __FUNCTION__, data.size(), mBuffer.size());
}


TcpBuffer::span_type TcpBuffer::read(size_type size) noexcept {
    span_type result;
    if (size < readablebytes()) {
        result = std::span { getReadPos(), size };
    } else {
        result = std::span { getReadPos(), readablebytes() };
    }
    return result;
}

std::string_view TcpBuffer::readString(size_type size) noexcept {
    if (size < readablebytes()) {
        return std::string_view { reinterpret_cast<char *>(getReadPos()), size };
    } else {
        return std::string_view { reinterpret_cast<char *>(getReadPos()), readablebytes() };
    }
}

std::u8string_view TcpBuffer::readU8String(size_type size) noexcept {
    if (size < readablebytes()) {
        return std::u8string_view { reinterpret_cast<char8_t *>(getReadPos()), size };
    } else {
        return std::u8string_view { reinterpret_cast<char8_t *>(getReadPos()), readablebytes() };
    }
}

std::vector<TcpBuffer::char_type> TcpBuffer::extract(size_type size) noexcept {
    buffer_type result;
    if (size < readablebytes()) {
        result.resize(size);
        ::memcpy(result.data(), getReadPos(), size);
    } else {
        result.resize(readablebytes());
        ::memcpy(result.data(), getReadPos(), readablebytes());
    }
    updateReadPos(result.size());
    return result;
}

std::string TcpBuffer::extractString(size_type size) noexcept {
    std::string result;
    if (size < readablebytes()) {
        result.resize(size);
        ::memcpy(result.data(), getReadPos(), size);
    } else {
        result.resize(readablebytes());
        ::memcpy(result.data(), getReadPos(), readablebytes());
    }
    updateReadPos(result.size());
    return result;
}

std::u8string TcpBuffer::extractU8String(size_type size) noexcept {
    std::u8string result;
    if (size < readablebytes()) {
        result.resize(size);
        ::memcpy(result.data(), getReadPos(), size);
    } else {
        result.resize(readablebytes());
        ::memcpy(result.data(), getReadPos(), readablebytes());
    }
    updateReadPos(result.size());
    return result;
}

void TcpBuffer::updateReadPos(size_type len) noexcept {
    assertTrue((mReadPos + len) <= mWritePos, "[TcpBuffer] the fomula (mReadPos + len <= mWritePos) dosn't hold!");
    mReadPos += len;
    if (mReadPos > readablebytes()) {
        ::memcpy(mBuffer.data(), getReadPos(), readablebytes());
        mWritePos = readablebytes();
        mReadPos = 0;
    }
}

} // namespace net::tcp
