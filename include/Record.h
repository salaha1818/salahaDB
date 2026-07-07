#pragma once

#include <cstdint>
#include <string>

struct Record {
    uint32_t id;
    std::string name;
    uint32_t age;
    bool deleted = false;
};