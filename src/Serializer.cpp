#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include "Record.h"
#include "Serializer.h"
#include "Page.h"
#include "PageManager.h"

// converts a record into a single line of text
std::string Serializer::serialize(const Record& record){
    std::string s = record.deleted ? "true" : "false";
    std::string line = std::to_string(record.id) + " " + record.name + " " + std::to_string(record.age) + " " + s;
    return line;
}

// rebuilds a record from its text format
Record Serializer::deserialize(const std::string& line) {
    Record record;
    std::string boolean;
    std::istringstream iss(line);
    iss >> record.id >> record.name >> record.age >> boolean;
    record.deleted = (boolean == "true");
    return record;
}

// returns how many bytes the record takes in binary
size_t Serializer::getRecordSize(const Record& record){
    return sizeof(record.id)
        + sizeof(record.age)
        + sizeof(record.deleted)
        + sizeof(uint32_t)  // stores the length of the name
        + record.name.size(); // actual name characters
}

// reinterpret_cast tells cpp to treat the value as raw bytes
// since write() expects a pointer to characters
void Serializer::serializeBinary(const Record& record, std::ostream& output){
    uint32_t nameLength =
        static_cast<uint32_t>(record.name.size());

    // write fixed-size fields first
    output.write(
        reinterpret_cast<const char*>(&record.id),
        sizeof(record.id)
    );

    output.write(
        reinterpret_cast<const char*>(&record.age),
        sizeof(record.age)
    );

    output.write(
        reinterpret_cast<const char*>(&record.deleted),
        sizeof(record.deleted)
    );

    // write the name length before the name itself
    output.write(
        reinterpret_cast<const char*>(&nameLength),
        sizeof(nameLength)
    );

    // write the actual name
    output.write(
        record.name.data(),
        nameLength
    );
}

// reads a binary record back in the same order it was written
Record Serializer::deserializeBinary(std::istream& input){
    Record record;
    uint32_t nameLength;

    // read fixed-size fields
    input.read(reinterpret_cast<char*>(&record.id), sizeof(record.id));
    input.read(reinterpret_cast<char*>(&record.age), sizeof(record.age));
    input.read(reinterpret_cast<char*>(&record.deleted), sizeof(record.deleted));

    // read the name length first so we know how much space to allocate
    input.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));

    record.name.resize(nameLength);

    // read the name characters
    input.read(record.name.data(), nameLength);

    return record;
}