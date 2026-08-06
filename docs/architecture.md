# salahaDB Architecture

**Version:** Current
**Language:** C++20
**Storage:** Custom Binary File Format
**Author:** Salaha Mukhtar

---

# 1. Project Overview

salahaDB is a lightweight database engine implemented from scratch in C++20. It was created to explore how database systems manage command parsing, records, pages, indexes, serialization, and persistent storage without relying on an existing database library such as SQLite.

The project prioritizes correctness, modularity, maintainability, and educational value over production-level optimization.

## Core Features

* SQL-inspired command parser
* Persistent binary storage
* Fixed-size 4096-byte pages
* B+ Tree primary index
* Hash-based secondary indexes
* Binary serialization and deserialization
* Variable-length record names
* Soft deletion
* Database compaction with `VACUUM`
* Interactive and script execution modes
* Database statistics
* File validation and corruption detection

## High-Level Architecture

```text
                User
                  │
                  ▼
              main.cpp
                  │
                  ▼
                Parser
                  │
                  ▼
               Command
                  │
                  ▼
               Database
       ┌──────────┼───────────┐
       ▼          ▼           ▼
 PageManager   BPlusTree   Hash Indexes
       │
       ▼
      Pages
       │
       ▼
   Serializer
       │
       ▼
 StorageEngine
       │
       ▼
    salaha.db
```

Each component has one primary responsibility:

* `main.cpp` handles user interaction and execution modes.
* `Parser` validates SQL-inspired input.
* `Command` stores validated command information.
* `Database` coordinates database operations.
* `PageManager` manages pages in memory.
* `Page` stores records inside fixed-size blocks.
* `Serializer` converts records to and from binary data.
* `StorageEngine` reads and writes the database file.
* `BPlusTree` provides primary-key and range lookups.
* Secondary hash indexes accelerate searches by name and age.

---

# 2. Core Data Structures

## Record

A `Record` represents one row in the database.

```cpp
struct Record {
    uint32_t id;
    std::string name;
    uint32_t age;
    bool deleted;
};
```

| Field     | Purpose                                      |
| --------- | -------------------------------------------- |
| `id`      | Unique identifier assigned by the database   |
| `name`    | User-provided variable-length name           |
| `age`     | User-provided age                            |
| `deleted` | Indicates whether the record is soft-deleted |

`Record` only stores data. Parsing, indexing, serialization, and persistence are handled by other components.

## Command

A `Command` represents validated user input.

```cpp
struct Command {
    std::string type;
    std::vector<std::string> arguments;
};
```

For example:

```sql
UPDATE records SET name = Alice age = 22 WHERE id = 5
```

is converted into a command containing:

```text
type = "update"
arguments = ["5", "Alice", "22"]
```

The `Database` receives validated `Command` objects rather than raw input.

## RecordLocation

A `RecordLocation` identifies where a record is stored.

```cpp
struct RecordLocation {
    size_t pageIndex;
    size_t slotIndex;
};
```

The primary index stores record locations instead of pointers or duplicate record objects.

```text
Record ID
    ↓
B+ Tree
    ↓
RecordLocation
    ↓
Page and Slot
    ↓
Record
```

Using page and slot indexes avoids dangling pointers when pages are reloaded or rebuilt.

## DatabaseHeader

Every database file begins with a fixed-size header.

```cpp
struct DatabaseHeader {
    char magic[4];
    uint32_t version;
    uint32_t pageSize;
};
```

The header allows the `StorageEngine` to validate:

* Magic number
* Database version
* Page size
* Overall file size

Invalid or incompatible database files are rejected before their pages are loaded.

---

# 3. Class Responsibilities

