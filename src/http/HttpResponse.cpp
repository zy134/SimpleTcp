#include "base/Format.h"
#include "base/Log.h"
#include "base/Compress.h"
#include "http/HttpResponse.h"
#include "http/HttpCommon.h"
#include "http/HttpError.h"
#include <algorithm>
#include <chrono>
#include <ctime>
#include <exception>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <iostream>
#include <vector>

using namespace std;
using namespace std::filesystem;
using namespace std::chrono;
using namespace std::string_literals;

inline static constexpr std::string_view TAG = "HttpResponse";

[[maybe_unused]] 
inline static constexpr std::string_view CRLF = "\r\n";

namespace simpletcp::http {

const std::unordered_map<std::string_view, ContentType> Str2ContentType {
    { ".html", ContentType::HTML },
    { ".jpg", ContentType::JPEG },
    { ".png", ContentType::PNG },
    { ".txt", ContentType::PLAIN },
    { ".js", ContentType::JAVASCRIPT },
    { ".json", ContentType::JAVASCRIPT },
    { ".xml", ContentType::XML },
    { ".mp4", ContentType::MP4 },
};

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
    setProperty("Date", date_str);
}

void HttpResponse::setContentByFilePath(const std::filesystem::path& filePath) {
    TRACE();
    try {
        ContentType type;
        if (!filePath.has_extension()) {
            type = ContentType::BINARY;
        } else {
            if (Str2ContentType.count(filePath.extension().c_str()) == 0) {
                std::string errMsg = "[setContentByFilePath] unsupport file type! Path:";
                errMsg.append(filePath);
                throw ResponseError(errMsg, ResponseErrorType::BadContentType);
            }
            type = Str2ContentType.at(filePath.extension().c_str());
        }
        // Need compress?
        if (auto selectEncode = selectEncodeType(filePath); selectEncode == EncodingType::NO_ENCODING) {
            LOG_INFO("{}: set content type :{}", __FUNCTION__, to_string_view(type));
            setBody(readFile(filePath));
        } else {
            LOG_INFO("{}: set content type :{}", __FUNCTION__, to_string_view(type));
            if (selectEncode == EncodingType::GZIP) {
                setBody(utils::compress_gzip(readFile(filePath)));
                setProperty("Content-Encoding", to_string_view(EncodingType::GZIP));
            } else {
                setBody(utils::compress_deflate(readFile(filePath)));
                setProperty("Content-Encoding", to_string_view(EncodingType::DEFLATE));
            }
        }
        setContentType(type);
        setContentLength(mBody.size());
    } catch (const std::exception& e) {
        LOG_ERR("{}: exception happen ! {}", __FUNCTION__, e.what());
        printBacktrace();
        throw ResponseError {"[HttpResponse] content file not found!", ResponseErrorType::BadContent};
    }
}

void HttpResponse::setProperty(std::string_view key, std::string_view value) {
    if (mHeaders.count(key.data()) != 0) {
        LOG_INFO("{}: key:{}, old value({})->new value({})", __FUNCTION__, key, mHeaders.at(key.data()), value);
        mHeaders.at(key.data()) = value.data();
    } else {
        LOG_INFO("{}: key:{} value:{}", __FUNCTION__, key, value);
        mHeaders.insert({ key.data(), value.data() });
    }
}
    
auto HttpResponse::getProperty(std::string_view key) -> std::optional<std::string_view> {
    std::optional<std::string_view> result {}; // enable NRVO
    if (mHeaders.count(key.data()) > 0) {
        result = mHeaders.at(key.data());
    }
    return result;
}

std::string HttpResponse::generateResponse() {
    TRACE();
    // Check is this response valid.
    [[unlikely]]
    if (mVersion == Version::UNKNOWN) {
        throw ResponseError {"[HttpResponse] please set http version correctly!", ResponseErrorType::BadVersion};
    }
    [[unlikely]]
    if (mStatus == StatusCode::UNKNOWN) {
        throw ResponseError {"[HttpResponse] please set status code correctly!", ResponseErrorType::BadStatus};
    }
    
    std::string buffer;
    // Generate status line
    auto statusLine = simpletcp::format("{} {}", to_string_view(mVersion), to_string_view(mStatus));
    buffer.append(statusLine);
    buffer.append(CRLF);
    LOG_INFO("{}: response status {}", __FUNCTION__, to_string_view(mStatus));

    // Generate headers
    // Set content type and length
    if (mContentType != ContentType::UNKNOWN) {
        if (mCharSet != CharSet::UNKNOWN) {
            setProperty("Content-Type", simpletcp::format("{}; {}"
                    , to_string_view(mContentType), to_string_view(mCharSet)));
        } else {
            setProperty("Content-Type", to_string_view(mContentType));
        }
    }
    setProperty("Content-Length", std::to_string(mContentLength));

    // Set content range
    if (mContentRange.total != 0) {
        setProperty("Content-Range", simpletcp::format("bytes {}-{}/{}"
                    ,mContentRange.start, mContentRange.end, mContentRange.total));
    }
    // set keep-alive property.
    if (mIsKeepAlive) {
        setProperty("Connection", "keep-alive");
    } else {
        setProperty("Connection", "close");
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
    LOG_DEBUG("{}: response", __FUNCTION__);
    return buffer;
}

EncodingType HttpResponse::selectEncodeType(const std::filesystem::path& filePath) const {
    if (!filePath.has_extension() || mAvailEncodings.empty()) {
        return EncodingType::NO_ENCODING;
    }
    const auto& ext = filePath.extension();
    if (ext == ".html" || ext == ".xml" || ext == ".js" || ext == ".json") {
        return mAvailEncodings[0];
    }
    return EncodingType::NO_ENCODING;
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

    if (mContentType == ContentType::HTML || mContentType == ContentType::PLAIN
            || mContentType == ContentType::JSON || mContentType == ContentType::XML) {
        LOG_DEBUG("{}: {}", __FUNCTION__, mBody);
    }
    
    std::cout << "[Response] http version: " << to_string_view(mVersion) << std::endl;
    std::cout << "[Response] status: " << to_string_view(mStatus) << std::endl;
    std::cout << "[Response] keep-alive: " << mIsKeepAlive << std::endl;
    std::cout << "[Response] contentType: " << to_string_view(mContentType) << std::endl;
    std::cout << "[Response] charset: " << to_string_view(mCharSet) << std::endl;
    for (auto&& header : mHeaders) {
        std::cout << "[Response] key:" << header.first << ", value: " << header.second << std::endl;
    }

    if (mContentType == ContentType::HTML || mContentType == ContentType::PLAIN
            || mContentType == ContentType::JSON || mContentType == ContentType::XML) {
        std::cout << "[Response] " << mBody << std::endl;
    }
}

} // namespace simpletcp::http
