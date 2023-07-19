#include "base/Error.h"
#include "net/FileDesc.h"

#include <memory>
#include <string>

extern "C" {
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/eventfd.h>
}

namespace utils {

constexpr auto DEFAULT_FILE_MODE = S_IRWXU | S_IRWXG | S_IRWXO;

FileDesc::FileDesc(std::string_view path, int flag, uint32_t mode = DEFAULT_FILE_MODE) {
    flag |= O_CREAT;
    if (flag | O_CREAT) {
        mFd = ::open(path.data(), flag, mode);
        if (mFd < 0) {
            std::string msg = "Can't open file: ";
            msg.append(path.data());
            throw utils::SystemException(msg);
        }
    } else {
        mFd = ::open(path.data(), flag);
        if (mFd < 0) {
            std::string msg = "Can't open file: ";
            msg.append(path.data());
            throw utils::SystemException(msg);
        }
    }
}


FileDesc::~FileDesc() {
    ::close(mFd);
}

void FileDesc::setNonBlock() {
    if (auto res = ::fcntl(mFd, F_SETFL, O_NONBLOCK); res != 0) {
        throw utils::SystemException("[FileDesc] failed to setNonBlock");
    }
}


FilePtr FileDesc::createFileDesc(std::string_view path, int flag, uint32_t mode) {
    return FilePtr(new FileDesc(path, flag, mode));
}

} // namespace utils
