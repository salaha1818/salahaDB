#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct Command {
    std::string type;
    std::vector<std::string> arguments;
};