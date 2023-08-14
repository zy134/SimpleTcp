#include "base/LogServer.h"
#include "base/Format.h"
#include "base/Error.h"
#include "base/LogConfig.h"
#include "base/StringHelper.h"
#include <cstdint>
#include <cstring>
#include <iostream>
#include <chrono>
#include <filesystem>
#include <ctime>
#include <string>
#include <string_view>

extern "C" {
#if defined (__linux__) || defined (__unix__) || defined (__ANDROID__)
    #include <unistd.h>
    #include <fcntl.h>
#elif defined (__WIN32) || defined(__WIN64) || defined(WIN32)
    #include <windows.h>
#else
    #error "Not support platform!"
#endif
}

using namespace std;
using namespace std::chrono;
using namespace std::chrono_literals;

#if defined (__linux__) || defined (__unix__) || defined (__ANDROID__)
static std::string getLinuxProcessName() {
    // Get process name.
    std::string procPath = "/proc/";
    std::string procCmdline;
    procCmdline.resize(256);
    procPath.append(std::to_string(::getpid())).append("/cmdline");
    auto procFd = ::open(procPath.c_str(), O_RDONLY);
    if (procFd > 0) {
        auto len = ::read(procFd, procCmdline.data(), procCmdline.size());
        if (len > 0) {
            procCmdline.resize(len);
            auto pos = procCmdline.find_last_of('/');
            if (pos != std::string_view::npos) {
                pos += 1;
                procCmdline = procCmdline.substr(pos);
            }
        }
    }
    if (procCmdline.empty()) {
        procCmdline = "Unknown";
    } else {
        procCmdline = simpletcp::utils::strip(procCmdline);
    }
    return procCmdline;
}
#endif

