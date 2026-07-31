#include <iostream>
#include <vector>
#include "Page.h"
#include "Parser.h"
#include "PageManager.h"

PageManager::PageManager(){
    pages.emplace_back(0);
}

void PageManager::newPage() {
    int id = pages.size();
    pages.emplace_back(id);
}

void PageManager::clear() {
    pages.clear();
    newPage();
}

bool PageManager::insertRecord (const Record & record, size_t& pageIndex, size_t& slotIndex){
    for(int i = 0; i < pages.size(); i++) {
        if(pages[i].addRecord(record, slotIndex)){
            pageIndex = i;
            return true;
        }
    }
    newPage();
    pageIndex = pages.size() - 1;
    return pages.back().addRecord(record, slotIndex);
}

const Page& PageManager::getPage (size_t id) const {
    return pages.at(id);
}

// modifiable
std::vector<Page>& PageManager::getPages() {
    return pages;
}

// read only
const std::vector<Page>& PageManager::getPages() const {
    return pages;
}

void PageManager::loadPages(const std::vector<Page>& loadedPages){
    pages = loadedPages;
}

std::vector<Record> PageManager::getAllRecords() const {
    std::vector<Record> allRecords;

    for (const auto& page : pages) {
        const std::vector<Record>& pageRecords = page.getRecords();

        for (const auto& record : pageRecords) {
            allRecords.push_back(record);
        }
    }

    return allRecords;
}

void PageManager::compact() {
    std::vector<Record> activeRecords;

    for (const Page& page : pages) {
        for (const Record& record : page.getRecords()) {
            if (!record.deleted) {
                activeRecords.push_back(record);
            }
        }
    }

    pages.clear();

    for (const Record& record : activeRecords) {
        size_t pageIndex;
        size_t slotIndex;

        insertRecord(record, pageIndex, slotIndex);
    }
}