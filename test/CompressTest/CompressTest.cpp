#include "base/Log.h"
#include "base/Compress.h"
#include <iostream>

using namespace simpletcp;
using namespace simpletcp::utils;

int main() {
    std::string input {
        "hello hello hello hello hello"\
        "world world world world world"\
        "one two three one two three"\
        "hello hello hello hello hello"\
        "world world world world world"\
        "one two three one two three"\
        "world world world world world"\
        "hello hello hello hello hello"\
        "one two three one two three"\
        "world world world world world"\
        "hello hello hello hello hello"
        "one two three one two three"\
    };
    auto medium = compress_gzip(input);
    auto output = uncompress_gzip(medium, input.size());
    std::cout << "[CompressTest] before compress length:" << input.size()
                << ", after compress length:" << medium.size() << std::endl;
    if (input.size() == output.size()) {
        std::cout << "[CompressTest] test gzip success" << std::endl;
    }
    
    medium = compress_deflate(input);
    output = uncompress_deflate(medium, input.size());
    std::cout << "[CompressTest] before compress length:" << input.size()
                << ", after compress length:" << medium.size() << std::endl;;
    if (input.size() == output.size()) {
        std::cout << "[CompressTest] test deflate success" << std::endl;
    }
}
