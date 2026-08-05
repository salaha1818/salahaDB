#include <iostream>
#include <vector>
#include "Page.h"
#include "Parser.h"
#include "PageManager.h"

// starts the database with one empty page
PageManager::PageManager(){
    pages.emplace_back(0);
}

// adds a new page using the next page id
void PageManager::newPage() {
    int id = pages.size();
    pages.emplace_back(id);
}

// removes every page from memory
void PageManager::clear() {
    pages.clear();
}

// tries each page until one has enough space for the record
bool PageManager::insertRecord(
    const Record& record,
    size_t& pageIndex,
    size_t& slotIndex
) {
    for(int i = 0; i < pages.size(); i++) {
        if(pages[i].addRecord(record, slotIndex)) {
            pageIndex = i;
            return true;
        }
    }

    // create a new page if none of the current pages have space
    newPage();

    pageIndex = pages.size() - 1;

    return pages.back().addRecord(record, slotIndex);
}

// returns one page by index
const Page& PageManager::getPage(size_t id) const {
    return pages.at(id);
}

// modifiable access to all pages
std::vector<Page>& PageManager::getPages() {
    return pages;
}

// read-only access to all pages
const std::vector<Page>& PageManager::getPages() const {
    return pages;
}

// replace the current pages with the pages loaded from disk
void PageManager::loadPages(
    const std::vector<Page>& loadedPages
) {
    pages = loadedPages;
}

// combines the records from every page into one vector
std::vector<Record> PageManager::getAllRecords() const {
    std::vector<Record> allRecords;

    for (const auto& page : pages) {
        const std::vector<Record>& pageRecords =
            page.getRecords();

        for (const auto& record : pageRecords) {
            allRecords.push_back(record);
        }
    }

    return allRecords;
}

// removes deleted records and rebuilds the pages
void PageManager::compact() {
    std::vector<Record> activeRecords;

    // keep only records that have not been deleted
    for (const Page& page : pages) {
        for (const Record& record : page.getRecords()) {
            if (!record.deleted) {
                activeRecords.push_back(record);
            }
        }
    }

    // remove the old pages that still contain tombstones
    pages.clear();

    // place the active records into new pages
    for (const Record& record : activeRecords) {
        size_t pageIndex;
        size_t slotIndex;

        insertRecord(
            record,
            pageIndex,
            slotIndex
        );
    }
}