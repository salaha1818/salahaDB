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

// safely converts a string into a non-negative integer
int toInt(std::string value);

// opens the database file and loads its saved data
Database::Database(const std::string& filename)
    : storage(filename)
{
    load();
}

// loads all saved pages and rebuilds the indexes
void Database::load() {
    // clear anything already stored in memory
    primaryIndex.clear();
    ages.clear();
    names.clear();
    pageManager.clear();

    // load pages from disk and give them to PageManager
    std::vector<Page> pages = storage.loadPages();
    pageManager.loadPages(pages);

    uint32_t largestID = 0;
    bool hasRecords = false;

    const std::vector<Page>& loadedPages = pageManager.getPages();

    // rebuild indexes from the loaded pages
    for (size_t pageIndex = 0;
         pageIndex < loadedPages.size();
         pageIndex++) {

        const std::vector<Record>& pageRecords =
            loadedPages[pageIndex].getRecords();

        for (size_t slotIndex = 0;
             slotIndex < pageRecords.size();
             slotIndex++) {

            const Record& record = pageRecords[slotIndex];

            // add the record back into the indexes
            reIndex(record, pageIndex, slotIndex);

            // track the largest id so the next id can be restored
            if (record.id > largestID) {
                largestID = record.id;
            }

            hasRecords = true;
        }
    }

    // continue after the largest saved id
    nextID = hasRecords ? largestID + 1 : 1;
}

// adds one active record to the primary and secondary indexes
void Database::reIndex(
    const Record& record,
    size_t pageIndex,
    size_t slotIndex
) {
    // deleted records should not be searchable
    if (record.deleted) {
        return;
    }

    RecordLocation location{pageIndex, slotIndex};

    // primary index uses id to find the page and slot
    primaryIndex.insert(record.id, location);

    // secondary indexes use age and name to find record ids
    ages[record.age].push_back(record.id);
    names[record.name].push_back(record.id);
}

// saves all current pages to the database file
void Database::save(){
    storage.savePages(pageManager.getPages());
}

// sends the parsed command to the correct database function
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
    } else if (cmd.type == "vacuum") {
        vacuum();
    } else if (cmd.type == "stats") {
        stats();
    } else if (cmd.type == "drop") {
        drop();
    }else {}
}

// counts the records and displays database information
void Database::stats() const {
    size_t totalRecordCount = 0;
    size_t activeRecordCount = 0;
    size_t deletedRecordCount = 0;
    size_t pageCount = 0;
    
    const std::vector <Page>& pages = pageManager.getPages();
    pageCount = pages.size();

    // database size is the header plus all fixed-size pages
    size_t databaseSize =
        sizeof(StorageEngine::DatabaseHeader) +
        pageCount * Page::maxSize;

    // count active and deleted records
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
        << "\n========== salahaDB Statistics ==========\n\n"
        << "Total records:     " << totalRecordCount << '\n'
        << "Active records:    " << activeRecordCount << '\n'
        << "Deleted records:   " << deletedRecordCount << "\n\n"
        << "Pages allocated:   " << pageCount << '\n'
        << "Page size:         " << Page::maxSize << " bytes\n"
        << "Database size:     " << databaseSize << " bytes\n\n"
        << "=========================================\n";
}

