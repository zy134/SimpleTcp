#include "base/Log.h"
#include "base/StringHelper.h"
#include "http/HttpCommon.h"
#include "http/HttpError.h"
#include "http/HttpRequest.h"
#include <exception>
#include <string_view>
#include <iostream>
#include <charconv>
#include <system_error>

static constexpr std::string_view TAG = "HttpRequest";

static constexpr std::string_view CRLF = "\r\n";

static constexpr std::string_view BLANK_LINE = "\r\n\r\n";

using namespace std;
using namespace simpletcp;
using namespace simpletcp::utils;

namespace simpletcp::http {

// TODO : Parse request by regex
static bool parseStartLine(std::string_view requestLine, HttpRequest& request) {
    TRACE();
    if (requestLine.empty()) {
        LOG_ERR("{}: parse error, empty request!", __FUNCTION__);
        return false;
    }
    const auto parser = [&] {
        if (requestLine.find("HTTP/1.1") != std::string_view::npos) {
            request.mVersion = Version::HTTP1_1;
        } else if (requestLine.find("HTTP/1.0") != std::string_view::npos) {
            request.mVersion = Version::HTTP1_0;
        } else {
            LOG_ERR("parseStartLine: parse error, unknown http version!");
            return false;
        }
        auto url_start = requestLine.find('/');
        auto url_end = requestLine.find('?');
        if (url_start == std::string_view::npos) {
            LOG_ERR("parseStartLine: parse error, bad request!");
            return false;
        }
        if (url_end == std::string_view::npos) {
            // no query parameters
            url_end = requestLine.find("HTTP");
        } else {
            auto query_begin = url_end;
            auto query_end = requestLine.find("HTTP");
            request.mQueryParameters = std::string { requestLine.data() + query_begin, query_end - query_begin };
        }
        url_start += 1;
        request.mUrl = strip({ requestLine.data() + url_start, url_end - url_start });
        return true;
    };
    if (requestLine.find("GET") != std::string_view::npos) {
        request.mType = RequestType::GET;
        if (!parser())
            return false;
    } else if (requestLine.find("POST") != std::string_view::npos) {
        request.mType = RequestType::POST;
        if (!parser())
            return false;
    } else {
        LOG_ERR("{}: parse error, unknown type request!", __FUNCTION__);
        return false;
    }
    return true;
}

static bool parseHeaders(std::string_view headers, HttpRequest& request) {
    // String translate helper functors
    constexpr auto to_encoding_type = [] (std::string_view type) {
        if (type == to_string_view(EncodingType::GZIP)) return EncodingType::GZIP;
        if (type == to_string_view(EncodingType::BR)) return EncodingType::BR;
        if (type == to_string_view(EncodingType::DEFLATE)) return EncodingType::DEFLATE;
        return EncodingType::NO_ENCODING;
    };

    constexpr auto to_content_type = [] (std::string_view type) {
        if (type == to_string_view(ContentType::HTML)) return ContentType::HTML;
        if (type == to_string_view(ContentType::JSON)) return ContentType::JSON;
        if (type == to_string_view(ContentType::JPEG)) return ContentType::JPEG;
        if (type == to_string_view(ContentType::PNG)) return ContentType::PNG;
        if (type == to_string_view(ContentType::GIF)) return ContentType::GIF;
        if (type == to_string_view(ContentType::PLAIN)) return ContentType::PLAIN;
        return ContentType::UNKNOWN;
    };
    //

    TRACE();
    if (headers.empty()) {
        LOG_INFO("{}: empty headers...", __FUNCTION__);
        return true;
    }
    auto start_pos = 0ul;
    auto end_pos = headers.find(CRLF, start_pos);
    while (end_pos != std::string_view::npos) {
        std::string_view current_header { headers.data() + start_pos, end_pos - start_pos };

        auto delim_pos = current_header.find(':');
        if (delim_pos != std::string_view::npos) {
            std::string key { current_header.begin(), current_header.begin() + delim_pos };
            std::string value { current_header.begin() + delim_pos + 1, current_header.end() };
            value = strip(value);
            request.mHeaders.insert({ std::move(key), std::move(value) });
        }

        start_pos = end_pos + CRLF.size();
        end_pos = headers.find(CRLF, start_pos);
    }
    // Set host
    if (auto iter = request.mHeaders.find("Host"); iter != request.mHeaders.end()) {
        request.mHost = iter->second;
    }
    // Set connection state
    if (auto iter = request.mHeaders.find("Connections");
            iter != request.mHeaders.end() && iter->second == "keep-alive") {
        request.mIsKeepAlive = true;
    }
    // Set availble encodings.
    if (auto iter = request.mHeaders.find("Accept-Encoding"); iter != request.mHeaders.end()) {
        auto split_result = split(iter->second, ",");
        for (auto&& encoding_str : split_result) {
            request.mAcceptEncodings.push_back(to_encoding_type(strip(encoding_str)));
        }
    }
    // Set content info.
    if (request.mType != RequestType::GET) {
        if (auto iter = request.mHeaders.find("Content-Type"); iter != request.mHeaders.end()) {
            auto content_type = to_content_type(iter->second);
            if (content_type == ContentType::UNKNOWN) {
                LOG_ERR("{}: Unknown content type!", __FUNCTION__);
                return false;
            }
            request.mContentType = content_type;
            if (auto len = request.mHeaders.find("Content-Length"); len != request.mHeaders.end()) {
                auto res = std::from_chars(len->second.c_str()
                        , len->second.c_str() + len->second.size(), request.mContentLength);
                if (res.ec == std::errc::invalid_argument) {
                    LOG_ERR("{}: Unknown content length argument {}!", __FUNCTION__, len->second.c_str());
                    return false;
                }
            }
        }
    }
    return true;
}

HttpRequest parseHttpRequest(std::string_view rawHttpPacket) {
    // parse state machine
    enum class ParseState {
        Request,
        Header,
        Body,
        Done,
        BadRequest,
    };

    TRACE();
    HttpRequest request {};
    request.mIsKeepAlive = false;
    ParseState state = ParseState::Request;
    std::string_view::size_type start_pos = 0, end_pos = 0;

    // start parse request
    while (true) {
        switch (state) {
            case ParseState::Request:
            {
                start_pos = 0;
                end_pos = rawHttpPacket.find(CRLF);
                if (end_pos == std::string_view::npos) {
                    LOG_ERR("{}: parse request error.", __FUNCTION__);
                    state = ParseState::BadRequest;
                    break;
                }
                auto requestLine = std::string(rawHttpPacket.data(), end_pos - start_pos);
                if (!parseStartLine(requestLine, request)) {
                    LOG_ERR("{}: parse request error.", __FUNCTION__);
                    state = ParseState::BadRequest;
                    break;
                }
                state = ParseState::Header;
                LOG_DEBUG("{}: parse request complete, then parse headers", __FUNCTION__);
                break;
            }
            case ParseState::Header:
            {
                // We hope all input line like (key:value CRLF)
                // Make sure start_pos not CRLF.
                start_pos = end_pos + CRLF.size();
                if (start_pos == std::string_view::npos) {
                    LOG_DEBUG("{}: parse headers complete", __FUNCTION__);
                    state = ParseState::Done;
                    break;
                }
                // end_pos is the ended CRLF of headers.
                end_pos = rawHttpPacket.find(BLANK_LINE, start_pos);
                if (end_pos == std::string_view::npos) {
                    LOG_ERR("{}: parse header error. BLANK_LINE not found.", __FUNCTION__);
                    state = ParseState::BadRequest;
                    break;
                }
                end_pos += CRLF.size(); // use CRLF as end indicator.
                std::string headers { rawHttpPacket.begin() + start_pos, end_pos - start_pos };
                if (parseHeaders(headers, request) == false) {
                    LOG_ERR("{}: parse header error. ", __FUNCTION__);
                    state = ParseState::BadRequest;
                    break;
                }
                LOG_DEBUG("{}: parse headers complete, then parse body", __FUNCTION__);
                state = ParseState::Body;
                break;
            }
            case ParseState::Body:
            {
                // Skip blank line.
                // Make sure start_pos not CRLF.
                start_pos = end_pos + CRLF.size();
                end_pos = rawHttpPacket.size();
                if (request.mType == RequestType::GET) {
                    LOG_DEBUG("{}: Accept Get request, ignore size check.", __FUNCTION__);
                } else {
                    auto content_length = end_pos - start_pos;
                    if (content_length != request.mContentLength) {
                        LOG_DEBUG("{}: Bad content length. Actually accepted size:{}, Expect size:{}"
                                , __FUNCTION__, content_length, request.mContentLength);
                        state = ParseState::BadRequest;
                        break;
                    }
                }
                request.mBody = std::string { rawHttpPacket.begin() + start_pos, end_pos - start_pos };
                request.mRequestSize = rawHttpPacket.size();
                state = ParseState::Done;
                LOG_DEBUG("{}: parse body complete", __FUNCTION__);
                break;
            }
            case ParseState::Done:
                LOG_INFO("{}: parse request success", __FUNCTION__);
                return request;
            case ParseState::BadRequest:
            default:
                LOG_ERR("{}: BadRequest!", __FUNCTION__);
                throw RequestError {"[HttpRequest] Parse error", RequestErrorType::BadRequest};
        }
    }
}

void dumpHttpRequest(const HttpRequest& request) {
    LOG_DEBUG("{}: {}", __FUNCTION__, to_string_view(request.mType));
    LOG_DEBUG("{}: {}", __FUNCTION__, to_string_view(request.mVersion));
    LOG_DEBUG("{}: {}", __FUNCTION__, request.mUrl);
    LOG_DEBUG("{}: {}", __FUNCTION__, to_string_view(request.mContentType));
    LOG_DEBUG("{}: {}", __FUNCTION__, request.mContentLength);
    LOG_DEBUG("{}: {}", __FUNCTION__, request.mHost);
    LOG_DEBUG("{}: {}", __FUNCTION__, request.mBody);
    LOG_DEBUG("{}: keep-alive: {}", __FUNCTION__, request.mIsKeepAlive);
    LOG_DEBUG("{}: size: {}", __FUNCTION__, request.mRequestSize);
    for (auto&& encodeType : request.mAcceptEncodings) {
        LOG_DEBUG("{}: accept encoding {}", __FUNCTION__, to_string_view(encodeType));
    }
    for (auto&& header : request.mHeaders) {
        LOG_DEBUG("{}: key:{}, value:{}", __FUNCTION__, header.first, header.second);
    }

    std::cout << "[REQUEST] method: " << to_string_view(request.mType) << std::endl;
    std::cout << "[REQUEST] version: " << to_string_view(request.mVersion) << std::endl;
    std::cout << "[REQUEST] url: " << request.mUrl << std::endl;
    std::cout << "[REQUEST] content_type: " << to_string_view(request.mContentType) << std::endl;
    std::cout << "[REQUEST] content_length: " << request.mContentLength << std::endl;
    std::cout << "[REQUEST]" << request.mHost << std::endl;
    for (auto&& encodeType : request.mAcceptEncodings) {
        std::cout << "[REQUEST] accept encoding:" << to_string_view(encodeType) << std::endl;
    }
    for (auto&& header : request.mHeaders) {
        std::cout << "[HEADERS] key:" << header.first << ", value:" << header.second << std::endl;
    }
    std::cout << "[BODY] body:" << request.mBody << std::endl;
    std::cout << "[SIZE] total size:" << request.mRequestSize << std::endl;
}

} // namespace simpletcp::http




