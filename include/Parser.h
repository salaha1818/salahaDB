#pragma once

#include <cstdint>
#include <string>
#include "command.h"

class Parser {
    private:

    public:
        bool parse(std::string input, Command& cmd);
        bool parseInsert(const std::vector<std::string>& tokens, Command& cmd);
        bool parseSelect(const std::vector<std::string>& tokens, Command& cmd);
        bool parseUpdate(const std::vector<std::string>& tokens, Command& cmd);
        bool parseDelete(const std::vector<std::string>& tokens, Command& cmd);
    };