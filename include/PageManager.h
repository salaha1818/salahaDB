#pragma once

#include <vector>
#include "Record.h"
#include "Page.h"

class PageManager {
    private:
        std::vector<Page> pages;
        size_t pageSize = Page::maxSize;
        void newPage();
    public:
        PageManager();
        void clear();
        bool insertRecord (const Record & record, size_t& pageIndex, size_t& slotIndex);
        const Page& getPage (size_t id) const;
        std::vector<Page>& getPages();
        const std::vector<Page>& getPages() const;
        void loadPages(const std::vector<Page>& loadedPages);

        std::vector<Record> getAllRecords() const;
        void compact();
};