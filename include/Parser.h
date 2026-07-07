#pragma once

#include <cstdint>
#include <string>
#include "command.h"

class Parser {
    private:

    public:
        bool parse(std::string input, Command& cmd);
};