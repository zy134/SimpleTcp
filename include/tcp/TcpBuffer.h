#pragma once

#include "base/Utils.h"
#include "net/Socket.h"
#include <string_view>
#include <string>
#include <vector>

namespace net::tcp {

/*
 *  |-------------------=================================|-------------------|
 *  |-------------------=========readablebytes===========|---writablebytes---|
 *  |-------------------=================================|-------------------|
 *  ^                   ^                                ^                   ^
 *  |                   |                                |                   |
 *  0               mReadPos                            mWritePos           last
 *
 */
class TcpBuffer final {
public:
    DISABLE_COPY(TcpBuffer);
    DISABLE_MOVE(TcpBuffer);
    TcpBuffer();
    ~TcpBuffer() = default;
    using BufferType = std::vector<char>;
    using SizeType = BufferType::size_type;

    // Read data from socket to TcpBuffer.
    // This operation may block.
    // If read error or peer socket is shutdown, this function will throw a NetworkException.
    void readFromSocket(const SocketPtr& socket);

    // Write data from TcpBuffer to socket
    // This operation may block.
    // If write error, this function will throw a NetworkException.
    void writeToSocket(const SocketPtr& socket);

    // Append data to the end of TcpBuffer.
    // This operation is non-block.
    void appendToBuffer(std::string_view message);

    // Return a string which read from TcpBuffer, but not extract data from TcpBuffer.
    // The size of result string may less then input size, please check it!
    std::string read(SizeType size) noexcept;

    // Return a string which read from TcpBuffer, but not extract data from TcpBuffer.
    // The size of result string may less then input size, please check it!
    std::string readAll() noexcept;

    // Return a string which read from TcpBuffer, and extract data from TcpBuffer.
    // The size of result string may less then input size, please check it!
    std::string extract(SizeType size) noexcept;

    // Return counts of bytes stored in buffer.
    [[nodiscard]]
    SizeType size() const noexcept { return readablebytes(); }

    [[nodiscard]]
    bool isReading() const noexcept;

    [[nodiscard]]
    bool isWriting() const noexcept;

private:
    BufferType mBuffer;
    SizeType mReadPos;
    SizeType mWritePos;

    [[nodiscard]]
    char* getWritePos() noexcept { return mBuffer.data() + mWritePos; }

    [[nodiscard]]
    char* getReadPos() noexcept { return mBuffer.data() + mReadPos; }

    [[nodiscard]]
    SizeType writablebytes() const noexcept { return mBuffer.size() - mWritePos; }

    [[nodiscard]]
    SizeType readablebytes() const noexcept { return mWritePos - mReadPos; }

    /*
     * Before shrink:
     *
     *                          readablebytes = 18bytes
     *
     *  |----------------------===================|-------------------|
     *  |----------------------===readablebytes===|---writablebytes---|
     *  |----------------------===================|-------------------|
     *  ^                      ^                  ^                   ^
     *  |                      |                  |                   |
     *  0                  mReadPos              mWritePos           last
     *
     * After shrink:
     *
     *  readablebytes = 18bytes
     *
     *  ===================|------------------------------------------|
     *  ===readablebytes===|---------------writablebytes--------------|
     *  ===================|------------------------------------------|
     *  ^                  ^                                          ^
     *  |                  |                                          |
     * mReadPos          mWritePos                                  last
     */
    void updateReadPos(SizeType len) noexcept;

};

} // namespace net::tcp
