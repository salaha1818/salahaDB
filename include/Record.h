#pragma once

#include <cstdint>
#include <string>

// stores one record in the database
struct Record {
    uint32_t id = 0;
    std::string name;
    uint32_t age = 0;

    // stays in the database until vacuum removes it
    bool deleted = false;
};