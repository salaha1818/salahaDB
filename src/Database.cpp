#include <iostream>
#include <string>
#include <vector>
#include "Command.h"
#include "Parser.h"
#include "Database.h"
#include <algorithm>
#include <cctype>


int toInt(std::string value);

Database::Database(const std::string& filename)
    : storage(filename)
{
    load();
}

void Database::load(){
    indices.clear();
    ages.clear();
    names.clear();
    records = storage.load();
    uint32_t largestID = 0;
    for (size_t i = 0; i < records.size(); i++) {
        Record& record = records[i];

        if (record.id > largestID) {
            largestID = record.id;
        }
        reIndex(record, i);
    }
    nextID = largestID + 1;
}

void Database::reIndex(Record& record, int index) {
    indices[record.id] = index;
    ages[record.age].push_back(record.id);
    names[record.name].push_back(record.id);
}

void Database::save(){
    storage.save(records);
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
    } else {}
}

void Database::insert(const std::vector<std::string>& arguments){
    if(arguments.size() != 2) return;
    int age = toInt(arguments[1]);
    if(age == -1) {
        std::cout << "Invalid command." << std::endl;
        return;
    }
    Record record;
    record.id = nextID;
    record.name = arguments[0];
    record.age = age;
    records.push_back(record);
    indices[record.id] = records.size() - 1;
    ages[record.age].push_back(record.id);
    names[record.name].push_back(record.id);
    nextID++;
    std::cout << "Record inserted." << std::endl;
    save();
}

void Database::deleteRecord(const std::vector<std::string>& arguments){
    if(arguments.size() != 1) return;
    int targetId = toInt(arguments[0]);
    if(targetId == -1){
        std::cout << "Invalid command." << std::endl;
        return;
    }
    // check if id exists and find matching record
    if (indices.count(targetId)) {
        int index = indices[targetId];
        if (!records[index].deleted) {
            records[index].deleted = true;
            removeIdFromAge(records[index].age, targetId);
            removeIdFromName(records[index].name, targetId);
            save();
            std::cout << "Record deleted." << std::endl;
        } else {
            std::cout << "Record has already been deleted." << std::endl;
        }
    } else {
        std::cout << "ID does not exist." << std::endl;
    }
}

void Database::select(const std::vector<std::string>& arguments) const {
    bool found = false;

    // select
    if (arguments.empty()) {
        for (const auto& record : records) {
            if (!record.deleted) {
                printRecord(record);
                found = true;
            }
        }

        if (!found) {
            std::cout << "No records found." << std::endl;
        }

        return;
    }

    // All current filters require exactly two arguments
    if (arguments.size() != 2) {
        std::cout << "Invalid command." << std::endl;
        return;
    }

    // select age <age>
    if (arguments[0] == "age") {
        int age = toInt(arguments[1]);

        if (age == -1) {
            std::cout << "Invalid command." << std::endl;
            return;
        }

        auto ageIt = ages.find(age);

        if (ageIt != ages.end()) {
            const std::vector<int>& ids = ageIt->second;

            for (int id : ids) {
                auto indexIt = indices.find(id);

                if (indexIt == indices.end()) {
                    continue;
                }

                size_t index = indexIt->second;

                if (!records[index].deleted) {
                    printRecord(records[index]);
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
                auto indexIt = indices.find(id);

                if (indexIt == indices.end()) {
                    continue;
                }

                size_t index = indexIt->second;

                if (!records[index].deleted) {
                    printRecord(records[index]);
                    found = true;
                }
            }
        }
    }

    // select sort <age|name>
    else if (arguments[0] == "sort") {
        std::vector<Record> tempRecords = records;

        if (arguments[1] == "age") {
            sortAge(tempRecords);
        } else if (arguments[1] == "name") {
            sortName(tempRecords);
        } else {
            std::cout << "Invalid sorting category." << std::endl;
            return;
        }

        for (const auto& record : tempRecords) {
            if (!record.deleted) {
                printRecord(record);
                found = true;
            }
        }

        if (!found) {
            std::cout << "No records found." << std::endl;
        }

        return;
    }

    else {
        std::cout << "Invalid filter." << std::endl;
        return;
    }

    if (!found) {
        std::cout << "No matching records found." << std::endl;
    }
}

void Database::help() const {
    std::cout << "Available Commands:" << std::endl << "----------------------" << std::endl;
    std::cout << "insert <name> <age>         Insert a new record." << std::endl;
    std::cout << "select                      Display all active records." << std::endl;
    std::cout << "update <id> <name> <age>    Update an existing record." << std::endl;
    std::cout << "find <id>                   Find a record by ID." << std::endl;
    std::cout << "delete <id>                 Soft delete a record." << std::endl;
    std::cout << "help                        Display help menu." << std::endl;
    std::cout << "exit                        Exit the program." << std::endl;
}

void Database::find(const std::vector<std::string>& arguments){
    // check valid command
    if(arguments.size() != 1) return;
    int id = toInt(arguments[0]);
    if(id == -1) {
        std::cout << "Invalid command." << std::endl;
        return;
    }

    // check if id exists and find matching record
    if (indices.count(id)) {
        int index = indices[id];
        if (!records[index].deleted) {
            printRecord(records[index]);
        } else {
            std::cout << "ID has been deleted." << std::endl;
        }
    } else {
        std::cout << "ID does not exist." << std::endl;
    }
}

void Database::updateRecord(const std::vector<std::string>& arguments) {
    if(arguments.size() != 3) return;
    int id = toInt(arguments[0]);
    int age = toInt(arguments[2]);
    if(id == -1 || age == -1){
        std::cout << "Invalid command." << std::endl;
        return;
    }
    if (indices.count(id)) {
        size_t index = indices[id];
        if (!records[index].deleted) {
            removeIdFromAge(records[index].age, id);
            removeIdFromName(records[index].name, id);
            records[index].name = arguments[1];
            records[index].age = age;
            ages[records[index].age].push_back(id);
            names[records[index].name].push_back(id);
            save();
            std::cout << "Record updated." << std::endl;
        } else {
            std::cout << "ID has been deleted." << std::endl;
        }
    } else {
        std::cout << "ID does not exist." << std::endl;
    }
}

void Database::compact () {
    std::vector <Record> compactRecords;

    for (const auto& record:records) {
        if(!record.deleted) {
            compactRecords.push_back(record);
        }
    }
    records = std::move(compactRecords);

    // rebuild the primary and secondary indexes
    indices.clear();
    ages.clear();
    names.clear();

    for (size_t i = 0; i < records.size(); i++){
        Record& record = records[i];
        reIndex(record, i);
    }
    save();
    std::cout << "Database compacted." << std::endl;
}

int toInt(std::string value) {
    int num;
    try {
        num = std::stoi(value);
    } catch (...) {
        return -1;
    }
    if(num >= 0) {
        return num;
    } else return -1;
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
            [](char c1, char c2) { return std::tolower(c1) < std::tolower(c2); }
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

void Database::removeIdFromName(std::string name, int id){
    auto it = names.find(name);
    if (it == names.end()) return;  // age not in map

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