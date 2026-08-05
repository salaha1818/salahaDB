#pragma once

#include <string>
#include <vector>

// stores the parsed command and its arguments
struct Command {
    std::string type;
    std::vector<std::string> arguments;
};