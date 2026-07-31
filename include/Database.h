#pragma once

#include <string>
#include "StorageEngine.h"
#include "StorageEngine.h"
#include <vector>
#include <cstdint>
#include "Record.h"
#include "Command.h"
#include "PageManager.h"
#include <unordered_map>

struct RecordLocation {
        size_t pageIndex;
        size_t slotIndex;
};

class Database {
private:
    uint32_t nextID = 1;
    PageManager pageManager;
    void insert(const std::vector<std::string>& arguments);
    void deleteRecord(const std::vector<std::string>& arguments);
    void select(const std::vector<std::string>& arguments) const;
    void find(const std::vector<std::string>& arguments);
    void help() const;
    void compact();
    void stats() const;
    void reIndex(const Record& record, size_t pageIndex, size_t slotIndex);
    StorageEngine storage;
    void updateRecord(const std::vector<std::string>& arguments);
    void sortAge(std::vector <Record>& records) const;
    void sortName(std::vector <Record>& records) const;
    void printRecord(const Record& record) const;
    void removeIdFromAge(int age, int id);
    void removeIdFromName(std::string& name, int id);
    std::unordered_map<int, RecordLocation> indices;
    std::unordered_map<std::string, std::vector<int>> names;
    std::unordered_map<int, std::vector<int>> ages;
public:
    void execute(const Command& cmd);
    Database(const std::string& filename);
    void load();
    void save();
};