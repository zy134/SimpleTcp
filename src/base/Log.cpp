#include <base/Log.h>
#include <base/Utils.h>
#include <base/Error.h>
#include <base/Backtrace.h>

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <exception>
#include <iostream>
#include <memory>
#include <string_view>
#include <sstream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <array>
#include <future>
#include <chrono>
#include <utility>
#include <vector>
#include <fstream>
#include <filesystem>
#include <ctime>

#ifdef TAG
#undef TAG
#endif
static constexpr std::string_view TAG = "LOG";

namespace simpletcp {

void printBacktrace() {
    static std::mutex gBtMutex;
    std::lock_guard lock { gBtMutex };
    auto backtraces = getBacktrace();
    LOG_WARN("================================================================================");
    LOG_WARN("============================== Start print backtrace ===========================");
    for (size_t i = 1; i < backtraces.size(); ++i) {
        detail::LogServer::getInstance().write(LogLevel::Warning, backtraces[i], TAG);
    }
    LOG_WARN("=============================== End print backtrace  ===========================");
    // Use LOG_ERR to flush current buffer to log file in last log line.
    LOG_ERR("================================================================================");
}

} // namespace utils
