#include "base/Log.h"
#include "base/Jthread.h"
#include "base/LogConfig.h"
#include <vector>
#include <chrono>
#include <iostream>
#include <iomanip>

constexpr auto TAG = "LogBench";
using namespace simpletcp;
using namespace std::chrono;

void multi_thread() {
    std::cout << "[LogBench] Test for 8 threads, each threads would write 1'000'000 logs." << std::endl;
    std::vector<simpletcp::utils::Jthread> vec;
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i != 8; ++i) {
        vec.emplace_back([] {
            for (int j = 0; j != 1'000'000; ++j) {
                LOG_INFO("test");
            }
        });
    }
    vec.clear();
    auto end = steady_clock::now();
    auto totalTime = duration_cast<milliseconds>(end - start).count();
    std::cout << "[LogBench] Total time: " << totalTime << "ms" << std::endl;
    double logPerSecond = static_cast<double>(8'000'000) / static_cast<double>(totalTime) * 1000;
    std::cout << std::setprecision(12) << "[LogBench] speed: " << logPerSecond << "/sec" << std::endl;

}

void single_thread() {
    std::cout << "[LogBench] Test in single thread, would write 1'000'000 logs." << std::endl;
    auto start = std::chrono::steady_clock::now();
    for (int j = 0; j != 1'000'000; ++j) {
        LOG_INFO("test");
    }
    auto end = steady_clock::now();
    auto totalTime = duration_cast<milliseconds>(end - start).count();
    std::cout << "[LogBench] Total time: " << totalTime << "ms" << std::endl;
    double logPerSecond = static_cast<double>(1'000'000) / static_cast<double>(totalTime) * 1000;
    std::cout << std::setprecision(12) << "[LogBench] speed: " << logPerSecond << "/sec" << std::endl;
}

int main() {
    LOG_INFO("LogBench start");
    std::cout << "[LogBench] default log buffer size:" << LOG_BUFFER_SIZE << std::endl;
    std::cout << "[LogBench] default log file size:" << LOG_MAX_FILE_SIZE << std::endl;
    single_thread();
    multi_thread();
    LOG_INFO("LogBench end");
}
