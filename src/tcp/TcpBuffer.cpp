#include "tcp/TcpBuffer.h"
#include "base/Utils.h"
#include "base/Log.h"
#include "base/Error.h"
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <string_view>

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
void TcpBuffer::readFromSocket(const SocketPtr &socket) {
    auto res = ::read(socket->getFd(), getWritePos(), writablebytes());
    if (res > 0 && (SizeType)res == writablebytes()) {
        // Don't write again. Make write availble in next loop.
        mWritePos += static_cast<SizeType>(res);
        mBuffer.resize(mBuffer.size() * 2);
    } else if (res > 0) {
        mWritePos += static_cast<SizeType>(res);
    } else {
        throw NetworkException("[TcpBuffer] write error.", errno);
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
        updateReadPos(static_cast<SizeType>(res));
    } else {
        throw NetworkException("[TcpBuffer] write error.", errno);
    }
}

void TcpBuffer::appendToBuffer(std::span<char> data) {
    LOG_DEBUG("{}: start, message size:{}, current buffer size:{}", __FUNCTION__, data.size(), mBuffer.size());
    while (writablebytes() < data.size()) {
        mBuffer.resize(data.size() * 2);
    }
    std::copy(data.begin(), data.end(), getWritePos());
    mWritePos += data.size();
    LOG_DEBUG("{}: end, message size:{}, current buffer size:{}", __FUNCTION__, data.size(), mBuffer.size());
}


std::string_view TcpBuffer::read(SizeType size) noexcept {
    std::string_view result;
    if (size < readablebytes()) {
        result = std::string_view { getReadPos(), size };
    } else {
        result = std::string_view { getReadPos(), readablebytes() };
    }
    return result;
}

std::string TcpBuffer::extract(SizeType size) noexcept {
    std::string result;
    if (size < readablebytes()) {
        result.resize(size);
        std::copy(getReadPos(), getReadPos() + size, result.begin());
    } else {
        result.resize(readablebytes());
        std::copy(getReadPos(), getReadPos() + readablebytes(), result.begin());
    }
    updateReadPos(result.size());
    return result;
}


void TcpBuffer::updateReadPos(SizeType len) noexcept {
    assertTrue((mReadPos + len) <= mWritePos, "[TcpBuffer] the fomula (mReadPos + len <= mWritePos) dosn't hold!");
    mReadPos += len;
    if (mReadPos > readablebytes()) {
        std::copy(getReadPos(), getWritePos(), mBuffer.data());
        mWritePos = readablebytes();
        mReadPos = 0;
    }
}

} // namespace net::tcp
