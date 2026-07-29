#pragma once

#include <vector>
#include "Record.h"
#include "Serializer.h"
#include "Page.h"

class StorageEngine {
public:
    StorageEngine(const std::string& filename);

    std::vector<Page> loadPages();
    void savePages(const std::vector<Page>& pages);

    struct DatabaseHeader {
        char magic[4];
        uint32_t version;
        uint32_t pageSize;
    };

private:
    std::string filename;
};
