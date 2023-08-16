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

int main() {
    LOG_VER("ver");
    LOG_DEBUG("debug");
    LOG_INFO("info");
    LOG_WARN("warn");
    LOG_ERR("error");
    LOG_FATAL("fatal");
}
