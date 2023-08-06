#include "base/Log.h"
#include "base/StringHelper.h"
#include "http/HttpError.h"
#include "http/HttpRequest.h"
#include <pstl/execution_defs.h>
#include <string_view>
#include <iostream>

static constexpr std::string_view TAG = "HttpRequest";

static constexpr std::string_view CRLF = "\r\n";

static constexpr std::string_view BLANK_LINE = "\r\n\r\n";

namespace simpletcp::http {

static bool parseHttpRequestLine(std::string_view requestLine, HttpRequest& request) {
    TRACE();
    if (requestLine.empty()) {
        LOG_ERR("{}: parse error, empty request!", __FUNCTION__);
        return false;
    }
    const auto parser = [&] {
        if (requestLine.find("HTTP/1.1") != std::string_view::npos) {
            request.mVersion = HttpRequestVersion::HTTP1_1;
        } else if (requestLine.find("HTTP/1.0") != std::string_view::npos) {
            request.mVersion = HttpRequestVersion::HTTP1_0;
        } else {
            LOG_ERR("parseHttpRequestLine: parse error, unknown http version!");
            return false;
        }
        auto url_start = requestLine.find('/');
        auto url_end = requestLine.find("HTTP");
        if (url_start == std::string_view::npos) {
            LOG_ERR("parseHttpRequestLine: parse error, bad request!");
            return false;
        }
        url_start += 1;
        request.mUrl = std::string { requestLine.data() + url_start, url_end - url_start };
        return true;
    };
    if (requestLine.find("GET") != std::string_view::npos) {
        request.mType = HttpRequestType::GET;
        if (!parser())
            return false;
    } else if (requestLine.find("POST") != std::string_view::npos) {
        request.mType = HttpRequestType::POST;
        if (!parser())
            return false;
    } else {
        LOG_ERR("{}: parse error, unknown type request!", __FUNCTION__);
        return false;
    }
    return true;
}

static bool parseHttpHeaders(std::string_view headers, HttpRequest& request) {
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
            value = utils::strip(value);
            request.mHeaders.insert({ std::move(key), std::move(value) });
        }

        start_pos = end_pos + CRLF.size();
        end_pos = headers.find(CRLF, start_pos);
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
    HttpRequest request;
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
                if (!parseHttpRequestLine(requestLine, request)) {
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
                if (parseHttpHeaders(headers, request) == false) {
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
                // Empty body.
                if (start_pos == rawHttpPacket.size()) {
                    LOG_DEBUG("{}: empty body, start_pos = {}", __FUNCTION__, start_pos);
                    state = ParseState::Done;
                    request.mRequestSize = rawHttpPacket.size();
                    break;
                } else if (start_pos > rawHttpPacket.size()) {
                    LOG_ERR("{}: request not ended with CRLF", __FUNCTION__);
                    state = ParseState::BadRequest;
                    break;
                }
                // Append body.
                end_pos = rawHttpPacket.find(CRLF, start_pos);
                if (end_pos == std::string_view::npos) {
                    state = ParseState::BadRequest;
                    break;
                }
                request.mBody = std::string { rawHttpPacket.begin() + start_pos, end_pos - start_pos };
                request.mRequestSize = end_pos + CRLF.size();
                state = ParseState::Done;
                LOG_DEBUG("{}: parse body complete", __FUNCTION__);
                break;
            }
            case ParseState::Done:
                LOG_INFO("{}: parse request success", __FUNCTION__);
                if (auto iter = request.mHeaders.find("Connections");
                        iter != request.mHeaders.end() && iter->second == "keep-alive") {
                    request.mIsKeepAlive = true;
                }
                return request;
            case ParseState::BadRequest:
            default:
                LOG_ERR("{}: BadRequest!", __FUNCTION__);
                throw HttpException {"[HttpRequest] Parse error", HttpErrorType::BadRequest};
        }
    }
}

void dumpHttpRequest(const HttpRequest& request) {
    LOG_DEBUG("{}: {}", __FUNCTION__, to_string_view(request.mType));
    LOG_DEBUG("{}: {}", __FUNCTION__, to_string_view(request.mVersion));
    LOG_DEBUG("{}: {}", __FUNCTION__, request.mUrl);
    for (auto&& header : request.mHeaders) {
        LOG_DEBUG("{}: key:{}, value:{}", __FUNCTION__, header.first, header.second);
    }
    LOG_DEBUG("{}: {}", __FUNCTION__, request.mBody);
    LOG_DEBUG("{}: keep-alive: {}", __FUNCTION__, request.mIsKeepAlive);
    LOG_DEBUG("{}: size: {}", __FUNCTION__, request.mRequestSize);

    std::cout << "[REQUEST]" << to_string_view(request.mType) << std::endl;
    std::cout << "[REQUEST]" << to_string_view(request.mVersion) << std::endl;
    std::cout << "[REQUEST]" << request.mUrl << std::endl;
    for (auto&& header : request.mHeaders) {
        std::cout << "[HEADERS] key:" << header.first << ", value:" << header.second << std::endl;
    }
    std::cout << "[BODY]" << request.mBody << std::endl;
    std::cout << "[SIZE]" << request.mRequestSize << std::endl;
}

} // namespace simpletcp::http




