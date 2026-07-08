#pragma once

#include <vector>
#include "Record.h"
#include "Serializer.h"

class StorageEngine {
public:
    StorageEngine(const std::string& filename);

    std::vector<Record> load();
    void save(const std::vector<Record>& records);

private:
    std::string filename;
};