// displays every supported salahaDB command
void Database::help() const {
    std::cout
        << "\n========================== salahaDB Help ==========================\n\n"

        << "Data Manipulation\n"
        << "-----------------\n"
        << "  INSERT INTO records VALUES <name> <age>\n"
        << "      Insert a new record.\n\n"

        << "  SELECT * FROM records\n"
        << "      Display all active records.\n\n"

        << "  SELECT * FROM records WHERE name = <name>\n"
        << "      Display records matching a name.\n\n"

        << "  SELECT * FROM records WHERE age = <age>\n"
        << "      Display records matching an age.\n\n"

        << "  SELECT * FROM records WHERE id BETWEEN <min> AND <max>\n"
        << "      Display records within an ID range.\n\n"

        << "  SELECT * FROM records ORDER BY name\n"
        << "  SELECT * FROM records ORDER BY age\n"
        << "      Display records sorted by a field.\n\n"

        << "  UPDATE records SET name = <name> age = <age> WHERE id = <id>\n"
        << "      Update an existing record.\n\n"

        << "  DELETE FROM records WHERE id = <id>\n"
        << "      Delete a record.\n\n"

        << "  FIND <id>\n"
        << "      Find a record by ID.\n\n\n"

        << "Database Commands\n"
        << "-----------------\n"
        << "  VACUUM\n"
        << "      Permanently remove deleted records.\n\n"

        << "  DROP DATABASE CONFIRM\n"
        << "      Delete all records and reset the database.\n\n"

        << "  STATS\n"
        << "      Display database statistics.\n\n"

        << "  HELP\n"
        << "      Display this help menu.\n\n"

        << "  EXIT\n"
        << "      Save the database and exit.\n\n"

        << "==================================================================\n";
}

// finds one record using the primary id index
void Database::find(
    const std::vector<std::string>& arguments
) const {
    if (arguments.size() != 1) {
        std::cout << "Invalid command.\n";
        return;
    }

    int id = toInt(arguments[0]);

    if (id == -1) {
        std::cout << "Invalid command.\n";
        return;
    }

    // search for the record location in the B+ tree
    auto location = primaryIndex.search(id);

    if (!location.has_value()) {
        std::cout << "No matching record found.\n";
        return;
    }

    // use the stored page and slot to get the record
    const Record& record =
        pageManager.getPages()
                   [location->pageIndex]
                   .getRecords()
                   [location->slotIndex];

    printRecord(record);
}

// creates a new record and inserts it into the database
void Database::insert(
    const std::vector<std::string>& arguments
) {
    if (arguments.size() != 2) {
        std::cout << "Invalid command.\n";
        return;
    }

    int age = toInt(arguments[1]);

    if (age == -1) {
        std::cout << "Invalid command.\n";
        return;
    }

    // build the new record
    Record record;
    record.id = nextID;
    record.name = arguments[0];
    record.age = age;
    record.deleted = false;

    size_t pageIndex;
    size_t slotIndex;

    // find a page with enough space for the record
    if (!pageManager.insertRecord(
            record,
            pageIndex,
            slotIndex
        )) {

        std::cout
            << "Error: Record could not fit in a page.\n";
        return;
    }

    // add the new record to every index
    reIndex(record, pageIndex, slotIndex);

    nextID++;

    save();

    std::cout
        << "1 row inserted. ID: "
        << record.id
        << '\n';
}

// removes a record from the indexes and marks it as deleted
void Database::deleteRecord(
    const std::vector<std::string>& arguments
) {
    if (arguments.size() != 1) {
        std::cout << "Invalid command.\n";
        return;
    }

    int id = toInt(arguments[0]);

    if (id == -1) {
        std::cout << "Invalid command.\n";
        return;
    }

    auto location = primaryIndex.search(id);

    if (!location.has_value()) {
        std::cout << "No matching record found.\n";
        return;
    }

    size_t pageIndex = location->pageIndex;
    size_t slotIndex = location->slotIndex;

    Record& record =
        pageManager.getPages()[pageIndex]
                   .getRecords()[slotIndex];

    // remove the id from the primary index
    if (!primaryIndex.remove(id)) {
        std::cout
            << "Error: Could not remove record from primary index.\n";
        return;
    }

    // remove the id from the secondary indexes
    removeIdFromAge(record.age, id);
    removeIdFromName(record.name, id);

    // keep the record as a tombstone until vacuum runs
    record.deleted = true;

    save();

    std::cout << "1 row deleted.\n";
}

