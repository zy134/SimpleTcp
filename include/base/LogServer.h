#pragma once
#include "base/LogBuffer.h"
#include "base/LogConfig.h"
#include "base/Format.h"
#include <chrono>
#include <fstream>
#include <iterator>
#include <mutex>
#include <string_view>
#include <thread>
#include <condition_variable>
#include <utility>
#include <vector>

namespace simpletcp::detail {


// LogServer is the backend server, which manage multiple memory buffers and
// flush these buffers to Log file asynchronously in appropriate time.
class LogServer final {
    DISABLE_COPY(LogServer);
    DISABLE_MOVE(LogServer);
public:
    LogServer();

    ~LogServer();

    static LogServer& getInstance() {
        static LogServer gLogServerInstance;
        return gLogServerInstance;
    }

    // Call by LOG_FATAL, force flush all log files and terminate process.
    // Thread-safety.
    void forceDestroy() noexcept;

    // Call by LOG_ERR, force flush current log buffer.
    // Thread-safety.
    void forceFlush() noexcept;

    template <StringType T, Formatable...Args>
    void format(LogLevel level, std::string_view tag, T&& fmt, Args&&...args) {
        std::vector<char> buffer;
        simpletcp::format_to(std::back_inserter(buffer), std::forward<T>(fmt), std::forward<Args>(args)...);
        std::string_view formatted { buffer.data(), buffer.size() };
        write(level, formatted, tag);
    }

private:
    // Client would call this funtion to format log line and write it to LogBuffer.
    // Thread-safety.
    void write(LogLevel level, std::string_view formatted, std::string_view tag);

    auto createLogFileStream() -> std::fstream;

    void doFlushAsync();

    // Default interval time which LogServer would flush all current buffer to log file.
    static constexpr auto DEFAULT_FLUSH_INTERVAL = std::chrono::milliseconds(2000);

    std::fstream            mLogFileStream;
    LogBuffer::SizeType     mLogAlreadyWritenBytes;

    std::mutex              mMutex;
    std::condition_variable mCond;
    std::thread             mFlushThread;
    bool                    mStopThread;
    bool                    mNeedFlushNow;
    std::unique_ptr<LogBuffer>
                            mpCurrentBuffer;
    std::vector<std::unique_ptr<LogBuffer>>
                            mvAvailbleBuffers;
    std::vector<std::unique_ptr<LogBuffer>>
                            mvPendingBuffers;
};

} // namespace simpletcp::detail