| Component        | Responsibility                                   |
| ---------------- | ------------------------------------------------ |
| `Record`         | Stores one database row                          |
| `Command`        | Stores one validated command                     |
| `Parser`         | Parses and validates SQL-inspired input          |
| `Database`       | Executes commands and coordinates all subsystems |
| `Page`           | Stores records inside one 4096-byte block        |
| `PageManager`    | Creates, retrieves, and compacts pages           |
| `Serializer`     | Converts records to and from binary data         |
| `StorageEngine`  | Reads, writes, and validates database files      |
| `BPlusTree`      | Maps record IDs to record locations              |
| `RecordLocation` | Identifies a record's page and slot              |

## Parser

The `Parser`:

* Tokenizes user input
* Validates command syntax
* Rejects malformed commands
* Produces `Command` objects
* Handles commands case-insensitively

It does not access records, pages, indexes, or storage.

## Database

The `Database` is the central coordinator. It handles:

* `INSERT`
* `SELECT`
* `UPDATE`
* `DELETE`
* `FIND`
* `VACUUM`
* `DROP DATABASE CONFIRM`
* `STATS`
* `HELP`
* `EXIT`

It also manages record IDs, updates indexes, coordinates page operations, and saves changes.

The next record ID is determined during startup:

```text
nextID = largest existing ID + 1
```

## PageManager

The `PageManager` owns all pages loaded into memory. It:

* Creates new pages
* Searches for available page space
* Inserts records
* Retrieves pages and records
* Rebuilds pages during `VACUUM`

If no existing page can fit a record, it creates a new page.

## StorageEngine

The `StorageEngine` is the only component that directly opens the database file. It:

* Loads the database header
* Validates the file
* Loads and saves pages
* Writes page padding
* Reports file read and write failures

This keeps file I/O separate from command execution and record management.

---

# 4. Storage Architecture

salahaDB stores records in a custom binary file made of fixed-size pages.

## File Layout

```text
+----------------------+
| Database Header      |
+----------------------+
| Page 0 - 4096 bytes  |
+----------------------+
| Page 1 - 4096 bytes  |
+----------------------+
| Page 2 - 4096 bytes  |
+----------------------+
| ...                  |
+----------------------+
```

The total database size is:

```text
Database header + (number of pages × 4096 bytes)
```

## Page Layout

Each page occupies exactly 4096 bytes.

```text
+--------------------------------+
| Page ID                        |
+--------------------------------+
| Used Size                      |
+--------------------------------+
| Record Count                   |
+--------------------------------+
| Serialized Records             |
+--------------------------------+
| Zero Padding                   |
+--------------------------------+
```

A page tracks its used space so that it can quickly determine whether another record will fit.

Before insertion:

```text
used size + new record size <= 4096
```

Before an update:

```text
new used size =
current used size
- old record size
+ updated record size
```

The update is rejected if the new record would overflow the page.

## Binary Record Layout

Each record is serialized in the following order:

```text
+-----------------------+
| ID - 4 bytes          |
+-----------------------+
| Age - 4 bytes         |
+-----------------------+
| Deleted Flag - 1 byte |
+-----------------------+
| Name Length - 4 bytes |
+-----------------------+
| Name Characters       |
+-----------------------+
```

Storing the name length before the characters allows names to have different lengths without using a fixed-size string buffer.

## Persistence Strategy

Every successful modifying command saves the database immediately:

* `INSERT`
* `UPDATE`
* `DELETE`
* `VACUUM`
* `DROP DATABASE CONFIRM`

This approach increases disk writes but reduces the risk of losing completed operations if the application closes unexpectedly.

---

# 5. Indexing System

salahaDB maintains its indexes in memory.

```text
                Database
                    │
        ┌───────────┴────────────┐
        ▼                        ▼
 Primary Index             Secondary Indexes
    B+ Tree                  Hash Tables
        │                        │
        ▼                        ▼
 RecordLocation          Vectors of Record IDs
```

## Primary B+ Tree Index

The B+ Tree maps:

```text
Record ID → RecordLocation
```

It is used by:

* `FIND`
* `UPDATE`
* `DELETE`
* ID range queries

The B+ Tree was selected because it supports ordered keys, logarithmic searches, and efficient range queries.

