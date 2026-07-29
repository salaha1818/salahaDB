#include <iostream>
#include <cstdint>
#include <vector>
#include "Page.h"
#include "Serializer.h"

Page::Page(uint32_t id) {
    pageID = id;
    usedSize = HEADER_SIZE;
    maxSize;
}

bool Page::canFit (const Record & record) const {
    return usedSize + Serializer::getRecordSize(record) <= maxSize;
}

bool Page::addRecord(const Record & record, int& slotIndex) {
    if(!canFit(record)){
        return false;
    }
    records.push_back(record);
    slotIndex = records.size() - 1;
    usedSize += Serializer::getRecordSize(record);
    return true;
}

uint32_t Page::getFreeSpace() const {
    return maxSize - usedSize;
}

uint32_t Page::getUsedSize() const {
    return usedSize;
}

uint32_t Page::getMaxSize() const {
    return maxSize;
}

uint32_t Page::getPageID() const {
    return pageID;
}

// modifiable
std::vector<Record>& Page::getRecords() {
    return records;
}

// read only
const std::vector<Record>& Page::getRecords() const {
    return records;
}

void Page::serialize(std::ostream& output) const {
    uint32_t recordCount =
        static_cast<uint32_t>(records.size());
    
    // write page id
    output.write(
        reinterpret_cast<const char*>(&pageID),
        sizeof(pageID)
    );

    // write num of bytes used
    output.write(
        reinterpret_cast<const char*>(&usedSize),
        sizeof(usedSize)
    );

    // write record count
    output.write(
        reinterpret_cast<const char*>(&recordCount),
        sizeof(recordCount)
    );

    for(const auto& record:records) {
        Serializer::serializeBinary(record, output);
    }

    // write 0's in the empty space
    uint32_t paddingSize = maxSize - usedSize;

    std::vector<char> padding(
        paddingSize,
        0
    );

    output.write(
        padding.data(),
        padding.size()
    );
}

Page Page::deserialize(std::istream& input) {
    uint32_t loadedPageID;
    uint32_t loadedUsedSize;
    uint32_t recordCount;
    int slotIndex;

    // read page header info
    input.read(
        reinterpret_cast<char*>(&loadedPageID),
        sizeof(loadedPageID)
    );

    input.read(
        reinterpret_cast<char*>(&loadedUsedSize),
        sizeof(loadedUsedSize)
    );

    input.read(
        reinterpret_cast<char*>(&recordCount),
        sizeof(recordCount)
    );

    if (!input) {
        std::cerr
            << "Error: Could not read page header."
            << std::endl;

        return Page(0);
    }

    Page page(loadedPageID);

    // check for usedsize greater than max size or less than min
    if (loadedUsedSize < HEADER_SIZE ||
        loadedUsedSize > page.getMaxSize()) {

        std::cerr
            << "Error: Invalid page size."
            << std::endl;

        return page;
    }

    // deserialize records
    for (uint32_t i = 0; i < recordCount; i++) {
        Record record =
            Serializer::deserializeBinary(input);

        if (!input) {
            std::cerr
                << "Error: Could not read page record."
                << std::endl;

            return page;
        }

        if (!page.addRecord(record, slotIndex)) {
            std::cerr
                << "Error: Record does not fit in page."
                << std::endl;

            return page;
        }
    }

    // check for incorrect used size
    if (page.getUsedSize() != loadedUsedSize) {
        std::cerr
            << "Warning: Page size mismatch."
            << std::endl;
    }

    // ignore empty space (zeroes)
    uint32_t paddingSize =
        page.getMaxSize() - loadedUsedSize;

    input.ignore(paddingSize);

    return page;
}


