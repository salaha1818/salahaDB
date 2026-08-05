#pragma once

#include <vector>
#include "Record.h"

// stores records inside one fixed-size database page
class Page {
private:
    uint32_t pageID;
    uint32_t usedSize;
    std::vector<Record> records;

    // space used by the page id, used size, and record count
    static constexpr uint32_t HEADER_SIZE =
        sizeof(uint32_t) +
        sizeof(uint32_t) +
        sizeof(uint32_t);

public:
    // every page is limited to 4096 bytes
    static constexpr uint32_t maxSize = 4096;

    Page(uint32_t id);

    // checks if another record can fit in the page
    bool canFit(const Record& record) const;

    // adds a record and returns the slot it was stored in
    bool addRecord(const Record& record, size_t& slotIndex);

    uint32_t getUsedSize() const;
    uint32_t getMaxSize() const;
    uint32_t getPageID() const;
    uint32_t getFreeSpace() const;

    // lets the database modify records in the page
    std::vector<Record>& getRecords();

    // read-only access to the records
    const std::vector<Record>& getRecords() const;

    // writes and reads a full page in binary
    void serialize(std::ostream& output) const;
    static Page deserialize(std::istream& input);

    // replaces a record and updates the used page space
    bool updateRecord(
        size_t slotIndex,
        const Record& updatedRecord
    );
};