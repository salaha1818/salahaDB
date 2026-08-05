#pragma once

#include <vector>
#include "Record.h"
#include "Page.h"

// manages all pages in the database
class PageManager {
    private:
        std::vector<Page> pages;
        size_t pageSize = Page::maxSize;

        // creates a new empty page
        void newPage();

    public:
        PageManager();

        // removes all pages
        void clear();

        // finds a page with space and inserts the record
        bool insertRecord(
            const Record& record,
            size_t& pageIndex,
            size_t& slotIndex
        );

        // gets one page by its index
        const Page& getPage(size_t id) const;

        // modifiable access to all pages
        std::vector<Page>& getPages();

        // read-only access to all pages
        const std::vector<Page>& getPages() const;

        // replaces the current pages with pages loaded from disk
        void loadPages(const std::vector<Page>& loadedPages);

        // returns every record from every page
        std::vector<Record> getAllRecords() const;

        // rebuilds the pages using only active records
        void compact();
};