#pragma once

#include <cstdint>
#include <string>
#include "command.h"

class Parser {
    private:

    public:
        // turns a full input line into a command
        bool parse(std::string input, Command& cmd);

        // checks the INSERT command format
        bool parseInsert(
            const std::vector<std::string>& tokens,
            Command& cmd
        );

        // checks each supported SELECT format
        bool parseSelect(
            const std::vector<std::string>& tokens,
            Command& cmd
        );

        // checks the UPDATE command format
        bool parseUpdate(
            const std::vector<std::string>& tokens,
            Command& cmd
        );

        // checks the DELETE command format
        bool parseDelete(
            const std::vector<std::string>& tokens,
            Command& cmd
        );

        // checks DROP DATABASE CONFIRM
        bool parseDrop(
            const std::vector<std::string>& tokens,
            Command& cmd
        );
};