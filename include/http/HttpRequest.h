#pragma once
#include "http/HttpCommon.h"
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>

namespace simpletcp::http {

struct HttpRequest {
    // Parsed request.
    std::string                 mUrl;
    std::string                 mQueryParameters;
    HttpRequestType             mType;
    HttpVersion                 mVersion;
    bool                        mIsKeepAlive;

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

