#pragma once

#include <string>
#include "StorageEngine.h"
#include "StorageEngine.h"
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
    StorageEngine storage;
    void updateRecord(const std::vector<std::string>& arguments);
public:
    void execute(const Command& cmd);
    Database(const std::string& filename);
    void load();
    void save();
};