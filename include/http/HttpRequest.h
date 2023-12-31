#pragma once

#include "http/HttpCommon.h"
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>
#include <unordered_map>

namespace simpletcp::http {

// TODO: add cookie.
struct HttpRequest {
    HttpRequest()
        : mUrl()
        , mHost()
        , mQueryParameters()
        , mType(RequestType::UNKNOWN)
        , mVersion(Version::UNKNOWN)
        , mContentType(ContentType::UNKNOWN)
        , mContentLength(-1)
        , mIsKeepAlive(false)
        , mAcceptEncodings()
        , mHeaders()
        , mBody()
        , mRawRequest()
        , mRequestSize(-1)
    {}
    // Parsed request.
    std::string                 mUrl;
    std::string                 mHost;
    std::string                 mQueryParameters;
    RequestType                 mType;
    Version                     mVersion;
    ContentType                 mContentType;
    int64_t                     mContentLength;

    std::vector<std::pair<int64_t, int64_t>>
                                mRanges;
    bool                        mIsKeepAlive;
    std::vector<EncodingType>
                                mAcceptEncodings;

    std::unordered_map<std::string, std::string>
                                mHeaders;
    std::string                 mBody;
    // Raw request data.
    std::string                 mRawRequest;
    int64_t                     mRequestSize;
};

HttpRequest parseHttpRequest(std::string_view rawHttpPacket);

void dumpHttpRequest(const HttpRequest& request);

} // namespace simpletcp::http

