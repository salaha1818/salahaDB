#include <vector>
#include "Record.h"
#include "Serializer.h"
#include "StorageEngine.h"
#include <iostream>
#include <fstream>
#include <string>

StorageEngine::StorageEngine(const std::string& filename)
    : filename(filename)
{}

// read from file
std::vector<Page> StorageEngine::loadPages() {
    std::vector<Page> pages;
    // reading a binary file
    std::ifstream file(filename, std::ios::binary);

    // check if the file opened successfully
    if (!file) {
        return pages;
    }

    // go to end of file and store the size
    file.seekg(0, std::ios::end);
    std::streampos fileSize = file.tellg();

    // go back to begining of new file
    file.seekg(0, std::ios::beg);

    // check if the file is a valid size (multiple of 4096)
    if(fileSize % Page::maxSize != 0) {
        std::cout
            << "Error: Invalid database file size."
            << std::endl;
        return pages;
    }

    // find the page number
    uint32_t pageCount =
        static_cast<uint32_t>(fileSize / Page::maxSize);

    // deserialzie every page
    for (uint32_t i = 0; i < pageCount; i++) {
        Page page =
            Page::deserialize(file);

        if (!file) {
            std::cout
                << "Error: Could not load page."
                << std::endl;

            break;
        }

        pages.push_back(page);
    }

    return pages;
}

// write to file
void StorageEngine::savePages(
    const std::vector<Page>& pages
) {
    std::ofstream file(
        filename,
        std::ios::binary | std::ios::trunc
    );

    if (!file) {
        std::cout
            << "Error: Could not open file."
            << std::endl;

        return;
    }

    for (const Page& page : pages) {
        page.serialize(file);

        if (!file) {
            std::cout
                << "Error: Could not save page."
                << std::endl;

            return;
        }
    }
}