// clears the pages, indexes, and id counter
void Database::drop() {
    pageManager.clear();

    primaryIndex.clear();
    ages.clear();
    names.clear();

    nextID = 1;

    save();

    std::cout << "Database dropped.\n";
}

// handles every supported SELECT format
void Database::select(
    const std::vector<std::string>& arguments
) const {
    size_t rowsReturned = 0;

    // SELECT * FROM records
    if (arguments.empty()) {
        std::vector<Record> allRecords =
            pageManager.getAllRecords();

        // print every active record
        for (const Record& record : allRecords) {
            if (!record.deleted) {
                printRecord(record);
                std::cout << '\n';
                rowsReturned++;
            }
        }

        if (rowsReturned == 0) {
            std::cout << "No records found.\n";
        } else {
            std::cout
                << rowsReturned
                << (rowsReturned == 1
                    ? " row returned.\n"
                    : " rows returned.\n");
        }

        return;
    }

    // SELECT * FROM records WHERE id BETWEEN <min> AND <max>
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

        // use the B+ tree leaf links to search the full id range
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
                std::cout << '\n';
                rowsReturned++;
            }
        }

        if (rowsReturned == 0) {
            std::cout << "No matching records found.\n";
        } else {
            std::cout
                << rowsReturned
                << (rowsReturned == 1
                    ? " row returned.\n"
                    : " rows returned.\n");
        }

        return;
    }

    if (arguments.size() != 2) {
        std::cout << "Invalid command.\n";
        return;
    }

    // SELECT by age
    if (arguments[0] == "age") {
        int age = toInt(arguments[1]);

        if (age == -1) {
            std::cout << "Invalid command.\n";
            return;
        }

        // get all ids stored under this age
        auto ageIt = ages.find(age);

        if (ageIt != ages.end()) {
            for (int id : ageIt->second) {
                // use the primary index to find the full record
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
                    std::cout << '\n';
                    rowsReturned++;
                }
            }
        }
    }

    // SELECT by name
    else if (arguments[0] == "name") {
        // get all ids stored under this name
        auto nameIt = names.find(arguments[1]);

        if (nameIt != names.end()) {
            for (int id : nameIt->second) {
                // use the primary index to find the full record
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
                    std::cout << '\n';
                    rowsReturned++;
                }
            }
        }
    }

    // ORDER BY age|name
    else if (arguments[0] == "sort") {
        // make a copy so the stored record order does not change
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
                std::cout << '\n';
                rowsReturned++;
            }
        }

        if (rowsReturned == 0) {
            std::cout << "No records found.\n";
        } else {
            std::cout
                << rowsReturned
                << (rowsReturned == 1
                    ? " row returned.\n"
                    : " rows returned.\n");
        }

        return;
    }

    else {
        std::cout << "Invalid filter.\n";
        return;
    }

    if (rowsReturned == 0) {
        std::cout << "No matching records found.\n";
    } else {
        std::cout
            << rowsReturned
            << (rowsReturned == 1
                ? " row returned.\n"
                : " rows returned.\n");
    }
}

// updates one record and fixes its secondary index entries
void Database::updateRecord(
    const std::vector<std::string>& arguments
) {
    if (arguments.size() != 3) {
        std::cout << "Invalid command.\n";
        return;
    }

    int id = toInt(arguments[0]);
    int age = toInt(arguments[2]);

    if (id == -1 || age == -1) {
        std::cout << "Invalid command.\n";
        return;
    }

    // find the record using the primary index
    auto location = primaryIndex.search(id);

    if (!location.has_value()) {
        std::cout << "No matching record found.\n";
        return;
    }

    size_t pageIndex = location->pageIndex;
    size_t slotIndex = location->slotIndex;

    Record& record =
        pageManager.getPages()[pageIndex]
                   .getRecords()[slotIndex];

    if (record.deleted) {
        std::cout << "No matching record found.\n";
        return;
    }

    // save the old values so their index entries can be removed
    std::string oldName = record.name;
    int oldAge = record.age;

    // build the updated version of the record
    Record updatedRecord = record;
    updatedRecord.name = arguments[1];
    updatedRecord.age = age;

    // reject the update if the new record no longer fits
    if (!pageManager.getPages()[pageIndex].updateRecord(
            slotIndex,
            updatedRecord
        )) {

        std::cout
            << "Error: Updated record does not fit in page.\n";
        return;
    }

    // remove the old secondary index entries
    removeIdFromAge(oldAge, id);
    removeIdFromName(oldName, id);

    // add the new secondary index entries
    ages[updatedRecord.age].push_back(id);
    names[updatedRecord.name].push_back(id);

    save();

    std::cout << "1 row updated.\n";
}