Internal nodes store separator keys and child pointers. Leaf nodes store IDs and record locations. Leaf nodes are linked together, allowing range queries to continue through adjacent leaves without restarting the search.

Example:

```text
SELECT * FROM records WHERE id BETWEEN 50 AND 100
```

Execution:

```text
Locate the first matching leaf
    ↓
Traverse linked leaves
    ↓
Stop after ID 100
```

## Secondary Hash Indexes

Two `std::unordered_map` indexes are maintained:

```text
Name → vector<Record IDs>
Age  → vector<Record IDs>
```

Names and ages are not unique, so each key can reference multiple record IDs.

The IDs are resolved through the primary B+ Tree:

```text
Name or Age
    ↓
Matching IDs
    ↓
B+ Tree
    ↓
RecordLocation
    ↓
Record
```

This avoids storing duplicate copies of records inside the indexes.

## Index Maintenance

### Insert

```text
Insert record into page
    ↓
Add ID to B+ Tree
    ↓
Add ID to name index
    ↓
Add ID to age index
```

### Update

```text
Locate record
    ↓
Remove old secondary index entries
    ↓
Update record
    ↓
Insert new secondary index entries
```

### Delete

```text
Locate record
    ↓
Remove index entries
    ↓
Set deleted = true
```

### Startup and VACUUM

Indexes are rebuilt by scanning all active records whenever the database is loaded or compacted.

---

# 6. Database Operations

All commands follow the same general pipeline:

```text
User Input
    ↓
Parser
    ↓
Command
    ↓
Database
    ↓
Pages and Indexes
    ↓
StorageEngine
    ↓
Disk
```

## INSERT

```text
Validate command
    ↓
Assign next ID
    ↓
Create record
    ↓
Find or create a page
    ↓
Insert record
    ↓
Update indexes
    ↓
Save database
```

## FIND

```text
Search B+ Tree
    ↓
Retrieve RecordLocation
    ↓
Access page and slot
    ↓
Return record
```

No full table scan is required.

## SELECT

### Select All

Scans all pages, skips soft-deleted records, and prints active records.

### Filter by Name or Age

Uses the appropriate hash index to retrieve matching IDs, then resolves each ID through the B+ Tree.

### ID Range

Uses the B+ Tree's ordered leaf nodes to retrieve records within the requested range.

### Order By

Copies active records into a temporary vector, sorts them using `std::sort()`, and prints the result. Stored record order is not changed.

## UPDATE

```text
Locate record using B+ Tree
    ↓
Verify updated record fits page
    ↓
Remove old secondary index values
    ↓
Replace record
    ↓
Insert new secondary index values
    ↓
Save database
```

## DELETE

Deleting a record performs a soft deletion.

```text
Locate record
    ↓
Remove index entries
    ↓
Set deleted = true
    ↓
Save database
```

The record continues occupying page space until `VACUUM` is executed.

## VACUUM

```text
Collect active records
    ↓
Remove existing pages
    ↓
Create new pages
    ↓
Reinsert active records
    ↓
Rebuild indexes
    ↓
Save database
```

This permanently removes deleted records and compacts storage.

## DROP DATABASE

```text
Clear pages
    ↓
Clear indexes
    ↓
Reset nextID
    ↓
Save empty database
```

## STATS

The `STATS` command scans the loaded pages and reports information such as:

* Total records
* Active records
* Deleted records
* Page count
* Page size
* Estimated database size

---

# 7. Design Decisions and Trade-offs

## Binary Storage

Binary storage was selected instead of plain text because it provides:

* Smaller files
* Predictable record layouts
* Fixed-size pages
* Better validation
* A design closer to real storage engines

The trade-off is that database files are less human-readable.

## Fixed-Size Pages

Pages simplify space tracking, storage organization, and future caching. They also isolate many changes to a single storage block.

The current page size is fixed at 4096 bytes.

## Separate Serialization and File I/O

The `Serializer` handles binary formats, while the `StorageEngine` handles files.

