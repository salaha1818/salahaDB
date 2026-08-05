#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include "Command.h"
#include "Parser.h"
#include <cctype>

// converts a string to uppercase so SQL keywords are case-insensitive
std::string upperCase(std::string s) {
    for (char& c : s) {
        c = std::toupper(static_cast<unsigned char>(c));
    }
    return s;
}

// converts a string to lowercase for comparisons
std::string lowerCase(std::string s) {
    for (char& c : s) {
        c = std::tolower(static_cast<unsigned char>(c));
    }
    return s;
}

bool Parser::parse(std::string input, Command& cmd) {
    // clear the previous command
    cmd = {};

    std::vector<std::string> tokens;
    std::istringstream iss(input);
    std::string token;

    // split the input into tokens
    while (iss >> token) {
        tokens.push_back(token);
    }

    if (tokens.empty()) {
        return false;
    }

    // use the first token to determine the command type
    std::string commandType = upperCase(tokens[0]);

    if (commandType == "SELECT") {
        return parseSelect(tokens, cmd);
    }

    if (commandType == "INSERT") {
        return parseInsert(tokens, cmd);
    }

    if (commandType == "UPDATE") {
        return parseUpdate(tokens, cmd);
    }

    if (commandType == "DELETE") {
        return parseDelete(tokens, cmd);
    }

    // FIND <id>
    if (commandType == "FIND") {
        if (tokens.size() != 2) {
            return false;
        }

        cmd.type = "find";
        cmd.arguments = {tokens[1]};
        return true;
    }

    if (commandType == "DROP") {
        return parseDrop(tokens, cmd);
    }

    // commands with no arguments
    if (commandType == "VACUUM" ||
        commandType == "STATS" ||
        commandType == "HELP" ||
        commandType == "EXIT") {

        if (tokens.size() != 1) {
            return false;
        }

        cmd.type = lowerCase(tokens[0]);
        cmd.arguments.clear();
        return true;
    }

    return false;
}

bool Parser::parseInsert(const std::vector<std::string>& tokens, Command& cmd) {
    // INSERT INTO records VALUES <name> <age>
    if (tokens.size() != 6) {
        return false;
    }

    if (upperCase(tokens[0]) != "INSERT" ||
        upperCase(tokens[1]) != "INTO" ||
        lowerCase(tokens[2]) != "records" ||
        upperCase(tokens[3]) != "VALUES") {

        return false;
    }

    // insert expects: name, age
    cmd.type = "insert";
    cmd.arguments = {tokens[4], tokens[5]};

    return true;
}

bool Parser::parseSelect(
    const std::vector<std::string>& tokens,
    Command& cmd
) {
    if (tokens.size() < 4) {
        return false;
    }

    // every select starts with SELECT * FROM records
    if (upperCase(tokens[0]) != "SELECT" ||
        tokens[1] != "*" ||
        upperCase(tokens[2]) != "FROM" ||
        lowerCase(tokens[3]) != "records") {
        return false;
    }

    cmd.type = "select";

    // SELECT * FROM records
    if (tokens.size() == 4) {
        cmd.arguments.clear();
        return true;
    }

    // SELECT * FROM records ORDER BY age|name
    if (tokens.size() == 7 &&
        upperCase(tokens[4]) == "ORDER" &&
        upperCase(tokens[5]) == "BY") {

        std::string category = lowerCase(tokens[6]);

        if (category == "age" ||
            category == "name") {

            // select expects: sort, category
            cmd.arguments = {
                "sort",
                category
            };

            return true;
        }
    }

    // SELECT * FROM records WHERE age|name = value
    if (tokens.size() == 8 &&
        upperCase(tokens[4]) == "WHERE" &&
        tokens[6] == "=") {

        std::string category =
            lowerCase(tokens[5]);

        if (category == "age" ||
            category == "name") {

            // select expects: category, value
            cmd.arguments = {
                category,
                tokens[7]
            };

            return true;
        }
    }

    // SELECT * FROM records WHERE id BETWEEN min AND max
    if (tokens.size() == 10 &&
        upperCase(tokens[4]) == "WHERE" &&
        lowerCase(tokens[5]) == "id" &&
        upperCase(tokens[6]) == "BETWEEN" &&
        upperCase(tokens[8]) == "AND") {

        // select expects: id, min, max
        cmd.arguments = {
            "id",
            tokens[7],
            tokens[9]
        };

        return true;
    }

    return false;
}

bool Parser::parseUpdate(
    const std::vector<std::string>& tokens,
    Command& cmd
) {
    // UPDATE records SET name = <name> age = <age> WHERE id = <id>
    if (tokens.size() != 13) {
        return false;
    }

    if (upperCase(tokens[0]) != "UPDATE" ||
        lowerCase(tokens[1]) != "records" ||
        upperCase(tokens[2]) != "SET" ||
        lowerCase(tokens[3]) != "name" ||
        tokens[4] != "=" ||
        lowerCase(tokens[6]) != "age" ||
        tokens[7] != "=" ||
        upperCase(tokens[9]) != "WHERE" ||
        lowerCase(tokens[10]) != "id" ||
        tokens[11] != "=") {

        return false;
    }

    // update expects: id, new name, new age
    cmd.type = "update";
    cmd.arguments = {
        tokens[12], // ID
        tokens[5],  // new name
        tokens[8]   // new age
    };

    return true;
}

bool Parser::parseDelete(const std::vector<std::string>& tokens, Command& cmd) {
    // DELETE FROM records WHERE id = <id>
    if (tokens.size() != 7) {
        return false;
    }

    if (upperCase(tokens[0]) != "DELETE" ||
        upperCase(tokens[1]) != "FROM" ||
        lowerCase(tokens[2]) != "records" ||
        upperCase(tokens[3]) != "WHERE" ||
        lowerCase(tokens[4]) != "id" ||
        tokens[5] != "=") {

        return false;
    }

    // delete expects: id
    cmd.type = "delete";
    cmd.arguments = {tokens[6]};

    return true;
}

bool Parser::parseDrop(const std::vector<std::string>& tokens, Command& cmd) {
    // DROP DATABASE CONFIRM
    if (tokens.size() != 3) {
        return false;
    }

    // require CONFIRM so the database isn't dropped by accident
    if (upperCase(tokens[0]) != "DROP" ||
        upperCase(tokens[1]) != "DATABASE" ||
        upperCase(tokens[2]) != "CONFIRM") {
        return false;
    }

    cmd.type = "drop";
    return true;
}