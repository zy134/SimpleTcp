#include "base/Log.h"
#include "base/Jthread.h"
#include <vector>
#include <chrono>
#include <iostream>

constexpr auto TAG = "LogBench";
using namespace simpletcp;
using namespace std::chrono;

int main() {
    std::vector<simpletcp::utils::Jthread> vec;
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i != 4; ++i) {
        vec.emplace_back([] {
            for (int j = 0; j != 250'000; ++j) {
                LOG_INFO("test");
            }
        });
    }
    vec.clear();
    auto end = steady_clock::now();
    std::cout << "Total time: " << duration_cast<milliseconds>(end - start).count() << "ms" << std::endl;
}
