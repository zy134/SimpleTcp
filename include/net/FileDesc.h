#pragma once
#include "base/Utils.h"
#include <memory>
#include <string_view>

namespace simpletcp::net {

class FileDesc final {
public:
    using FilePtr = std::unique_ptr<FileDesc>;
    static FilePtr createFileDesc(std::string_view, int, uint32_t);

    DISABLE_COPY(FileDesc);
    DISABLE_MOVE(FileDesc);
    ~FileDesc();

    [[nodiscard]]
    int getFd() const noexcept { return mFd; }

    void setNonBlock();

private:
    FileDesc(std::string_view, int, uint32_t);

    int mFd;
};

using FilePtr = FileDesc::FilePtr;

inline auto operator<=>(const FilePtr& lhs, const int fd) noexcept {
    return lhs->getFd()<=>fd;
};

inline auto operator<=>(const FilePtr& lhs, const FilePtr& rhs) noexcept {
    return lhs->getFd()<=>rhs->getFd();
};


} // namespace utils
