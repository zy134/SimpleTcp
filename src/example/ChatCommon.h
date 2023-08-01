#include "base/Log.h"
#include "net/Socket.h"
#include <cstdint>
#include <cstring>
#include <string>
#include <iostream>
#include <string_view>

inline const simpletcp::net::SocketAddr chatServerAddr {
    .mIpAddr = "127.0.0.1",
    .mIpProtocol = simpletcp::net::IP_PROTOCOL::IPv4,
    .mPort = 8848
};

enum RequestType : uint64_t {
    Message = 0xAAAAAA,
    Register = 0xBBBBBB,
    Unregister = 0xCCCCCC
};

struct RequestHdr {
    uint64_t    mReqLength;
    RequestType mReqType;
};

static_assert(sizeof(RequestHdr) == 16, "Bad RequestHdr type!");

inline std::string normalRequest(std::string_view sv) {
    std::string result;
    RequestHdr hdr {
        hdr.mReqLength = sv.size() + sizeof(RequestHdr),
        hdr.mReqType = RequestType::Message
    };
    result.resize(sv.size() + sizeof(RequestHdr));
    std::memcpy(result.data(), reinterpret_cast<char *>(&hdr), sizeof(RequestHdr));
    std::memcpy(result.data() + sizeof(RequestHdr), sv.data(), sv.size());
    simpletcp::assertTrue(result.size() >= sizeof(RequestHdr), "[ChatCommon] Bad request!");
    return result;
}

inline std::string registerRequest(std::string_view sv) {
    std::string result;
    RequestHdr hdr {
        hdr.mReqLength = sv.size() + sizeof(RequestHdr),
        hdr.mReqType = RequestType::Register
    };
    result.resize(sv.size() + sizeof(RequestHdr));
    std::memcpy(result.data(), reinterpret_cast<char *>(&hdr), sizeof(hdr));
    std::memcpy(result.data() + sizeof(RequestHdr), sv.data(), sv.size());
    simpletcp::assertTrue(result.size() >= sizeof(RequestHdr), "[ChatCommon] Bad request!");
    return result;
}

inline std::string unregisterRequest(std::string_view sv) {
    std::string result;
    RequestHdr hdr {
        hdr.mReqLength = sv.size() + sizeof(RequestHdr),
        hdr.mReqType = RequestType::Unregister
    };
    result.resize(hdr.mReqLength);
    std::memcpy(result.data(), reinterpret_cast<unsigned char *>(&hdr), sizeof(RequestHdr));
    std::memcpy(result.data(), sv.data(), sv.size());
    simpletcp::assertTrue(result.size() >= sizeof(RequestHdr), "[ChatCommon] Bad request!");
    return result;
}



