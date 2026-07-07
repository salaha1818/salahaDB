#include <iostream>
#include "Record.h"
#include "Parser.h"
#include "Command.h"

int main() {
    std::string input = "exit";
    Parser parser;
    Command cmd;
    bool valid = parser.parse(input, cmd);
    if(valid){
        std::cout << cmd.type << std::endl;
        for (const auto& element : cmd.arguments) {
            std::cout << element << std::endl;
        }
    } else{
        std::cout << "error" << std::endl;
    }

    return 0;
}