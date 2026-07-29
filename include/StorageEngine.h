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

private:
    std::string filename;
};
