#include "base/LogBuffer.h"
#include <cstring>
#include <fstream>
#include <iostream>

namespace simpletcp::detail {

void LogBuffer::write(std::string_view line) {
    ::memcpy(mRawBuffer.data() + mUsedSize, line.data(), line.size());
    mUsedSize += line.size();
}

void LogBuffer::flush(std::fstream &fileStream) {
    fileStream.write(mRawBuffer.data(), static_cast<std::streamsize>(mUsedSize));
    fileStream.flush();
	fileStream.sync();
    mUsedSize = 0;
}

} // namespace simpletcp::detail
