#define BPLUSTREE_H

#include <iostream>
#include <string>
#include <vector>
#include "Command.h"
#include "Parser.h"
#include "Page.h"
#include "PageManager.h"
#include "Database.h"
#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <optional>

int toInt(std::string value);

Database::Database(const std::string& filename)
    : storage(filename)
{
    load();
}

void Database::load() {
    primaryIndex.clear();
    ages.clear();
    names.clear();
    pageManager.clear();

    // Load pages from disk and give them to PageManager
    std::vector<Page> pages = storage.loadPages();
    pageManager.loadPages(pages);

    uint32_t largestID = 0;
    bool hasRecords = false;

    const std::vector<Page>& loadedPages = pageManager.getPages();

    // Rebuild indexes from the loaded pages
    for (size_t pageIndex = 0;
         pageIndex < loadedPages.size();
         pageIndex++) {

        const std::vector<Record>& pageRecords =
            loadedPages[pageIndex].getRecords();

        for (size_t slotIndex = 0;
             slotIndex < pageRecords.size();
             slotIndex++) {

            const Record& record = pageRecords[slotIndex];

            reIndex(record, pageIndex, slotIndex);

            if (record.id > largestID) {
                largestID = record.id;
            }

            hasRecords = true;
        }
    }

    nextID = hasRecords ? largestID + 1 : 1;
}

void Database::reIndex(const Record& record, size_t pageIndex, size_t slotIndex) {
    if (record.deleted) {
        return;
    }

    RecordLocation location{pageIndex, slotIndex};

    primaryIndex.insert(record.id, location);
    ages[record.age].push_back(record.id);
    names[record.name].push_back(record.id);
}

void Database::save(){
    storage.savePages(pageManager.getPages());
}

void Database::execute(const Command& cmd){
    if(cmd.type == "insert"){
        insert(cmd.arguments);
    } else if(cmd.type == "delete"){
        deleteRecord(cmd.arguments);
    } else if(cmd.type == "select"){
        select(cmd.arguments);
    } else if (cmd.type == "update") {
        updateRecord(cmd.arguments);
    } else if (cmd.type == "find") {
        find(cmd.arguments);
    } else if (cmd.type == "help") {
        help();
    } else if (cmd.type == "compact") {
        compact();
    } else if (cmd.type == "stats") {
        stats();
    } else {}
}

void Database::stats() const {
    size_t totalRecordCount = 0;
    size_t activeRecordCount = 0;
    size_t deletedRecordCount = 0;
    size_t pageCount = 0;
    
    const std::vector <Page>& pages = pageManager.getPages();
    pageCount = pages.size();

    size_t databaseSize = sizeof(StorageEngine::DatabaseHeader) + pageCount * Page::maxSize;

    for(const auto& page:pages) {
        const std::vector<Record>& records = page.getRecords();
        for(const auto& record:records) {
            if (record.deleted) {
                deletedRecordCount++;
            } else {
                activeRecordCount++;
            }
            totalRecordCount++;
        }
    }

    std::cout
        << "\n=============== salahaDB Statistics =================\n\n"
        << "Total Records:   " << totalRecordCount << '\n'
        << "Active Records:  " << activeRecordCount << '\n'
        << "Deleted Records: " << deletedRecordCount << "\n\n"
        << "Pages:           " << pageCount << '\n'
        << "Page Size:       " << Page::maxSize << " bytes\n"
        << "Database Size:   " << databaseSize << " bytes\n\n"
        << "=====================================================\n"
        << std::endl;
}

void Database::help() const {
    std::cout << "\n=========================== salahaDB Commands ===========================\n\n";

    std::cout << "  insert <name> <age>         Insert a new record.\n";
    std::cout << "  find <id>                   Find a record by ID.\n";
    std::cout << "  update <id> <name> <age>    Update an existing record.\n";
    std::cout << "  delete <id>                 Soft-delete a record.\n";
    std::cout << "\n";

    std::cout << "  select                      Display all active records.\n";
    std::cout << "  select name <name>          Filter records by name.\n";
    std::cout << "  select age <age>            Filter records by age.\n";
    std::cout << "  select sort name            Sort records by name.\n";
    std::cout << "  select sort age             Sort records by age.\n";
    std::cout << "\n";

    std::cout << "  compact                     Permanently remove deleted records.\n";
    std::cout << "\n";

    std::cout << "  help                        Show this help menu.\n";
    std::cout << "  exit                        Save the database and exit.\n";
    std::cout << "\n";
    std::cout << "==========================================================================\n";
}