This allows the binary record layout to change without placing serialization logic throughout the storage code.

## Record Locations Instead of Pointers

Indexes store page and slot numbers rather than memory addresses.

This prevents invalid references when pages are reloaded, compacted, or reconstructed.

## B+ Tree and Hash Indexes

Different index types are used for different query requirements:

* B+ Tree for unique, ordered IDs and range queries
* Hash tables for fast equality searches on non-unique names and ages

## Soft Deletion

Soft deletion makes deletes faster and avoids immediately reorganizing page contents.

The trade-off is unused storage space, which is reclaimed through `VACUUM`.

## Immediate Saving

Saving after every modification prioritizes reliability and simplicity.

The trade-off is reduced performance during workloads containing many writes.

## Modular Responsibilities

Each class performs one primary task. This reduces coupling and allows individual components to be modified or tested without rewriting the entire system.

---

# 8. Complexity Analysis

Let:

* **n** be the total number of records
* **k** be the number of matching records
* **p** be the number of pages

## Command Complexity

| Command        | Complexity         | Reason                                   |
| -------------- | ------------------ | ---------------------------------------- |
| `INSERT`       | `O(p + log n)`     | Finds page space and updates indexes     |
| `FIND`         | `O(log n)`         | Uses the B+ Tree                         |
| `UPDATE`       | `O(log n)`         | Locates record and updates indexes       |
| `DELETE`       | `O(log n)`         | Locates record and removes index entries |
| `SELECT *`     | `O(n)`             | Scans all records                        |
| ID range query | `O(log n + k)`     | Finds first match and traverses leaves   |
| Name query     | `O(1 + k)` average | Uses the name hash index                 |
| Age query      | `O(1 + k)` average | Uses the age hash index                  |
| `ORDER BY`     | `O(n log n)`       | Sorts a temporary record vector          |
| `VACUUM`       | `O(n log n)`       | Rebuilds pages and indexes               |
| `STATS`        | `O(n)`             | Scans all records                        |

The `PageManager` currently searches pages linearly to locate available space, giving page selection a complexity of `O(p)`.

## Memory Usage

The entire database and all indexes are loaded into memory at startup.

Memory usage includes:

* Pages
* Records
* B+ Tree nodes
* Secondary hash indexes

This simplifies implementation and improves lookup performance, but limits scalability for very large databases.

---

# 9. Current Limitations

salahaDB currently has the following limitations:

* Supports one table
* No joins
* No transactions
* No concurrency control
* No write-ahead logging
* Entire database is loaded into memory
* Fixed 4096-byte page size
* In-memory indexes
* Single-threaded command execution
* Linear search for pages with available space

These limitations keep the implementation focused on the core storage, indexing, and command-processing architecture.

---

# 10. Implementation Notes

This section records the lower-level behavior that is most useful when returning to the project after a long period of time.

---

## Database Startup

When salahaDB starts, it reconstructs the database and all in-memory indexes from the binary file.

```text
Open salaha.db
    ↓
Read DatabaseHeader
    ↓
Validate magic number, version, and page size
    ↓
Verify the remaining file size contains complete pages
    ↓
Read each 4096-byte page
    ↓
Deserialize the records stored in each page
    ↓
Load pages into PageManager
    ↓
Scan all active records
    ↓
Rebuild primary and secondary indexes
    ↓
Find the largest existing record ID
    ↓
Set nextID = largest ID + 1
```

If no database file exists, the program starts with an empty database.

If the header, page size, version, or file structure is invalid, loading stops rather than attempting to use potentially corrupted data.

Indexes are not stored permanently in the database file. They are rebuilt from the records whenever the database is loaded.

---

## Record Lookup Process

The B+ Tree does not store complete records. It maps record IDs to `RecordLocation` values.

```text
Record ID
    ↓
B+ Tree search
    ↓
RecordLocation
    ↓
pageIndex and slotIndex
    ↓
PageManager retrieves the page
    ↓
Page retrieves the record
```

