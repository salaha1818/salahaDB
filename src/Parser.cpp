#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "Command.h"
#include "Parser.h"

bool Parser::parse(std::string input, Command& cmd) {
    cmd = {};
    bool valid = false;
    std::string type;
    std::vector<std::string> arguments;
    std::istringstream iss(input);
    std::string token;

    iss >> type;
    while (iss >> token) {
        arguments.push_back(token);
    }

    if((type == "update" && arguments.size() == 3) || (type == "insert" && arguments.size() == 2) || (type == "delete" && arguments.size() == 1) || ((type == "select" || type == "help" || type == "exit") && arguments.size() == 0) || (type == "select" && (arguments.size() == 0 || arguments.size() == 2)) || (type == "find" && arguments.size() == 1)){
        valid = true;
        cmd.type = type;
        cmd.arguments = arguments;
    }
    return valid;
}
