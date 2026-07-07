#pragma once

#include <vector>
#include <cstdint>
#include "Record.h"
#include "Command.h"

class Database {
private:
    std::vector<Record> records;
    uint32_t nextID = 1;
    void insert(const std::vector<std::string>& arguments);
    void deleteRecord(const std::vector<std::string>& arguments);
    void select() const;

public:
    void execute(const Command& cmd);
};