#include <iostream>
#include <string>
#include <vector>
#include "Command.h"
#include "Parser.h"
#include "Database.h"

int toInt(std::string value);

Database::Database(const std::string& filename)
    : storage(filename)
{
    load();
}
void Database::load(){
    records = storage.load();
    uint32_t largestID = 0;
    for (const Record& record : records){
        if(record.id > largestID){
            largestID = record.id;
        }
    }
    nextID = largestID + 1;
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
        select();
    } else if (cmd.type == "update") {
        updateRecord(cmd.arguments);
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
    for(auto& record : records){
        if(record.id == targetId){
            if(record.deleted) {
                std::cout << "Record has been deleted." << std::endl;
            } else {
                record.deleted = true;
                save();
                std::cout << "Record deleted." << std::endl;
            }
            return;
        }
    }
    std::cout << "Record does not exist." << std::endl;
}

void Database::select() const{
    for(const auto& record : records){
        if(record.deleted == false) {
            std::cout << "ID: " << record.id <<std::endl;
            std::cout << "Name: " << record.name <<std::endl;
            std::cout << "Age: " << record.age << std::endl << std::endl;
        }
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
    for(auto& record:records){
        if(id == record.id && !record.deleted){
            record.name = arguments[1];
            record.age = age;
            save();
            std::cout << "Record updated." << std::endl;
            return;
        }
    }
    std::cout << "Record does not exist." << std::endl;
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