#pragma once

#include "base/Utils.h"
#include "base/LogConfig.h"
#include <array>
#include <string_view>

namespace simpletcp::detail {

// LogBuffer is a simple memory buffer wrapper.
// Not thread-safety!
class LogBuffer final {
    DISABLE_COPY(LogBuffer);
    DISABLE_MOVE(LogBuffer);

public:
    using BufferType = std::array<char, LOG_BUFFER_SIZE>;
    using SizeType = BufferType::size_type;

    LogBuffer() {
        mUsedSize = 0;
    }

    ~LogBuffer() noexcept = default;

    [[nodiscard]]
    bool writable(size_t size) const {
        return (mRawBuffer.size() - mUsedSize) >= size;
    }

    void write(std::string_view line);

    [[nodiscard]]
    bool flushEnable() const noexcept {
        return mUsedSize > 0;
    }

    void flush(std::fstream &fileStream);

    [[nodiscard]]
    size_t size() const noexcept {
        return mUsedSize;
    }

private:
    std::array<char, LOG_BUFFER_SIZE>   mRawBuffer;
    size_t                              mUsedSize;
};

}// namespace utils