namespace simpletcp::detail {

LogServer::LogServer() : mCurrentDate(duration_cast<days>(system_clock::now().time_since_epoch())) {
    // Initialize static variables
#if defined (__linux__) || defined (__unix__) || defined (__ANDROID__)
    mProcessId = ::getpid();
    mProcessName = getLinuxProcessName();
#elif defined (__WIN32) || defined(__WIN64) || defined(WIN32)
    mProcessId = GetCurrentProcessId();
    // FIXME:
    // Add support for windows.
    mProcessName = "Unknown";
#else
    #error "Not support platform!"
#endif
    mProcessIdStr = simpletcp::format("{:5d}", mProcessId);

    // Create log file.
    mLogFileStream = createLogFileStream();
    mLogAlreadyWritenBytes = 0;

    // Create LogServer thread.
    mStopThread = false;
    mNeedFlushNow = false;
    mpCurrentBuffer = std::make_unique<LogBuffer>();
    mFlushThread = std::thread([this] {
        doFlushAsync();
    });
}

LogServer::~LogServer() {
    forceDestroy();
}

void LogServer::forceDestroy() noexcept {
    try {
        // std::lock_guard lock { mMutex };
        // Notify flush thread to syncronize log buffer and close log file.
        mStopThread = true;
        mCond.notify_one();
        // Destroy flush thread.
        if (mFlushThread.joinable()) {
            mFlushThread.join();
        }
    } catch (const std::exception& e) {
        // ignore exception
        std::cerr << e.what() << std::endl;
    }
}

void LogServer::forceFlush() noexcept {
    try {
        // std::lock_guard lock { mMutex };
        mNeedFlushNow = true;
        mCond.notify_one();
    } catch (const std::exception& e) {
        // ignore exception.
        std::cerr << e.what() << std::endl;
    }
}

void LogServer::write(LogLevel level, std::string_view formatted, std::string_view tag) {
#if defined (__linux__) || defined (__unix__) || defined (__ANDROID__)
    thread_local static int tCurThreadId = gettid();
#elif defined (__WIN32) || defined(__WIN64) || defined(WIN32)
    thread_local static int tCurThreadId = GetCurrentThreadId();
#else
    #error "Not support platform!"
#endif
    thread_local static std::string tCurThreadIdStr = simpletcp::format("{:5d}", tCurThreadId);

    constexpr auto log_level_to_string= [] (LogLevel l) {
        switch (l) {
            case LogLevel::Version:
                return "Ver  ";
            case LogLevel::Debug:
                return "Debug";
            case LogLevel::Info:
                return "Info ";
            case LogLevel::Warning:
                return "Warn ";
            case LogLevel::Error:
                return "Error";
            case LogLevel::Fatal:
                return "Fatal";
            default:
                return " ";
        }
    };


    // Get date.
    thread_local static chrono::days tCurDate;
    thread_local static std::string tCurDateStr;
    if (tCurDate < mCurrentDate) {
        tCurDate = mCurrentDate;
        auto t = system_clock::to_time_t(system_clock::now());
        std::tm date;
        if (::gmtime_r(&t, &date) == nullptr) {
            throw SystemException {"[LogServer] invoke gmtime_r failed."};
        }
        tCurDateStr = simpletcp::format("{:04d}-{:02d}-{:02d}"
                , date.tm_year + 1900, date.tm_mon + 1, date.tm_mday);
    }

    // Get timestamp
    auto timestamp = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    auto suffix = timestamp - tCurDate;
    auto suffixHour = duration_cast<hours>(suffix); suffix -= suffixHour;
    auto suffixMin = duration_cast<minutes>(suffix); suffix -= suffixMin;
    auto suffixSec = duration_cast<seconds>(suffix); suffix -= suffixSec;

#if LOG_STYLE_FMT
    // Use libfmt to format log string.
    std::vector<char> logBuffer {};
    logBuffer.reserve(64);
    simpletcp::format_to(std::back_inserter(logBuffer)
            , "{} {:02d}:{:02d}:{:02d}.{:03d} {} {} [{}][{}] {}\n"
            , tCurDateStr
            , suffixHour.count(), suffixMin.count(), suffixSec.count(), suffix.count()
            , mProcessIdStr, tCurThreadIdStr
            , log_level_to_string(level), tag
            , formatted
            );
    std::string_view logLine { logBuffer.data(), logBuffer.size() };
#else
    // Use C-style snprintf to format log string. Faster than fmt...
    std::array<char, LOG_MAX_LINE_SIZE> logBuffer;
    auto len = snprintf(logBuffer.data(), logBuffer.size()
            , "%04d-%02d-%02d %02d:%02d:%02d.%03d %s %s [%s][%s] %s\n"
            , timeStruct.tm_year + 1900, timeStruct.tm_mon + 1, timeStruct.tm_mday
            , timeStruct.tm_hour, timeStruct.tm_min, timeStruct.tm_sec, static_cast<int>(millisSuffix)
            , mProcessIdStr.c_str(), tCurThreadIdStr.c_str()
            , log_level_to_string(level), tag.data()
            , formatted.data()
    );
    [[unlikely]]
    if (len < 0) {
        throw SystemException("[LogServer] snprintf error.");
    }
    std::string_view logLine { logBuffer.data(), static_cast<size_t>(len) };
#endif

    std::lock_guard lock { mMutex };
    // Write log line to memory buffer.
    [[likely]]
    if (mpCurrentBuffer->writable(logLine.size())) {
        mpCurrentBuffer->write(logLine);
    } else {
        if (mvPendingBuffers.size() <= MAX_PENDING_BUFFERS) {
            // Current buffer is full, need to flush.
            mvPendingBuffers.emplace_back(std::move(mpCurrentBuffer));
            // And get new availble buffer.
            if (mvAvailbleBuffers.empty()) {
                mpCurrentBuffer = std::make_unique<LogBuffer>();
            } else {
                mpCurrentBuffer = std::move(mvAvailbleBuffers.back());
                mvAvailbleBuffers.pop_back();
            }
            mpCurrentBuffer->write(logLine);
            // Notify backend server to flush pending buffers.
            mCond.notify_one();
        } else {
            // There has too much pending buffers.
            // Discard new log request, and just notify asynchronous thread to flush pending buffers.
            std::cerr << "[LogServer] Too much pending buffers!" << std::endl;
            mCond.notify_one();
        }
    }
}

void LogServer::doFlushAsync() {
    std::vector<std::unique_ptr<LogBuffer>> needFlushBuffers;
    while (!mStopThread) {
        // Get pending buffers.
        {
            std::unique_lock lock { mMutex };
            // Wait for availble pending buffers.
            auto timeout = !mCond.wait_for(lock, DEFAULT_FLUSH_INTERVAL, [&] {
                return mStopThread || !mvPendingBuffers.empty() || mNeedFlushNow;
            });
            // Flush thread is exited, so flush all buffers, and then close log file.
            if (mStopThread) {
                for (auto& buffer: mvPendingBuffers) {
                    buffer->flush(mLogFileStream);
                }
                if (mpCurrentBuffer->flushEnable()) {
                    mpCurrentBuffer->flush(mLogFileStream);
                }
                mLogFileStream.flush();
                mLogFileStream.close();
                return ;
            }
            // Condition wait time out, there has no pending buffers, so flush current buffer to log file.
            // mNeedFlushNow == true, so need to flush current buffer immediately.
            if (timeout || mNeedFlushNow) {
                mvPendingBuffers.emplace_back(std::move(mpCurrentBuffer));
                if (mvAvailbleBuffers.empty()) {
                    mpCurrentBuffer = std::make_unique<LogBuffer>();
                } else {
                    mpCurrentBuffer = std::move(mvAvailbleBuffers.back());
                    mvAvailbleBuffers.pop_back();
                }
                // set mNeedFlushNow as false.
                mNeedFlushNow = false;
            }
            needFlushBuffers.swap(mvPendingBuffers);
        }
        // Start flush buffer to log file.
        // This operation may take long time, so don't lock mutex now.
        for (auto& buffer: needFlushBuffers) {
            if (buffer->size() + mLogAlreadyWritenBytes >= LOG_MAX_FILE_SIZE) {
                // std::cout << __FUNCTION__ << ": Log file is full, create new log file." << std::endl;
                mLogFileStream.flush();
                mLogFileStream.close();
                mLogFileStream = createLogFileStream();
                mLogAlreadyWritenBytes = 0;
            }
            mLogAlreadyWritenBytes += buffer->size();
            buffer->flush(mLogFileStream);
        }
        // Return availble buffers.
        // std::cout << __FUNCTION__ << ": Return buffers" << std::endl;
        {
            std::lock_guard lock { mMutex };
            for (auto& buffer: needFlushBuffers) {
                mvAvailbleBuffers.emplace_back(std::move(buffer));
            }
            needFlushBuffers.clear();
        }
        // update date in flush thread.
        mCurrentDate = duration_cast<days>(system_clock::now().time_since_epoch());
    }

}

std::fstream LogServer::createLogFileStream() {
    // Create log path and change its permissions to 0777.
    // TODO: Permission error may be happen, how to deal with that case?
    try {
        // create_directory() return false because the directory is existed, ignored it.
        // We just focus on the case which create_directory() or permissions() throw a exception
        std::filesystem::create_directory(DEFAULT_LOG_PATH);
        std::filesystem::permissions(DEFAULT_LOG_PATH, std::filesystem::perms::all);
    } catch (...) {
        std::cerr << "[LogServer] Failed to create log file!" << strerror(errno) << std::endl;
        throw SystemException("Can't create log filePath because:");
    }

    // Format the name of log file.
    time_t t = system_clock::to_time_t(system_clock::now());
    struct tm now = {};
    // TODO: Deal with this exception.
    // No need to check result. it would throw exception.
    ::gmtime_r(&t, &now);

    auto filePath = simpletcp::format("{}/{}_{:d}_{:04d}-{:02d}-{:02d}_{:02d}-{:02d}-{:02d}.log"
            , DEFAULT_LOG_PATH
            , mProcessName.data()
            , mProcessId
            , now.tm_year + 1900, now.tm_mon + 1, now.tm_mday
            , now.tm_hour, now.tm_min, now.tm_sec
    );

    // Create log file.
    try {
        auto fileStream = std::fstream(filePath, std::ios::in | std::ios::out | std::ios::trunc);
        return fileStream;
    } catch (...) {
        throw SystemException("Can't create log file because:");
    }
}

} // namespace simpletcp::detail
