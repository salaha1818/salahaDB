#pragma once

#include <vector>
#include "Record.h"
#include "Serializer.h"
#include "Page.h"

// handles saving and loading database pages
class StorageEngine {
public:
    // stores the database file name
    StorageEngine(const std::string& filename);

    // loads all pages from the database file
    std::vector<Page> loadPages();

    // saves all pages to the database file
    void savePages(const std::vector<Page>& pages);

    // information stored at the start of the database file
    struct DatabaseHeader {
        char magic[4];
        uint32_t version;
        uint32_t pageSize;
    };

private:
    std::string filename;
};