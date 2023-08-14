#include "http/HttpResponse.h"
#include "base/Format.h"
#include "base/Log.h"
#include "http/HttpCommon.h"
#include "http/HttpError.h"
#include <chrono>
#include <ctime>
#include <exception>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>
#include <iostream>

using namespace std;
using namespace std::filesystem;
using namespace std::chrono;
using namespace std::string_literals;

inline static constexpr std::string_view TAG = "HttpResponse";

[[maybe_unused]] 
inline static constexpr std::string_view CRLF = "\r\n";

namespace simpletcp::http {

static std::string readFile(const std::filesystem::path& filePath) {
    auto file = std::fstream { filePath, std::ios::in };
    auto fileSize = filesystem::file_size(filePath);
    std::string buffer;
    buffer.resize(fileSize);
    file.read(buffer.data(), static_cast<streamsize>(fileSize));
    return buffer;
}

void HttpResponse::setDate() {
    auto time = system_clock::to_time_t(system_clock::now());
    std::tm tm {};
    if (localtime_r(&time, &tm) == nullptr) {
        LOG_ERR("{}; invoke localtime_r failed!", __FUNCTION__);
        return ;
    }
    std::array<char, 32> buffer;
    auto len = strftime(buffer.data(), buffer.size(), "%c", &tm);
    if (len <= 0) {
        LOG_ERR("{}; invoke strftime failed!", __FUNCTION__);
        return ;
    }
    std::string_view date_str { buffer.data(), len };
    addHeader("Date", date_str);
}

void HttpResponse::addHeader(std::string_view key, std::string_view value) {
    if (mHeaders.count(key.data()) != 0) {
        LOG_INFO("{}: key:{}, old value({})->new value({})", __FUNCTION__, key, mHeaders.at(key.data()), value);
        mHeaders.at(key.data()) = value.data();
    } else {
        LOG_INFO("{}: key:{} value:{}", __FUNCTION__, key, value);
        mHeaders.insert({ key.data(), value.data() });
    }
}

void HttpResponse::setContentByFilePath(const std::filesystem::path& filePath, HttpContentType type) {
    TRACE();
    try {
        LOG_INFO("{}: set content type :{}", __FUNCTION__, to_string_view(type));
        setBody(readFile(filePath));
        setContentType(type);
        setContentLenght(mBody.size());
    } catch (const std::exception& e) {
        LOG_ERR("{}: exception happen ! {}", __FUNCTION__, e.what());
        printBacktrace();
        throw ResponseError {"[HttpResponse] content file not found!", ResponseErrorType::BadContent};
    }
}

std::string HttpResponse::generateResponse() {
    TRACE();
    // Check is this response valid.
    [[unlikely]]
    if (mVersion == HttpVersion::UNKNOWN) {
        throw ResponseError {"[HttpResponse] please set http version correctly!", ResponseErrorType::BadVersion};
    }
    [[unlikely]]
    if (mStatus == HttpStatusCode::UNKNOWN) {
        throw ResponseError {"[HttpResponse] please set status code correctly!", ResponseErrorType::BadStatus};
    }
    [[unlikely]]
    if (mContentType == HttpContentType::UNKNOWN) {
        throw ResponseError {"[HttpResponse] please set content type correctly !", ResponseErrorType::BadContentType};
    }
    
    std::string buffer;
    // Generate status line
    auto statusLine = simpletcp::format("{} {}", to_string_view(mVersion), to_string_view(mStatus));
    buffer.append(statusLine);
    buffer.append(CRLF);
    LOG_INFO("{}: response status {}", __FUNCTION__, to_string_view(mStatus));

    // Generate headers
    // Set content type and charset
    if (mCharSet != CharSet::UNKNOWN) {
        addHeader("Content-Type", simpletcp::format("{}; {}"
                , to_string_view(mContentType), to_string_view(mCharSet)));
    } else {
        addHeader("Content-Type", to_string_view(mContentType));
    }
    // set keep-alive property.
    if (mIsKeepAlive) {
        addHeader("Connection", "keep-alive");
    } else {
        addHeader("Connection", "close");
    }
    // add other headers
    for (auto&& header : mHeaders) {
        buffer.append(header.first).append(": ").append(header.second).append(CRLF);
    }
    buffer.append(CRLF);
    if (mBody.empty()) {
        LOG_DEBUG("{}: response", __FUNCTION__);
        return buffer;
    }

    // Generate content.
    buffer.append(mBody);
    buffer.append(CRLF);
    LOG_DEBUG("{}: response", __FUNCTION__);
    return buffer;
}

void HttpResponse::dump() const {
    LOG_DEBUG("{}: http version: {}", __FUNCTION__, to_string_view(mVersion));
    LOG_DEBUG("{}: status: {}", __FUNCTION__, to_string_view(mStatus));
    LOG_DEBUG("{}: keep-alive: {}", __FUNCTION__, mIsKeepAlive);
    LOG_DEBUG("{}: contentType: {}", __FUNCTION__, to_string_view(mContentType));
    LOG_DEBUG("{}: charset: {}", __FUNCTION__, to_string_view(mCharSet));
    for (auto&& header : mHeaders) {
        LOG_DEBUG("{}: key:{}, value:{}", __FUNCTION__, header.first, header.second);
    }

    if (mContentType == HttpContentType::HTML || mContentType == HttpContentType::PLAIN
            || mContentType == HttpContentType::JSON || mContentType == HttpContentType::XML) {
        LOG_DEBUG("{}: {}", __FUNCTION__, mBody);
    }
    
    std::cout << "[Response] http version: " << to_string_view(mVersion) << std::endl;
    std::cout << "[Response] status: " << to_string_view(mStatus) << std::endl;
    std::cout << "[Response] keep-alive: " << mIsKeepAlive << std::endl;
    std::cout << "[Response] contentType: " << to_string_view(mContentType) << std::endl;
    std::cout << "[Response] charset: " << to_string_view(mCharSet) << std::endl;
    for (auto&& header : mHeaders) {
        std::cout << "[Response] key:" << header.first << "value: " << header.second << std::endl;
    }

    if (mContentType == HttpContentType::HTML || mContentType == HttpContentType::PLAIN
            || mContentType == HttpContentType::JSON || mContentType == HttpContentType::XML) {
        std::cout << "[Response] " << mBody << std::endl;
    }
}

} // namespace simpletcp::http
