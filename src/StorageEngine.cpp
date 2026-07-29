#include <vector>
#include "Record.h"
#include "Page.h"
#include "Serializer.h"
#include "StorageEngine.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

StorageEngine::StorageEngine(const std::string& filename)
    : filename(filename)
{}

// read from file
std::vector<Page> StorageEngine::loadPages() {
    std::vector<Page> pages;

    std::ifstream file(filename, std::ios::binary);

    // Missing file means this is a new database.
    if (!file) {
        return pages;
    }

    DatabaseHeader header{};

    file.read(
        reinterpret_cast<char*>(&header),
        sizeof(DatabaseHeader)
    );

    if (!file) {
        std::cout
            << "Error: Could not read database header."
            << std::endl;

        std::exit(EXIT_FAILURE);
    }

    if (header.magic[0] != 'S' ||
        header.magic[1] != 'M' ||
        header.magic[2] != 'D' ||
        header.magic[3] != 'B') {

        std::cout
            << "Error: Invalid database file."
            << std::endl;

        std::exit(EXIT_FAILURE);
    }

    if (header.version != 1) {
        std::cout
            << "Error: Unsupported database version."
            << std::endl;

        std::exit(EXIT_FAILURE);
    }

    if (header.pageSize != Page::maxSize) {
        std::cout
            << "Error: Page size mismatch."
            << std::endl;

        std::exit(EXIT_FAILURE);
    }

    file.seekg(0, std::ios::end);

    if (!file) {
        std::cout
            << "Error: Could not determine database file size."
            << std::endl;

        std::exit(EXIT_FAILURE);
    }

    std::streamoff fileSize = file.tellg();

    if (fileSize <
        static_cast<std::streamoff>(
            sizeof(DatabaseHeader)
        )) {

        std::cout
            << "Error: Invalid database file size."
            << std::endl;

        std::exit(EXIT_FAILURE);
    }

    std::streamoff pageBytes =
        fileSize -
        static_cast<std::streamoff>(
            sizeof(DatabaseHeader)
        );

    if (pageBytes %
        static_cast<std::streamoff>(Page::maxSize) != 0) {

        std::cout
            << "Error: Invalid database file size."
            << std::endl;

        std::exit(EXIT_FAILURE);
    }

    uint32_t pageCount =
        static_cast<uint32_t>(
            pageBytes /
            static_cast<std::streamoff>(
                Page::maxSize
            )
        );

    file.seekg(
        static_cast<std::streamoff>(
            sizeof(DatabaseHeader)
        ),
        std::ios::beg
    );

    if (!file) {
        std::cout
            << "Error: Could not seek to database pages."
            << std::endl;

        std::exit(EXIT_FAILURE);
    }

    for (uint32_t i = 0; i < pageCount; i++) {
        Page page = Page::deserialize(file);

        if (!file) {
            std::cout
                << "Error: Could not load page."
                << std::endl;

            std::exit(EXIT_FAILURE);
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

        std::exit(EXIT_FAILURE);
    }

    DatabaseHeader header = {
        {'S', 'M', 'D', 'B'},
        1,
        Page::maxSize
    };

    file.write(
        reinterpret_cast<const char*>(&header),
        sizeof(DatabaseHeader)
    );

    if (!file) {
        std::cout
            << "Error: Could not write database header."
            << std::endl;

        std::exit(EXIT_FAILURE);
    }

    for (const Page& page : pages) {
        page.serialize(file);

        if (!file) {
            std::cout
                << "Error: Could not save page."
                << std::endl;

            std::exit(EXIT_FAILURE);
        }
    }
}