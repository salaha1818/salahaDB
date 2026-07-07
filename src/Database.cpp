#include <iostream>
#include <string>
#include <vector>
#include "Command.h"
#include "Parser.h"
#include "Database.h"

void Database::execute(const Command& cmd){
    if(cmd.type == "insert"){
        insert(cmd.arguments);
    } else if(cmd.type == "delete"){
        deleteRecord(cmd.arguments);
    } else if(cmd.type == "select"){
        select();
    } else {}
}

void Database::insert(const std::vector<std::string>& arguments){
    if(arguments.size() != 2) return;
    Record record;
    record.id = nextID;
    record.name = arguments[0];
    record.age = stoi(arguments[1]);
    records.push_back(record);
    nextID++;
    std::cout << "Record inserted." << std::endl;
}

void Database::deleteRecord(const std::vector<std::string>& arguments){
    if(arguments.size() != 1) return;
    int targetId = stoi(arguments[0]);
    for(auto& record : records){
        if(record.id == targetId){
            record.deleted = true;
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