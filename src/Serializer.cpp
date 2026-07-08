#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include "Record.h"
#include "Serializer.h"

std::string Serializer::serialize(const Record& record){
    std::string s = record.deleted ? "true" : "false"; 
    std::string line = std::to_string(record.id) + " " + record.name + " " + std::to_string(record.age) + " " + s;
    return line;
}

Record Serializer::deserialize(const std::string& line) {
    Record record;
    std::string boolean;
    std::istringstream iss(line);
    iss >> record.id >> record.name >> record.age >> boolean;
    record.deleted = (boolean == "true");
    return record;
}