A `RecordLocation` contains:

```cpp
struct RecordLocation {
    size_t pageIndex;
    size_t slotIndex;
};
```

Using page and slot indexes instead of memory pointers prevents the index from becoming invalid when pages are reloaded or rebuilt.

---

## B+ Tree Search

A B+ Tree search begins at the root.

```text
Start at root
    ↓
Compare the target ID with separator keys
    ↓
Follow the matching child
    ↓
Repeat until reaching a leaf
    ↓
Search the leaf for the ID
```

Internal nodes direct searches, while leaf nodes contain record IDs and their corresponding `RecordLocation` values.

---

## B+ Tree Insertion and Splitting

When a new record is inserted:

```text
Locate the correct leaf
    ↓
Insert the ID in sorted order
    ↓
Node exceeds its allowed capacity?
    ├── No → insertion is complete
    └── Yes
          ↓
       Split the node
          ↓
       Promote a separator key to the parent
          ↓
       Parent exceeds capacity?
          ├── No → insertion is complete
          └── Yes → split the parent
```

Splitting may continue upward through the tree.

If the root splits, a new root is created and the tree increases in height.

Leaf nodes remain linked using `nextLeaf`, which allows range queries to continue directly from one leaf to the next.

---

## B+ Tree Deletion and Rebalancing

When a record is deleted, its ID is removed from the B+ Tree.

```text
Locate the leaf containing the ID
    ↓
Remove the ID and RecordLocation
    ↓
Node still satisfies the minimum size?
    ├── Yes → deletion is complete
    └── No
          ↓
       Try borrowing from the left sibling
          ↓
       If unavailable, try the right sibling
          ↓
       If borrowing is not possible, merge nodes
          ↓
       Update the parent separator keys
```

If a merge causes the parent to fall below its minimum size, the same borrowing or merging process continues upward.

If the root is left with only one child, that child becomes the new root.

The exact node-capacity rules are defined by the B+ Tree implementation.

---

## B+ Tree Range Queries

An ID range query uses the linked leaf nodes.

```text
Search for the minimum ID
    ↓
Locate the first matching leaf
    ↓
Read matching entries from that leaf
    ↓
Follow nextLeaf
    ↓
Continue until an ID exceeds the maximum
```

This gives the range query a complexity of:

```text
O(log n + k)
```

where `k` is the number of matching records.

---

## Index Consistency Rules

The database maintains three indexes:

```text
Primary:
ID → RecordLocation

Secondary:
Name → vector<Record IDs>
Age  → vector<Record IDs>
```

The physical record exists only inside a page. Indexes store IDs or record locations rather than duplicate record objects.

### INSERT

```text
Insert record into a page
    ↓
Determine its page and slot location
    ↓
Insert ID and RecordLocation into the B+ Tree
    ↓
Insert ID into the name index
    ↓
Insert ID into the age index
    ↓
Save database
```

### UPDATE

The old secondary-index values must be removed before the new values are added.

```text
Locate record using the B+ Tree
    ↓
Save the old name and age
    ↓
Verify the updated record still fits inside the page
    ↓
Remove ID from the old name index entry
    ↓
Remove ID from the old age index entry
    ↓
Replace the record inside the page
    ↓
Insert ID into the new name index entry
    ↓
Insert ID into the new age index entry
    ↓
Save database
```

The primary index does not need to change if the record remains in the same page and slot.

If an update were ever changed to move records between pages, the B+ Tree's `RecordLocation` would also need to be updated.

### DELETE

```text
Locate record using the B+ Tree
    ↓
Remove ID from the B+ Tree
    ↓
Remove ID from the name index
    ↓
Remove ID from the age index
    ↓
Set deleted = true
    ↓
Save database
```

The record remains physically inside its page but is no longer reachable through the indexes.

Queries that scan pages must also check the `deleted` flag and skip deleted records.

### VACUUM

Record locations change during compaction, so all indexes must be rebuilt.

