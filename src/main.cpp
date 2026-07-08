#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "Record.h"
#include "Parser.h"
#include "Command.h"
#include "Database.h"

int main() {
    Database database("salaha.db");
    Parser parser;
    Command cmd;
    std::string input;

    int mode;

    std::cout << "1 - Interactive (Terminal Based)" << std::endl << "2 - Program (File Based)" << std::endl;
    std::cout << "Select a mode: ";
    std::cin >> mode;

    if (mode == 1) {
        while (true) {
            std::cout << "> ";
            std::getline(std::cin, input);

            if (input == "exit") {
                break;
            }

            if (parser.parse(input, cmd)) {
                database.execute(cmd);
            } else {
                std::cout << "Invalid command." << std::endl;
            }
        }
    } 
    
    else if (mode == 2) {
        std::ifstream file("code.txt");

        if (!file) {
            std::cout << "Could not open code.txt\n";
            return 1;
        }

        while (std::getline(file, input)) {
            std::cout << "> " << input << std::endl;

            if (parser.parse(input, cmd)) {
                database.execute(cmd);
            } else {
                std::cout << "Invalid command.\n";
            }
        }
    }

    return 0;
}