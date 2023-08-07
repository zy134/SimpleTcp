#pragma once

#include "base/Utils.h"
#include "net/Socket.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <span>

namespace simpletcp::tcp {

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

    using char_type         = uint8_t;
    using buffer_type       = std::vector<char_type>;
    using size_type         = buffer_type::size_type;
    using span_type         = std::span<const char_type>;
    static_assert(std::is_same_v<span_type::size_type, buffer_type::size_type>, "[TcpBuffer] What happen?");

    // Read data from socket to TcpBuffer.
    // This operation may block.
    // If read error or peer socket is shutdown, this function will throw a NetworkException.
    void readFromSocket(const net::SocketPtr& socket);

    // Write data from TcpBuffer to socket
    // This operation may block.
    // If write error, this function will throw a NetworkException.
    void writeToSocket(const net::SocketPtr& socket);

    // Append data to the end of TcpBuffer.
    // This operation is non-block.
    void appendToBuffer(span_type data);

    /**
     * @brief read : Read data from current buffer, not modified any data in buffer.
     *
     * @param size: The size would be read. The size of return data may smaller then input size!
     *
     * @return : return a view of data, not copy any data!
     */
    span_type read(size_type size) noexcept;

    std::string_view readString(size_type size) noexcept;

    std::u8string_view readU8String(size_type size) noexcept;

    /**
     * @brief extract : Extract data from current buffer, any then may shrink buffer.
     *
     * @param size: The size would be extract.
     *
     * @return : return a container which is copy from current buffer.
     */
    buffer_type extract(size_type size) noexcept;

    std::string extractString(size_type size) noexcept;

    std::u8string extractU8String(size_type size) noexcept;

    // Return counts of bytes stored in buffer.
    [[nodiscard]]
    size_type size() const noexcept { return readablebytes(); }

    [[nodiscard]]
    bool isReading() const noexcept;

    [[nodiscard]]
    bool isWriting() const noexcept;

private:
    buffer_type mBuffer;
    size_type mReadPos;
    size_type mWritePos;

    [[nodiscard]]
    auto getWritePos() noexcept { return mBuffer.data() + mWritePos; }

    [[nodiscard]]
    auto getReadPos() noexcept { return mBuffer.data() + mReadPos; }

    [[nodiscard]]
    size_type writablebytes() const noexcept { return mBuffer.size() - mWritePos; }

    [[nodiscard]]
    size_type readablebytes() const noexcept { return mWritePos - mReadPos; }

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
    void updateReadPos(size_type len) noexcept;

};

} // namespace net::tcp