```text
Collect active records
    ↓
Clear all pages
    ↓
Clear the B+ Tree
    ↓
Clear the secondary indexes
    ↓
Reinsert active records into new pages
    ↓
Create new RecordLocations
    ↓
Rebuild every index
    ↓
Save database
```

### Database Loading

The same rebuilding process occurs during startup.

```text
Load and deserialize pages
    ↓
Clear indexes
    ↓
Scan all active records
    ↓
Insert each ID and location into the B+ Tree
    ↓
Insert each ID into the name and age indexes
```

Rebuilding ensures that the in-memory indexes always match the records stored in the file.

---

## Page Insertion

The `PageManager` searches pages in order until it finds enough available space.

```text
Check Page 0
    ↓
Record fits?
    ├── Yes → insert record
    └── No
          ↓
       Check next page
```

If no existing page has enough space:

```text
Create a new page
    ↓
Assign its page ID
    ↓
Insert the record
    ↓
Add the page to PageManager
```

The returned page and slot indexes are stored inside the B+ Tree as the record's `RecordLocation`.

---

## Record Size and Page Updates

The `Serializer` calculates the binary size of each record.

A record contains:

```text
ID              4 bytes
Age             4 bytes
Deleted flag    1 byte
Name length     4 bytes
Name            variable number of bytes
```

Before insertion:

```text
page used size + new record size <= 4096
```

Before update:

```text
new used size =
current used size
- old record size
+ updated record size
```

If the new used size exceeds 4096 bytes, the update is rejected.

The current implementation does not automatically move an oversized updated record to another page.

---

## Page Serialization

Every page must occupy exactly 4096 bytes in the database file.

```text
Write page header
    ↓
Write each serialized record
    ↓
Calculate unused page space
    ↓
Fill the remaining bytes with zeros
```

When loading, the page header tells the program how many records and how many used bytes the page contains.

Padding is not interpreted as record data.

---

## Persistence Behavior

salahaDB saves after every successful modifying command:

* `INSERT`
* `UPDATE`
* `DELETE`
* `VACUUM`
* `DROP DATABASE CONFIRM`

The save process is:

```text
Database operation succeeds
    ↓
Database calls save()
    ↓
StorageEngine writes the DatabaseHeader
    ↓
StorageEngine serializes and writes every page
    ↓
File is closed
```

Because every completed modification is written immediately, `EXIT` does not need to perform an additional database save.

This design prioritizes reliability and implementation simplicity over write performance.

A consequence is that a single small modification may cause every page to be written again.

---

## Soft Deletion Behavior

Deleting a record does not immediately reduce the database file size.

```text
deleted = true
```

The deleted record:

* Remains physically stored in its page
* Is removed from all indexes
* Is ignored by normal queries
* Continues occupying page space
* Is permanently removed only by `VACUUM`

This makes deletion simple and fast but causes unused space to accumulate over time.

---

## File Validation

Before loading records, the `StorageEngine` should verify:

* The file can be opened
* The complete database header is present
* The magic number identifies a salahaDB file
* The database version is supported
* The stored page size is 4096 bytes
* The remaining file size is divisible by the page size
* Every page can be deserialized successfully
* Record data does not exceed page boundaries

Validation occurs before the database begins normal command execution.

This prevents malformed or incompatible files from being treated as valid databases.

---

## Important Invariants

The following rules must remain true for the database to operate correctly:

1. Every active record has a unique ID.
2. Every active record has one B+ Tree entry.
3. Every B+ Tree location points to the correct page and slot.
4. Every active record appears in the correct name index.
5. Every active record appears in the correct age index.
6. Deleted records do not appear in any index.
7. Every page occupies exactly 4096 bytes on disk.
8. A page's used size never exceeds 4096 bytes.
9. `nextID` is greater than every existing record ID.
10. Indexes are rebuilt whenever page compaction changes record locations.

When debugging unexpected database behavior, these invariants should be checked first.
