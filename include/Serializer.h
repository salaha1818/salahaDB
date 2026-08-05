#pragma once

#include <vector>
#include "Record.h"

class Serializer
{
public:
    // convert a record to and from a text format
    std::string serialize(const Record& record);
    Record deserialize(const std::string& line);

    // returns the size of a record when stored in binary
    static size_t getRecordSize(const Record& record);

    // write and read records in binary format
    static void serializeBinary(const Record& record, std::ostream& output);
    static Record deserializeBinary(std::istream& input);
};