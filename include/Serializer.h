#pragma once

#include <vector>
#include "Record.h"

class Serializer
{
public:
    std::string serialize(const Record& record);
    Record deserialize(const std::string& line);
};