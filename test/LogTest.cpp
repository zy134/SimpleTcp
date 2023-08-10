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
    for (auto i = 0; i != 4; ++i) {
        vec.emplace_back([&] {
            for (int j = 0; j != 1'000'000; ++j) {
                LOG_INFO("test log Info, index {}", j);
            }
        });
    }
    vec.clear();
}

int main() {
    LOG_INFO("test log start");
    bench();
    LOG_ERR("test log end");
}
