#include <iostream>
#include <cstdint>
#include <vector>
#include "Page.h"
#include "Serializer.h"

Page::Page(uint32_t id) {
    pageID = id;

    // a new page only contains its header
    usedSize = HEADER_SIZE;

    maxSize;
}

// checks if the page has enough free space for the record
bool Page::canFit(const Record& record) const {
    return usedSize +
        Serializer::getRecordSize(record) <= maxSize;
}

// adds the record to the end of the page
bool Page::addRecord(
    const Record& record,
    size_t& slotIndex
) {
    if (!canFit(record)) {
        return false;
    }

    records.push_back(record);

    // the record's position inside the page
    slotIndex = records.size() - 1;

    usedSize += Serializer::getRecordSize(record);

    return true;
}

// returns how much space is still available
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

// modifiable access to the records
std::vector<Record>& Page::getRecords() {
    return records;
}

// read-only access to the records
const std::vector<Record>& Page::getRecords() const {
    return records;
}

// replaces a record and updates the space used by the page
bool Page::updateRecord(
    size_t slotIndex,
    const Record& updatedRecord
) {
    if (slotIndex >= records.size()) {
        return false;
    }

    // calculate the page size before and after the update
    size_t oldRecordSize =
        Serializer::getRecordSize(records[slotIndex]);

    size_t newRecordSize =
        Serializer::getRecordSize(updatedRecord);

    size_t newUsedSize =
        usedSize - oldRecordSize + newRecordSize;

    // reject the update if the new record no longer fits
    if (newUsedSize > maxSize) {
        return false;
    }

    records[slotIndex] = updatedRecord;
    usedSize = static_cast<uint32_t>(newUsedSize);

    return true;
}

// writes the full page into its fixed-size space in the file
void Page::serialize(std::ostream& output) const {
    std::streampos pageStart = output.tellp();

    uint32_t recordCount =
        static_cast<uint32_t>(records.size());

    // write the page header
    output.write(
        reinterpret_cast<const char*>(&pageID),
        sizeof(pageID)
    );

    output.write(
        reinterpret_cast<const char*>(&usedSize),
        sizeof(usedSize)
    );

    output.write(
        reinterpret_cast<const char*>(&recordCount),
        sizeof(recordCount)
    );

    // write every record after the header
    for (const auto& record : records) {
        Serializer::serializeBinary(record, output);
    }

    if (!output) {
        return;
    }

    std::streampos currentPosition = output.tellp();

    std::streamoff bytesWritten =
        currentPosition - pageStart;

    // the serialized page should never be larger than 4096 bytes
    if (bytesWritten >
        static_cast<std::streamoff>(maxSize)) {

        std::cerr
            << "Error: Serialized page exceeds maximum size."
            << std::endl;

        output.setstate(std::ios::failbit);
        return;
    }

    // fill the rest of the page with zeroes
    std::streamoff paddingSize =
        static_cast<std::streamoff>(maxSize) -
        bytesWritten;

    std::vector<char> padding(
        static_cast<size_t>(paddingSize),
        0
    );

    output.write(
        padding.data(),
        static_cast<std::streamsize>(padding.size())
    );
}

// rebuilds one page from the database file
Page Page::deserialize(std::istream& input) {
    uint32_t loadedPageID;
    uint32_t loadedUsedSize;
    uint32_t recordCount;
    size_t slotIndex = 0;

    // read the page header
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
        return Page(0);
    }

    Page page(loadedPageID);

    // make sure the stored page size is possible
    if (loadedUsedSize < HEADER_SIZE ||
        loadedUsedSize > page.getMaxSize()) {

        input.setstate(std::ios::failbit);
        return page;
    }

    // read each record stored in the page
    for (uint32_t i = 0; i < recordCount; i++) {
        Record record =
            Serializer::deserializeBinary(input);

        if (!input) {
            return page;
        }

        // addRecord rebuilds the records and used size
        if (!page.addRecord(record, slotIndex)) {
            input.setstate(std::ios::failbit);
            return page;
        }
    }

    // warn if the rebuilt size does not match the stored size
    if (page.getUsedSize() != loadedUsedSize) {
        std::cerr
            << "Warning: Page size mismatch. "
            << "Stored: " << loadedUsedSize
            << ", calculated: " << page.getUsedSize()
            << '\n';
    }

    // skip the empty zero-filled part of the page
    uint32_t paddingSize =
        page.getMaxSize() - loadedUsedSize;

    input.ignore(
        static_cast<std::streamsize>(paddingSize)
    );

    return page;
}