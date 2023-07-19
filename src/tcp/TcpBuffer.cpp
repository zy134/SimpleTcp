#include "tcp/TcpBuffer.h"
#include "base/Utils.h"
#include "base/Log.h"
#include "base/Error.h"
#include <algorithm>
#include <iostream>

extern "C" {
#include <unistd.h>
#include <fcntl.h>
}

#ifdef TAG
#undef TAG
#endif
static constexpr std::string_view TAG = "TcpBuffer";

using namespace utils;

namespace net::tcp {

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
    if (res == writablebytes()) {
        // Don't write again. Make write availble in next loop.
        mWritePos += res;
        mBuffer.resize(mBuffer.size() * 2);
    } else if (res > 0) {
        mWritePos += res;
    } else {
        throw NetworkException("[TcpBuffer] write error.", res);
    }
    LOG_DEBUG("%s: read bytes: %d, readablebytes %d, writablebytes %d", __FUNCTION__
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
        LOG_DEBUG("%s: write done, write bytes: %d", __FUNCTION__, res);
        // Don't write again. Make write availble in next loop.
        updateReadPos(res);
    } else {
        throw NetworkException("[TcpBuffer] write error.", res);
    }
}

void TcpBuffer::appendToBuffer(std::string_view message) {
    LOG_DEBUG("%s: start, message size:%zu, current buffer size:%zu", __FUNCTION__, message.size(), mBuffer.size());
    while (writablebytes() < message.size()) {
        mBuffer.resize(mBuffer.size() * 2);
    }
    std::copy(message.begin(), message.end(), getWritePos());
    mWritePos += message.size();
    LOG_DEBUG("%s: end, message size:%zu, current buffer size:%zu", __FUNCTION__, message.size(), mBuffer.size());
}

std::string TcpBuffer::read(size_t size) noexcept {
    std::string result;
    if (size < readablebytes()) {
        result.resize(size);
        std::copy(getReadPos(), getReadPos() + size, result.begin());
    } else {
        result.resize(readablebytes());
        std::copy(getReadPos(), getReadPos() + readablebytes(), result.begin());
    }
    return result;
}

std::string TcpBuffer::readAll() noexcept {
    std::string result;
    result.resize(readablebytes());
    std::copy(getReadPos(), getReadPos() + readablebytes(), result.begin());
    return result;
}

std::string TcpBuffer::extract(size_t size) noexcept {
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


void TcpBuffer::updateReadPos(size_t len) noexcept {
    assertTrue((mReadPos + len) <= mWritePos, "[TcpBuffer] the fomula (mReadPos + len <= mWritePos) dosn't hold!");
    mReadPos += len;
    if (mReadPos > readablebytes()) {
        std::copy(getReadPos(), getWritePos(), mBuffer.data());
        mWritePos = readablebytes();
        mReadPos = 0;
    }
}

} // namespace net::tcp
