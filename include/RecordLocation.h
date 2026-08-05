#pragma once

#include <cstddef>

// stores where a record is located inside the database
struct RecordLocation {
    size_t pageIndex;
    size_t slotIndex;
};