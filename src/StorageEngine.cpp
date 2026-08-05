#include <vector>
#include "Record.h"
#include "Page.h"
#include "Serializer.h"
#include "StorageEngine.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

// saves the database file name
StorageEngine::StorageEngine(const std::string& filename)
    : filename(filename)
{}

// prints the error and stops the program
static void fatalError(const std::string& message) {
    std::cerr << "Error: " << message << '\n';
    std::exit(EXIT_FAILURE);
}

// reads all database pages from the file
std::vector<Page> StorageEngine::loadPages() {
    std::vector<Page> pages;

    std::ifstream file(filename, std::ios::binary);

    // missing file means this is a new database
    if (!file) {
        return pages;
    }

    DatabaseHeader header{};

    // read the database header
    file.read(
        reinterpret_cast<char*>(&header),
        sizeof(DatabaseHeader)
    );

    if (!file) {
        fatalError("Could not read database header.");
    }

    // make sure this is a salahaDB file
    if (header.magic[0] != 'S' ||
        header.magic[1] != 'M' ||
        header.magic[2] != 'D' ||
        header.magic[3] != 'B') {

        fatalError("Invalid database file.");
    }

    // only version 1 is currently supported
    if (header.version != 1) {
        fatalError("Unsupported database version.");
    }

    // saved pages must use the same page size
    if (header.pageSize != Page::maxSize) {
        fatalError("Page size mismatch.");
    }

    // move to the end to calculate the file size
    file.seekg(0, std::ios::end);

    if (!file) {
        fatalError("Could not determine database file size.");
    }

    std::streamoff fileSize = file.tellg();

    // the file must at least contain the header
    if (fileSize <
        static_cast<std::streamoff>(
            sizeof(DatabaseHeader)
        )) {

        fatalError("Invalid database header size.");
    }

    // everything after the header should be page data
    std::streamoff pageBytes =
        fileSize -
        static_cast<std::streamoff>(
            sizeof(DatabaseHeader)
        );

    // page data must be made of complete fixed-size pages
    if (pageBytes %
        static_cast<std::streamoff>(
            Page::maxSize
        ) != 0) {

        fatalError("Invalid database file size.");
    }

    // calculate how many pages are stored
    uint32_t pageCount =
        static_cast<uint32_t>(
            pageBytes /
            static_cast<std::streamoff>(
                Page::maxSize
            )
        );

    // move back to the start of the page data
    file.seekg(
        static_cast<std::streamoff>(
            sizeof(DatabaseHeader)
        ),
        std::ios::beg
    );

    if (!file) {
        fatalError("Could not seek to database pages.");
    }

    // load each page one at a time
    for (uint32_t i = 0; i < pageCount; i++) {
        Page page = Page::deserialize(file);

        if (!file) {
            fatalError("Could not load page.");
        }

        pages.push_back(page);
    }

    return pages;
}

// writes all database pages to the file
void StorageEngine::savePages(
    const std::vector<Page>& pages
) {
    // overwrite the old database file
    std::ofstream file(
        filename,
        std::ios::binary | std::ios::trunc
    );

    if (!file) {
        fatalError("Could not open database file for writing.");
    }

    // header used to identify and validate the file
    DatabaseHeader header = {
        {'S', 'M', 'D', 'B'},
        1,
        Page::maxSize
    };

    // write the database header first
    file.write(
        reinterpret_cast<const char*>(&header),
        sizeof(DatabaseHeader)
    );

    if (!file) {
        fatalError("Could not write database header.");
    }

    // write every page after the header
    for (const Page& page : pages) {
        page.serialize(file);

        if (!file) {
            fatalError("Could not write database page.");
        }
    }

    // make sure all buffered data reaches the file
    file.flush();

    if (!file) {
        fatalError("Could not finish writing database file.");
    }
}