// removes tombstones and rebuilds every index
void Database::vacuum() {
    // rebuild the pages using only active records
    pageManager.compact();

    primaryIndex.clear();
    ages.clear();
    names.clear();

    const std::vector<Page>& pages =
        pageManager.getPages();

    // record locations changed, so rebuild all indexes
    for (size_t pageIndex = 0;
         pageIndex < pages.size();
         pageIndex++) {

        const std::vector<Record>& records =
            pages[pageIndex].getRecords();

        for (size_t slotIndex = 0;
             slotIndex < records.size();
             slotIndex++) {

            const Record& record =
                records[slotIndex];

            reIndex(
                record,
                pageIndex,
                slotIndex
            );
        }
    }

    save();

    std::cout << "Vacuum completed.\n";
}

// safely converts a string to a non-negative integer
int toInt(const std::string value) {
    try {
        size_t position;
        int num = std::stoi(value, &position);

        // reject extra characters and negative values
        if (position != value.size() || num < 0) {
            return -1;
        }

        return num;
    }
    catch (...) {
        return -1;
    }
}

// prints one record in the terminal
void Database::printRecord(const Record& record) const {
    std::cout
        << "ID:   " << record.id << '\n'
        << "Name: " << record.name << '\n'
        << "Age:  " << record.age << '\n';
}

// sorts records from youngest to oldest
void Database::sortAge(std::vector<Record>& records) const {
    std::sort(records.begin(), records.end(), [](const Record& a, const Record& b) {
        return a.age < b.age;
    });
}

// sorts names alphabetically without caring about capitalization
void Database::sortName(std::vector<Record>& records) const {
    std::sort(records.begin(), records.end(), [](const Record& a, const Record& b) {
        return std::lexicographical_compare(
            a.name.begin(), a.name.end(), b.name.begin(), b.name.end(),
            [](unsigned char c1, unsigned char c2) { return std::tolower(c1) < std::tolower(c2); }
        );
    });
}

// removes an id from the age index
// removes the full age entry when its vector becomes empty
void Database::removeIdFromAge(int age, int id){
    auto it = ages.find(age);

    // age is not currently stored in the index
    if (it == ages.end()) return;

    // vector of ids that have this age
    std::vector<int>& ids = it->second;
    
    // delete the id if it exists
    auto idIt = std::find(ids.begin(), ids.end(), id);

    if (idIt != ids.end()) {
        ids.erase(idIt);
    }

    // remove the age from the map when no ids are left
    if(ids.empty()){
        ages.erase(it);
    }
}

// removes an id from the name index
// removes the full name entry when its vector becomes empty
void Database::removeIdFromName(std::string& name, int id){
    auto it = names.find(name);

    // name is not currently stored in the index
    if (it == names.end()) return;

    // vector of ids that have this name
    std::vector<int>& ids = it->second;
    
    // delete the id if it exists
    auto idIt = std::find(ids.begin(), ids.end(), id);

    if (idIt != ids.end()) {
        ids.erase(idIt);
    }

    // remove the name from the map when no ids are left
    if(ids.empty()){
        names.erase(it);
    }
}