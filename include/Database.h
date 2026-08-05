#pragma once

#include <string>
#include "StorageEngine.h"
#include "StorageEngine.h"
#include <vector>
#include <cstdint>
#include "Record.h"
#include "Command.h"
#include "PageManager.h"
#include "BPlusTree.h"
#include <unordered_map>
#include "RecordLocation.h"

// handles commands, records, indexes, and database storage
class Database {
private:
    // id that will be assigned to the next inserted record
    uint32_t nextID = 1;

    // manages the pages that store the records
    PageManager pageManager;

    // command functions
    void insert(const std::vector<std::string>& arguments);
    void deleteRecord(const std::vector<std::string>& arguments);
    void select(const std::vector<std::string>& arguments) const;
    void find(const std::vector<std::string>& arguments) const;
    void help() const;
    void vacuum();
    void drop();
    void stats() const;
    void updateRecord(const std::vector<std::string>& arguments);

    // adds a record back into all indexes
    void reIndex(
        const Record& record,
        size_t pageIndex,
        size_t slotIndex
    );

    // handles reading and writing the database file
    StorageEngine storage;

    // sorting functions used by SELECT ORDER BY
    void sortAge(std::vector<Record>& records) const;
    void sortName(std::vector<Record>& records) const;

    // displays one record
    void printRecord(const Record& record) const;

    // removes ids from the secondary indexes
    void removeIdFromAge(int age, int id);
    void removeIdFromName(std::string& name, int id);

    // primary index maps ids to page and slot locations
    BPlusTree primaryIndex;

    // secondary indexes map names and ages to record ids
    std::unordered_map<std::string, std::vector<int>> names;
    std::unordered_map<int, std::vector<int>> ages;

public:
    // runs the command created by the parser
    void execute(const Command& cmd);

    // opens or creates the database file
    Database(const std::string& filename);

    // loads and saves all database pages
    void load();
    void save();
};