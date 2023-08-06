#include "base/StringHelper.h"
#include <iostream>

int main() {
    std::string input = " Test Test Test Test Test";
    auto result = simpletcp::utils::split(input, " ");
    for (auto&& word : result) {
        if (word != "Test") {
            std::cerr << "[StringHelperTest] split failed!" << std::endl;
            return 1;
        }
    }

    input = " Test ";
    input = simpletcp::utils::strip(input);
    if (input != "Test") {
        std::cerr << "[StringHelperTest] strip failed!" << input << std::endl;
        return 1;
    }
    std::cerr << "[StringHelperTest] success" << input << std::endl;
    return 0;
}
