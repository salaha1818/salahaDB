#pragma once

#include <vector>
#include "Record.h"

class Page {
private:
    uint32_t pageID;
    uint32_t usedSize;
    std::vector<Record> records;

    static constexpr uint32_t HEADER_SIZE =
        sizeof(uint32_t) +
        sizeof(uint32_t) +
        sizeof(uint32_t);

public:
    static constexpr uint32_t maxSize = 4096;
    Page(uint32_t id);

    bool canFit(const Record& record) const;
    bool addRecord(const Record& record, size_t& slotIndex);

    uint32_t getUsedSize() const;
    uint32_t getMaxSize() const;
    uint32_t getPageID() const;
    uint32_t getFreeSpace() const;

    std::vector<Record>& getRecords();
    const std::vector<Record>& getRecords() const;

    void serialize(std::ostream& output) const;
    static Page deserialize(std::istream& input);
    bool updateRecord(size_t slotIndex, const Record& updatedRecord);
};