void Database::find(const std::vector<std::string>& arguments) {
    // Check valid command
    if (arguments.size() != 1) return;

    int id = toInt(arguments[0]);

    if (id == -1) {
        std::cout << "Invalid command." << std::endl;
        return;
    }

    auto location = primaryIndex.search(id);

   if (!location.has_value()) {
        std::cout << "Record not found." << std::endl;
        return;
    }

    const Record& record =
        pageManager.getPages()[location->pageIndex]
                   .getRecords()[location->slotIndex];

    if (record.deleted) {
        std::cout << "ID has been deleted." << std::endl;
        return;
    }

    printRecord(record);
}

void Database::insert(
    const std::vector<std::string>& arguments
) {
    if (arguments.size() != 2)
        return;

    int age = toInt(arguments[1]);

    if (age == -1) {
        std::cout << "Invalid command.\n";
        return;
    }

    Record record;
    record.id = nextID;
    record.name = arguments[0];
    record.age = age;
    record.deleted = false;

    size_t pageIndex;
    size_t slotIndex;

    if (!pageManager.insertRecord(record, pageIndex, slotIndex)) {
        std::cout
            << "Error: Record could not fit in a page.\n";
        return;
    }

    reIndex(record, pageIndex, slotIndex);

    nextID++;

    std::cout << "Record inserted.\n";

    save();
}

void Database::deleteRecord(
    const std::vector<std::string>& arguments
) {
    if (arguments.size() != 1) {
        std::cout << "Invalid command.\n";
        return;
    }

    int id = std::stoi(arguments[0]);

    std::optional<RecordLocation> location = primaryIndex.search(id);

    if (!location.has_value()) {
        std::cout << "Record not found.\n";
        return;
    }

    size_t pageIndex = location->pageIndex;
    size_t slotIndex = location->slotIndex;

    Record& record =
        pageManager.getPages()[pageIndex]
                   .getRecords()[slotIndex];

    primaryIndex.remove(id);
    removeIdFromAge(record.age, id);
    removeIdFromName(record.name, id);
    record.deleted = true;

    save();

    std::cout << "Record deleted.\n";
}

void Database::select(
    const std::vector<std::string>& arguments
) const {
    bool found = false;

    // select
    if (arguments.empty()) {
        std::vector<Record> allRecords =
            pageManager.getAllRecords();

        for (const Record& record : allRecords) {
            if (!record.deleted) {
                printRecord(record);
                found = true;
            }
        }

        if (!found) {
            std::cout << "No records found.\n";
        }

        return;
    }

    // select id <minID> <maxID>
    if (arguments[0] == "id") {
        if (arguments.size() != 3) {
            std::cout << "Invalid command.\n";
            return;
        }

        int minID = toInt(arguments[1]);
        int maxID = toInt(arguments[2]);

        if (minID == -1 || maxID == -1) {
            std::cout << "Invalid command.\n";
            return;
        }

        std::vector<RecordLocation> range =
            primaryIndex.rangeSearch(minID, maxID);

        for (const RecordLocation& location : range) {
            const Record& record =
                pageManager.getPages()
                           [location.pageIndex]
                           .getRecords()
                           [location.slotIndex];

            if (!record.deleted) {
                printRecord(record);
                found = true;
            }
        }

        if (!found) {
            std::cout << "No matching records found.\n";
        }

        return;
    }

    if (arguments.size() != 2) {
        std::cout << "Invalid command.\n";
        return;
    }

    // select age <age>
    if (arguments[0] == "age") {
        int age = toInt(arguments[1]);

        if (age == -1) {
            std::cout << "Invalid command.\n";
            return;
        }

        auto ageIt = ages.find(age);

        if (ageIt != ages.end()) {
            const std::vector<int>& ids = ageIt->second;

            for (int id : ids) {
                auto location = primaryIndex.search(id);

                if (!location.has_value()) {
                    continue;
                }

                const Record& record =
                    pageManager.getPages()
                               [location->pageIndex]
                               .getRecords()
                               [location->slotIndex];

                if (!record.deleted) {
                    printRecord(record);
                    found = true;
                }
            }
        }
    }

    // select name <name>
    else if (arguments[0] == "name") {
        auto nameIt = names.find(arguments[1]);

        if (nameIt != names.end()) {
            const std::vector<int>& ids = nameIt->second;

            for (int id : ids) {
                auto location = primaryIndex.search(id);

                if (!location.has_value()) {
                    continue;
                }

                const Record& record =
                    pageManager.getPages()
                               [location->pageIndex]
                               .getRecords()
                               [location->slotIndex];

                if (!record.deleted) {
                    printRecord(record);
                    found = true;
                }
            }
        }
    }

    // select sort <age|name>
    else if (arguments[0] == "sort") {
        std::vector<Record> tempRecords =
            pageManager.getAllRecords();

        if (arguments[1] == "age") {
            sortAge(tempRecords);
        }
        else if (arguments[1] == "name") {
            sortName(tempRecords);
        }
        else {
            std::cout << "Invalid sorting category.\n";
            return;
        }

        for (const Record& record : tempRecords) {
            if (!record.deleted) {
                printRecord(record);
                found = true;
            }
        }

        if (!found) {
            std::cout << "No records found.\n";
        }

        return;
    }

    else {
        std::cout << "Invalid filter.\n";
        return;
    }

    if (!found) {
        std::cout << "No matching records found.\n";
    }
}

