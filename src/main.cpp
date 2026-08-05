#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <limits>
#include "Record.h"
#include "Parser.h"
#include "Command.h"
#include "Database.h"

// checks if the input only contains spaces or tabs
bool isBlank(const std::string& s) {
    return s.find_first_not_of(" \t\r\n") == std::string::npos;
}

int main() {
    // create the database, parser, and command objects
    Database database("salaha.db");
    Parser parser;
    Command cmd;
    std::string input;

    int mode;

    // show the available input modes
    std::cout
        << "\n========== salahaDB ==========\n"
        << "1 - Interactive Mode\n"
        << "2 - Script Mode\n\n"
        << "Select a mode: ";

    // stop if the mode is not a number
    if (!(std::cin >> mode)) {
        std::cerr << "Invalid mode selection.\n";
        return 1;
    }

    // remove the leftover newline before using getline
    std::cin.ignore(
        std::numeric_limits<std::streamsize>::max(),
        '\n'
    );

    // interactive mode reads commands from the terminal
    if (mode == 1) {
        std::cout
            << "\nInteractive mode started.\n"
            << "Type HELP to view available commands.\n\n";

        while (true) {
            std::cout << "salahaDB> ";

            // exit cleanly if the input stream closes
            if (!std::getline(std::cin, input)) {
                std::cout << "\nExiting salahaDB.\n";
                break;
            }

            // ignore empty lines
            if (isBlank(input)) {
                continue;
            }

            // reject commands that do not match the parser formats
            if (!parser.parse(input, cmd)) {
                std::cout << "Invalid command.\n";
                continue;
            }

            // exit is handled here instead of inside Database
            if (cmd.type == "exit") {
                std::cout << "Exiting salahaDB.\n";
                break;
            }

            // run the parsed command
            database.execute(cmd);

            // separate the output from the next prompt
            std::cout << '\n';
        }
    }

    // script mode reads commands from code.txt
    else if (mode == 2) {
        std::ifstream file("code.txt");

        if (!file) {
            std::cerr << "Error: Could not open code.txt.\n";
            return 1;
        }

        std::cout << "\nExecuting commands from code.txt...\n\n";

        // read and run one command at a time
        while (std::getline(file, input)) {
            // ignore empty lines in the script
            if (isBlank(input)) {
                continue;
            }

            // show the command before running it
            std::cout << "salahaDB> " << input << '\n';

            // reject invalid commands and continue to the next line
            if (!parser.parse(input, cmd)) {
                std::cout << "Invalid command.\n\n";
                continue;
            }

            // stop reading the script when EXIT is reached
            if (cmd.type == "exit") {
                std::cout << "Program execution completed.\n";
                break;
            }

            // run the parsed command
            database.execute(cmd);

            // separate each command's output from the next command
            std::cout << '\n';
        }
    }

    // reject any mode other than 1 or 2
    else {
        std::cerr << "Invalid mode. Select 1 or 2.\n";
        return 1;
    }

    return 0;
}