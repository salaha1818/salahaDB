#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include "Record.h"
#include "Serializer.h"
#include "Page.h"
#include "PageManager.h"

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

size_t Serializer::getRecordSize(const Record& record){
    return sizeof(record.id) 
        + sizeof(record.age) 
        + sizeof(record.deleted) 
        + sizeof(uint32_t)  // int to hold the size of the name (holds a number)
        + record.name.size(); // the actual space taken by the name
}

// reinterpret_cast tells cpp to treat the int memory as a sequence of bytes, since output write expects
// a pointer to charecters 
void Serializer::serializeBinary(const Record& record, std::ostream& output){
    uint32_t nameLength =
        static_cast<uint32_t>(record.name.size());

    // Write ID
    output.write(
        reinterpret_cast<const char*>(&record.id),
        sizeof(record.id)
    );

    // Write age
    output.write(
        reinterpret_cast<const char*>(&record.age),
        sizeof(record.age)
    );

    // Write deleted flag
    output.write(
        reinterpret_cast<const char*>(&record.deleted),
        sizeof(record.deleted)
    );

    // Write name length
    output.write(
        reinterpret_cast<const char*>(&nameLength),
        sizeof(nameLength)
    );

    // Write the name characters
    output.write(
        record.name.data(),
        nameLength
    );
 }

Record Serializer::deserializeBinary(std::istream& input){
    Record record;
    uint32_t nameLength;

    input.read(reinterpret_cast<char*>(&record.id), sizeof(record.id)); // id
    input.read(reinterpret_cast<char*>(&record.age), sizeof(record.age)); // age
    input.read(reinterpret_cast<char*>(&record.deleted), sizeof(record.deleted)); //deleted
    input.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength)); // name length
    record.name.resize(nameLength);
    input.read(record.name.data(), nameLength); // name

    return record;
 }
