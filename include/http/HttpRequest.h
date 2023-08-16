#pragma once
#include "http/HttpCommon.h"
#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>
#include <unordered_map>

namespace simpletcp::http {

struct HttpRequest {
    HttpRequest()
        : mUrl()
        , mHost()
        , mQueryParameters()
        , mType(HttpRequestType::UNKNOWN)
        , mVersion(HttpVersion::UNKNOWN)
        , mContentType(HttpContentType::UNKNOWN)
        , mContentLength(0)
        , mIsKeepAlive(false)
        , mAcceptEncodings()
        , mHeaders()
        , mBody()
        , mRawRequest()
        , mRequestSize(0)
    {}
    // Parsed request.
    std::string                 mUrl;
    std::string                 mHost;
    std::string                 mQueryParameters;
    HttpRequestType             mType;
    HttpVersion                 mVersion;
    HttpContentType             mContentType;
    size_t                      mContentLength;
    bool                        mIsKeepAlive;
    std::vector<EncodingType>
                                mAcceptEncodings;

    std::unordered_map<std::string, std::string>
                                mHeaders;
    std::string                 mBody;
    // Raw request data.
    std::string                 mRawRequest;
    size_t                      mRequestSize;
};

HttpRequest parseHttpRequest(std::string_view rawHttpPacket);

void dumpHttpRequest(const HttpRequest& request);

} // namespace simpletcp::http

