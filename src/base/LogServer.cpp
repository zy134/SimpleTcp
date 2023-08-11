#include "base/LogServer.h"
#include "base/Format.h"
#include "base/Error.h"
#include "base/LogConfig.h"
#include <bits/types/time_t.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <chrono>
#include <filesystem>
#include <ctime>
#include <iterator>
#include <math.h>
#include <string_view>

extern "C" {
#if defined (__linux__) || defined (__unix__) || defined (__ANDROID__)
    #include <unistd.h>
#elif defined (__WIN32) || defined(__WIN64) || defined(WIN32)
    #include <windows.h>
#else
    #error "Not support platform!"
#endif
}

using namespace std;
using namespace std::chrono;
using namespace std::chrono_literals;

namespace simpletcp::detail {

LogServer::LogServer() {
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
    static int pid = getpid();
    thread_local static int tid = gettid();
#elif defined (__WIN32) || defined(__WIN64) || defined(WIN32)
    static int pid = GetCurrentProcessId();
    thread_local static int tid = GetCurrentThreadId();
#else
    #error "Not support platform!"
#endif
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


// Get timestamp.
// #if defined (__linux__) || defined (__unix__) || defined (__ANDROID__)
// The precise of CLOCK_REALTIME_COARSE is too low, instead we use std::system_clock to get current time.
#if 0
    std::timespec tp {};
    [[unlikely]]
    if (::clock_gettime(CLOCK_REALTIME_COARSE, &tp) != 0) {
        throw SystemException {"[LogServer] invoke clock_gettime failed."};
    }
    std::time_t curTime = tp.tv_sec;
    auto millisSuffix = tp.tv_nsec / 1'000'000;
#else
    auto now = system_clock::now();
    std::time_t curTime = system_clock::to_time_t(now);
    auto millisSuffix = duration_cast<milliseconds>(now.time_since_epoch()).count() % 1000;
#endif

    std::tm timeStruct {};
    [[unlikely]]
    if (::gmtime_r(&curTime, &timeStruct) == nullptr) {
        throw SystemException {"[LogServer] invoke gmtime_r failed."};
    }

#if LOG_STYLE_FMT
    // Use libfmt to format log string.
    std::vector<char> logBuffer;
    logBuffer.reserve(32);
    simpletcp::format_to(std::back_inserter(logBuffer)
            , "{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}.{:03d} {:5d} {:5d} [{}][{}] {}\n"
            , timeStruct.tm_year + 1900, timeStruct.tm_mon + 1, timeStruct.tm_mday
            , timeStruct.tm_hour, timeStruct.tm_min, timeStruct.tm_sec, millisSuffix
            , pid, tid
            , log_level_to_string(level), tag
            , formatted
            );
    std::string_view logLine { logBuffer.data(), logBuffer.size() };
#else
    // Use C-style snprintf to format log string. Faster than fmt...
    std::array<char, LOG_MAX_LINE_SIZE> logBuffer;
    auto len = snprintf(logBuffer.data(), logBuffer.size()
            , "%04d-%02d-%02d %02d:%02d:%02d.%03d %05d %05d [%s][%s] %s\n"
            , timeStruct.tm_year + 1900, timeStruct.tm_mon + 1, timeStruct.tm_mday
            , timeStruct.tm_hour, timeStruct.tm_min, timeStruct.tm_sec, static_cast<int>(millisSuffix)
            , pid, tid
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
    if (mpCurrentBuffer->writable(logLine.size())) {
        mpCurrentBuffer->write(logLine);
    } else {
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
        std::cerr << "Failed to create log file!" << strerror(errno) << std::endl;
        throw SystemException("Can't create log filePath because:");
    }

    // Format the name of log file.
    time_t t = system_clock::to_time_t(system_clock::now());
    struct tm now = {};
    // TODO: Deal with this exception.
    // No need to check result. it would throw exception.
    ::gmtime_r(&t, &now);

    auto filePath = simpletcp::format("{}/{:04d}-{:02d}-{:02d}_{:02d}-{:02d}-{:02d}.log"
            , DEFAULT_LOG_PATH
            , now.tm_year + 1900, now.tm_mon + 1, now.tm_mday
            , now.tm_hour, now.tm_min, now.tm_sec
    );

    // Create log file.
    try {
        auto fileStream = std::fstream(filePath.data(), std::ios::in | std::ios::out | std::ios::trunc);
        return fileStream;
    } catch (...) {
        std::cerr << "Failed to create log file!" << strerror(errno) << std::endl;
        throw SystemException("Can't create log file because:");
    }
}

} // namespace simpletcp::detail
