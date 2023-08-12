#include "base/LogBuffer.h"
#include <cstdio>
#include <cstring>
#include <fstream>

namespace simpletcp::detail {

void LogBuffer::write(std::string_view line) {
    // std::cout << "start write size:" << line.size() << std::endl;
    ::memcpy(mRawBuffer.data() + mUsedSize, line.data(), line.size());
    mUsedSize += line.size();
}

void LogBuffer::flush(std::fstream &fileStream) {
    fileStream.write(mRawBuffer.data(), static_cast<std::streamsize>(mUsedSize));
    fileStream.flush();
    mUsedSize = 0;
}

} // namespace simpletcp::detail
