#include "base/Log.h"
#include "base/Jthread.h"
#include <bits/types/time_t.h>
#include <chrono>
#include <cmath>
#include <ctime>
#include <ratio>
#include <vector>
#include <iostream>

inline static constexpr std::string_view TAG = "LogTest";

using namespace std;
using namespace simpletcp;

void bench() {
    std::vector<utils::Jthread> vec;
    auto start = chrono::steady_clock::now();
    for (auto i = 0; i != 4; ++i) {
        vec.emplace_back([&] {
            for (int j = 0; j != 1'000; ++j) {
                LOG_INFO("test log Info, index {}", j);
            }
        });
    }
    vec.clear();
    auto end = chrono::steady_clock::now();
    std::cout << "[LogTest] total runtime : "
        << chrono::duration_cast<chrono::milliseconds>(end - start).count() << "ms" << std::endl;
}

int main() {
    LOG_INFO("log Info start");

    auto start = chrono::steady_clock::now();
    LOG_INFO("test log Info");
    auto end = chrono::steady_clock::now();
    std::cout << "[LogTest] single log runtime : "
        << chrono::duration_cast<chrono::nanoseconds>(end - start).count() << "ns" << std::endl;

    start = chrono::steady_clock::now();
    LOG_INFO("test log Info");
    end = chrono::steady_clock::now();
    std::cout << "[LogTest] single log runtime : "
        << chrono::duration_cast<chrono::nanoseconds>(end - start).count() << "ns" << std::endl;

    LOG_ERR("test log error");
}