void Database::updateRecord(const std::vector<std::string>& arguments) {
    if (arguments.size() != 3)
        return;

    int id = toInt(arguments[0]);
    int age = toInt(arguments[2]);

    if (id == -1 || age == -1) {
        std::cout << "Invalid command." << std::endl;
        return;
    }

    auto location = primaryIndex.search(id);

    if (!location.has_value()) {
        std::cout << "ID does not exist." << std::endl;
        return;
    }

    size_t pageIndex = location->pageIndex;
    size_t slotIndex = location->slotIndex;

    Record& record =
        pageManager.getPages()[pageIndex]
                   .getRecords()[slotIndex];

    if (record.deleted) {
        std::cout << "ID has been deleted." << std::endl;
        return;
    }

    std::string oldName = record.name;
    int oldAge = record.age;

    Record updatedRecord = record;
    updatedRecord.name = arguments[1];
    updatedRecord.age = age;

    if (!pageManager.getPages()[pageIndex].updateRecord(
            slotIndex,
            updatedRecord
        )) {
        std::cout
            << "Error: Updated record does not fit in page."
            << std::endl;
        return;
    }

    removeIdFromAge(oldAge, id);
    removeIdFromName(oldName, id);

    ages[updatedRecord.age].push_back(id);
    names[updatedRecord.name].push_back(id);

    ages[updatedRecord.age].push_back(updatedRecord.id);
    names[updatedRecord.name].push_back(updatedRecord.id);

    save();

    std::cout << "Record updated." << std::endl;
}

void Database::compact() {
    pageManager.compact();

    primaryIndex.clear();
    ages.clear();
    names.clear();

    const std::vector<Page>& pages = pageManager.getPages();
    
    for (size_t pageIndex  = 0; pageIndex  < pages.size(); pageIndex ++) {
        const std::vector<Record>& records = pages[pageIndex ].getRecords();
        for (size_t slotIndex = 0; slotIndex < records.size(); slotIndex ++) {
            const Record& record = records[slotIndex];
            reIndex(record, pageIndex , slotIndex);
        }
    }

    save();

    std::cout << "Database compacted." << std::endl;
}

int toInt(const std::string value) {
    try {
        size_t position;
        int num = std::stoi(value, &position);

        if (position != value.size() || num < 0) {
            return -1;
        }

        return num;
    }
    catch (...) {
        return -1;
    }
}

void Database::printRecord(const Record& record) const {
    std::cout << "ID: " << record.id <<std::endl;
    std::cout << "Name: " << record.name <<std::endl;
    std::cout << "Age: " << record.age << std::endl << std::endl;
}

void Database::sortAge(std::vector<Record>& records) const {
    std::sort(records.begin(), records.end(), [](const Record& a, const Record& b) {
        return a.age < b.age;
    });
}

void Database::sortName(std::vector<Record>& records) const {
    std::sort(records.begin(), records.end(), [](const Record& a, const Record& b) {
        return std::lexicographical_compare(
            a.name.begin(), a.name.end(), b.name.begin(), b.name.end(),
            [](unsigned char c1, unsigned char c2) { return std::tolower(c1) < std::tolower(c2); }
        );
    });
}

void Database::removeIdFromAge(int age, int id){
    auto it = ages.find(age);
    if (it == ages.end()) return;  // age not in map

    std::vector<int>& ids = it->second; // gets vector of ids
    
    // delete id if it exists
    auto idIt = std::find(ids.begin(), ids.end(), id);
    if (idIt != ids.end()) {
        ids.erase(idIt);
    }

    if(ids.empty()){
        ages.erase(it);
    }
}

void Database::removeIdFromName(std::string& name, int id){
    auto it = names.find(name);
    if (it == names.end()) return;  // name not in map

    std::vector<int>& ids = it->second; // gets vector of ids
    
    // delete id if it exists
    auto idIt = std::find(ids.begin(), ids.end(), id);
    if (idIt != ids.end()) {
        ids.erase(idIt);
    }

    if(ids.empty()){
        names.erase(it);
    }
}