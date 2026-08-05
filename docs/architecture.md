# salahaDB Architecture

Version: Current  
Language: C++20  
Storage: Custom Binary File Format  
Author: Salaha Mukhtar

---

# 1. Project Overview

## Introduction

salahaDB is a lightweight relational database engine implemented entirely in C++. The project was built from scratch to explore how modern database systems work internally rather than relying on an existing database library such as SQLite.

The primary objective of the project was to understand the core components of a database engine by implementing them manually. This includes command parsing, binary storage, page management, indexing, record serialization, and persistent storage.

Although salahaDB is intentionally much smaller than production database systems, its architecture follows many of the same ideas used by commercial database engines.

Current features include:

- SQL-like command parser
- Persistent binary storage
- Fixed-size page management
- B+ Tree primary index
- Secondary hash indexes
- Binary serialization
- Soft deletion
- Database compaction (VACUUM)
- Interactive command mode
- Script execution mode
- Database statistics
- Corruption detection during loading

---

# Project Goals

The project was designed with the following goals:

- Learn how databases organize data internally.
- Separate responsibilities across independent classes.
- Store data in a binary file rather than plain text.
- Implement indexing to avoid linear searches.
- Simulate page-based storage used by real database systems.
- Keep the implementation readable and modular.
- Build a project suitable for demonstrating systems programming skills during software engineering interviews.

Performance was considered throughout the implementation, but correctness, maintainability, and educational value were prioritized over aggressive optimization.

---

# High-Level Architecture

Every command follows the same general execution path.

```
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
      ┌─────────┼──────────┐
      ▼         ▼          ▼
 PageManager  B+ Tree  Hash Indexes
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

Each component has a single responsibility.

- `main.cpp` handles user interaction.
- `Parser` validates SQL-like commands.
- `Database` coordinates all operations.
- `PageManager` manages memory pages.
- `Page` stores records.
- `Serializer` converts records to binary.
- `StorageEngine` reads and writes the database file.
- `BPlusTree` provides fast primary-key lookups.
- Secondary hash indexes accelerate searches by name and age.

Keeping each component independent makes the system easier to understand, test, and extend.

---

# Overall Command Flow

Regardless of the command being executed, salahaDB follows the same high-level workflow.

For example, the command

```
INSERT INTO records VALUES Alice 22
```

passes through the following stages:

```
User Input
      │
      ▼
main.cpp
      │
      ▼
Parser
      │
      ▼
Command Object
      │
      ▼
Database::execute()
      │
      ▼
Database::insert()
      │
      ▼
PageManager
      │
      ▼
Page
      │
      ▼
Update Indexes
      │
      ▼
StorageEngine
      │
      ▼
salaha.db
```

The parser is responsible only for validating syntax.

The database is responsible for executing commands.

The storage engine is responsible only for persistence.

Each class performs one clearly defined task, reducing coupling between different parts of the project.

---

# Major Components

salahaDB consists of ten primary components.

| Component | Responsibility |
|-----------|----------------|
| Record | Represents one database row |
| Command | Represents one validated command |
| Parser | Parses and validates SQL-like input |
| Database | Coordinates all database operations |
| Page | Stores records inside fixed-size pages |
| PageManager | Manages all pages in memory |
| Serializer | Converts records to and from binary format |
| StorageEngine | Reads and writes database pages to disk |
| BPlusTree | Primary index used for ID lookups |
| RecordLocation | Stores a record's page and slot location |

Each component is discussed in detail in later sections.

---

# 2. Core Data Structures

The entire database is built around a small number of data structures. Each one has a single responsibility and is used throughout the system.

---

## Record

The `Record` structure represents one row stored in the database.

```cpp
struct Record {
    uint32_t id;
    std::string name;
    uint32_t age;
    bool deleted;
};
```

### Fields

| Field | Purpose |
|--------|---------|
| id | Unique identifier assigned automatically by the database |
| name | User-provided name |
| age | User-provided age |
| deleted | Indicates whether the record has been soft deleted |

### Design Decisions

The `Record` structure only stores data. It intentionally contains no parsing, serialization, or storage logic.

Keeping the record as a plain data object makes it reusable throughout the project. Different components can manipulate records without depending on one another.

For example:

- The Parser never modifies records.
- The Serializer converts records into bytes.
- The Page stores records.
- The Database updates records.
- The StorageEngine writes serialized records to disk.

This separation follows the Single Responsibility Principle.

---

## Command

The `Command` structure represents a validated SQL-like command produced by the parser.

```cpp
struct Command {
    std::string type;
    std::vector<std::string> arguments;
};
```

### Fields

| Field | Purpose |
|--------|---------|
| type | Name of the command (insert, select, update, etc.) |
| arguments | Parsed command arguments |

### Example

User input

```
UPDATE records SET name = Alice age = 22 WHERE id = 5
```

becomes

```
type = "update"

arguments

5
Alice
22
```

The Database class never receives raw user input.

Instead, it always receives a validated `Command` object from the Parser.

This separates syntax validation from command execution.

---

## RecordLocation

The `RecordLocation` structure stores where a record exists inside the database.

```cpp
struct RecordLocation {
    size_t pageIndex;
    size_t slotIndex;
};
```

Instead of storing pointers to records, the B+ Tree stores `RecordLocation` objects.

This makes every record accessible using two numbers:

- Page number
- Slot number

Example

```
Record ID 37

↓

RecordLocation

pageIndex = 4

slotIndex = 8

↓

Page 4

↓

Record 8
```

Using locations instead of pointers avoids dangling references after pages are reloaded from disk.

---

## DatabaseHeader

Every database file begins with a fixed-size header.

```cpp
struct DatabaseHeader {
    char magic[4];
    uint32_t version;
    uint32_t pageSize;
};
```

### Fields

| Field | Purpose |
|--------|---------|
| magic | Identifies the file as a salahaDB database |
| version | Database format version |
| pageSize | Size of every page stored in the file |

The header allows salahaDB to detect corrupted or incompatible database files before loading any pages.

During startup, the StorageEngine verifies:

- Magic number
- Version
- Page size
- File size

If any validation fails, the database exits with an error instead of attempting to load corrupted data.

---

## Relationship Between Data Structures

The fundamental objects relate to one another as follows:

```
User Input
      │
      ▼
 Command
      │
      ▼
 Database
      │
      ▼
  Record
      │
      ▼
Serializer
      │
      ▼
Binary File
```

For indexed lookups:

```
Record ID

↓

B+ Tree

↓

RecordLocation

↓

Page

↓

Record
```

These simple structures form the foundation for every higher-level component in salahaDB.

# 3. Class Architecture

salahaDB follows a modular architecture in which each class has a single, well-defined responsibility.

Instead of allowing every class to communicate directly with one another, each component performs one task and exposes a simple interface to the rest of the system.

This separation keeps the project easier to understand, debug, and extend.

The following diagram shows how the major classes interact.

```
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
        ┌────────────┼──────────────┐
        ▼            ▼              ▼
   PageManager    BPlusTree   Secondary Indexes
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

The following sections describe each class individually.

---

# Record

## Purpose

The `Record` structure represents one row stored inside the database.

Every record consists of four fields:

- ID
- Name
- Age
- Deleted flag

The Record structure is intentionally simple and only stores data.

It contains no parsing logic, storage logic, or indexing logic.

Keeping Record independent allows it to be reused throughout the entire project.

---

## Responsibilities

The Record structure is responsible for storing:

- the record ID
- the user's name
- the user's age
- whether the record has been deleted

Nothing else.

---

## Used By

Record objects are used throughout the project.

```
Parser
     ×

Database
     ✓

Page
     ✓

Serializer
     ✓

StorageEngine
     ✓
```

The Parser never creates Record objects.

Instead, it creates Command objects.

The Database creates and modifies Records.

---

## Why separate Record from Storage?

A Record represents data.

The StorageEngine represents persistence.

Keeping them separate means the storage format can change without modifying the Record class.

For example:

```
Record

↓

Serializer

↓

Binary Bytes

↓

StorageEngine
```

This follows the Single Responsibility Principle.

---

# Command

## Purpose

The Command structure represents one validated SQL-like command.

Instead of passing raw strings through the program, the Parser converts every command into a Command object.

---

## Responsibilities

A Command stores

- command type
- command arguments

For example,

```
UPDATE records SET name = Alice age = 22 WHERE id = 5
```

becomes

```
type

update

arguments

5
Alice
22
```

The Database only works with Command objects.

It never parses user input itself.

---

## Why use Command objects?

Using Command objects separates parsing from execution.

Instead of writing code like

```
if(input == ...)
```

throughout the project, the Parser performs all syntax validation first.

Once a Command reaches the Database, it is already known to have valid syntax.

This greatly simplifies the Database implementation.

---

# Parser

## Purpose

The Parser converts raw user input into validated Command objects.

It acts as the first stage of the database pipeline.

```
User Input

↓

Parser

↓

Command
```

---

## Responsibilities

The Parser

- tokenizes input
- validates SQL-like syntax
- rejects malformed commands
- converts commands into Command objects

The Parser does **not**

- modify records
- access pages
- access indexes
- access storage

---

## Supported Commands

The parser currently recognizes

- INSERT
- SELECT
- UPDATE
- DELETE
- FIND
- VACUUM
- DROP DATABASE CONFIRM
- HELP
- STATS
- EXIT

The parser is also case-insensitive, allowing commands such as

```
insert

INSERT

InSeRt
```

to be treated identically.

---

## Design Decisions

The Parser performs validation as early as possible.

This follows the **fail fast** principle.

Invalid commands never reach the Database.

As a result, the Database only needs to verify argument values such as numeric conversions instead of checking SQL syntax.

---

# Database

## Purpose

The Database class is the central coordinator of salahaDB.

Every validated command eventually reaches the Database.

It owns the database state and coordinates communication between every other subsystem.

---

## Responsibilities

The Database class is responsible for

- INSERT
- SELECT
- UPDATE
- DELETE
- FIND
- VACUUM
- DROP
- HELP
- STATS

It also

- manages IDs
- maintains indexes
- coordinates page management
- saves changes
- loads existing databases

---

## Member Variables

### nextID

Stores the next available record ID.

During startup, the Database scans every loaded record to determine the highest existing ID.

The next inserted record receives

```
largestID + 1
```

This guarantees IDs remain unique across restarts.

---

### PageManager

Owns every page currently loaded into memory.

The Database never inserts records directly into Pages.

Instead, it asks the PageManager to find a page with enough free space.

---

### StorageEngine

Handles all disk input and output.

The Database never opens files directly.

Instead, it simply calls

```
save()

load()
```

and lets the StorageEngine handle persistence.

---

### BPlusTree

The primary index.

Maps

```
Record ID

↓

RecordLocation
```

This allows FIND, UPDATE, and DELETE to avoid scanning every record.

---

### Secondary Indexes

Two hash tables are maintained.

```
Age

↓

vector<IDs>
```

and

```
Name

↓

vector<IDs>
```

These indexes accelerate

```
SELECT WHERE age

SELECT WHERE name
```

without affecting the primary key index.

---

## Why make Database the coordinator?

Every subsystem has a different responsibility.

The Database coordinates them without allowing them to depend heavily on one another.

For example,

Pages never know about indexes.

The StorageEngine never knows about SQL commands.

The Parser never knows about pages.

This separation greatly reduces coupling.

# 4. Storage Architecture

Unlike many beginner database projects that store records directly in a text file, salahaDB stores data using fixed-size binary pages.

This closely follows the architecture used by modern database systems such as SQLite, PostgreSQL, and SQL Server, where data is organized into pages instead of individual records.

The storage subsystem is composed of four classes:

- Page
- PageManager
- Serializer
- StorageEngine

Each class performs one specific task.

```
Database
    │
    ▼
PageManager
    │
    ▼
Page
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

---

# Page

## Purpose

A Page represents one fixed-size block of storage.

Instead of writing individual records directly to disk, records are grouped into pages.

Each page has a maximum size of

```
4096 bytes
```

This matches the page size commonly used by operating systems and many database engines.

---

## Responsibilities

A Page is responsible for

- storing records
- tracking used space
- determining whether another record fits
- serializing itself
- deserializing itself
- updating records while maintaining page size

A Page does **not**

- search records
- assign IDs
- manage indexes
- read or write files

---

## Page Layout

Every page begins with a small header.

```
+--------------------------------+
| Page ID                        |
+--------------------------------+
| Used Size                      |
+--------------------------------+
| Record Count                   |
+--------------------------------+
| Record                         |
+--------------------------------+
| Record                         |
+--------------------------------+
| Record                         |
+--------------------------------+
| Padding                        |
+--------------------------------+
```

The remaining unused bytes are padded with zeros so that every page always occupies exactly

```
4096 bytes
```

---

## Why Fixed-Size Pages?

Using pages has several advantages.

- Reduces disk I/O
- Keeps storage organized
- Simplifies free-space tracking
- Makes future caching possible
- Mirrors real database engines

Without pages, every insert would require shifting data throughout the file.

Pages isolate changes to a single block.

---

## Space Management

Each page keeps track of

```
usedSize
```

instead of recalculating its size every time.

Before inserting a record,

```
canFit()
```

checks whether enough free space remains.

```
usedSize
+
newRecordSize

<=

4096
```

If the record fits, it is inserted.

Otherwise, the PageManager creates another page.

---

## Record Updates

Updating a record is slightly more complicated than inserting.

The updated record may be larger than the original.

Before replacing the record,

the page calculates

```
newUsedSize

=

currentUsedSize

-

oldRecordSize

+

newRecordSize
```

If the new size exceeds

```
4096 bytes
```

the update is rejected.

This prevents page overflow.

---

# PageManager

## Purpose

The PageManager owns every page currently loaded into memory.

It acts as the bridge between the Database and individual pages.

The Database never inserts records directly into a Page.

Instead,

```
Database

↓

PageManager

↓

Page
```

---

## Responsibilities

The PageManager is responsible for

- creating new pages
- finding a page with available space
- returning pages
- compacting pages
- returning all records

It does **not**

- write files
- maintain indexes
- parse commands

---

## Inserting Records

When a record is inserted,

the PageManager searches each page.

```
Page 0

↓

fits?

↓

No

↓

Page 1

↓

fits?

↓

Yes

↓

Insert
```

If no page has enough free space,

a new page is automatically created.

---

## Compacting Pages

VACUUM uses the PageManager to rebuild storage.

The process is

```
Collect active records

↓

Delete every page

↓

Create new pages

↓

Insert active records again
```

Deleted records disappear completely.

Pages become tightly packed again.

---

# Serializer

## Purpose

The Serializer converts Record objects into binary data.

It also performs the reverse operation.

```
Record

↓

Binary Bytes

↓

Record
```

This keeps storage independent of the Record class.

---

## Why Separate Serialization?

Without a Serializer,

Page would need to understand

- strings
- integers
- binary layout
- disk format

Separating serialization gives every class one responsibility.

---

## Binary Record Layout

Each record is stored as

```
ID

4 bytes

↓

Age

4 bytes

↓

Deleted Flag

1 byte

↓

Name Length

4 bytes

↓

Name Characters

Variable Length
```

Strings are stored using

```
length

+

characters
```

instead of a fixed-size buffer.

This allows names of different lengths.

---

## Record Size

The Serializer also calculates

```
getRecordSize()
```

This is used by Pages to determine

- free space
- page overflow
- update validity

---

# StorageEngine

## Purpose

The StorageEngine is responsible for all disk input and output.

It is the only class that opens

```
salaha.db
```

No other class performs file operations.

---

## Responsibilities

The StorageEngine

- loads pages
- saves pages
- validates database files
- writes headers
- writes pages

It does **not**

- understand SQL
- search records
- manage indexes

---

## Database Header

Every database begins with

```
+----------------------+
| Magic Number         |
+----------------------+
| Version              |
+----------------------+
| Page Size            |
+----------------------+
```

The header allows the database to detect

- corrupted files
- incompatible versions
- invalid page sizes

before loading any pages.

---

## File Layout

A complete database file looks like

```
Database Header

↓

Page 0

↓

Page 1

↓

Page 2

↓

...
```

Each page always occupies

```
4096 bytes
```

making page locations easy to calculate.

---

## Error Detection

During loading,

the StorageEngine verifies

- file exists
- header exists
- magic number
- database version
- page size
- file size
- page integrity

If any validation fails,

the database exits immediately rather than attempting to load corrupted data.

This follows the fail-fast principle.

---

# Summary

The storage subsystem is responsible for converting in-memory records into a persistent binary database.

Each class performs one specific task.

| Class | Responsibility |
|--------|----------------|
| Page | Stores records inside one 4096-byte page |
| PageManager | Owns and manages every page |
| Serializer | Converts records to and from binary |
| StorageEngine | Reads and writes pages to disk |

Keeping these responsibilities separate makes the storage layer modular, easier to debug, and easier to extend in future versions.

# 5. Indexing System

Searching every record one by one becomes increasingly expensive as the database grows.

Without indexes, operations such as

- FIND
- UPDATE
- DELETE
- SELECT WHERE

would require scanning every record in every page.

To avoid this, salahaDB maintains indexes entirely in memory.

The project uses two different indexing strategies because different types of queries have different requirements.

```
                Database
                    │
        ┌───────────┴────────────┐
        ▼                        ▼
 Primary Index             Secondary Indexes
    B+ Tree               unordered_map
        │                        │
        ▼                        ▼
 RecordLocation          vector<Record IDs>
```

---

# Primary Index (B+ Tree)

## Purpose

The primary index maps

```
Record ID

↓

RecordLocation
```

Every record has exactly one unique ID.

Because IDs are unique, a B+ Tree is an ideal data structure for storing them.

---

## Why a B+ Tree?

Several data structures were considered.

| Structure | Lookup | Ordered | Range Search |
|------------|--------|----------|--------------|
| Vector | O(n) | Yes | O(n) |
| Hash Table | O(1) average | No | Poor |
| Binary Search Tree | O(log n) average | Yes | Good |
| **B+ Tree** | **O(log n)** | **Yes** | **Excellent** |

The B+ Tree was chosen because it supports both

- fast single-record lookups
- efficient range queries

This allows commands such as

```
FIND 100
```

and

```
SELECT * FROM records
WHERE id BETWEEN 50 AND 100
```

to use the same index efficiently.

---

## What is Stored?

The B+ Tree does **not** store records.

Instead, each leaf stores

```
ID

↓

RecordLocation
```

Example

```
100

↓

Page 3

↓

Slot 8
```

The actual record remains inside the Page.

---

## RecordLocation

RecordLocation contains

```
pageIndex

slotIndex
```

Using a location instead of a pointer keeps the index independent of memory addresses.

Whenever a record is needed,

the Database retrieves it using

```
PageManager

↓

Page

↓

Record
```

---

# B+ Tree Structure

The tree consists of two node types.

## Internal Nodes

Internal nodes contain

- keys
- child pointers

They never store records.

Their only purpose is directing searches.

Example

```
          [40 | 90]

         /     |      \

      ...    ...     ...
```

---

## Leaf Nodes

Leaf nodes contain

- keys
- RecordLocations

Leaf nodes are linked together using

```
nextLeaf
```

Example

```
Leaf

1

↓

2

↓

3

↓

4
```

This linked-list structure makes range searches extremely efficient.

---

# Insert

Insertion follows these steps.

```
Find Leaf

↓

Insert Key

↓

Overflow?

↓

No

Done
```

If the node exceeds the maximum number of keys,

```
Split Node

↓

Promote Separator

↓

Update Parent
```

The process continues until the tree becomes balanced.

---

# Delete

Deletion follows the opposite process.

```
Remove Key

↓

Node Too Small?

↓

No

Done
```

If the node falls below the minimum size,

the tree attempts

1. Borrow from left sibling
2. Borrow from right sibling
3. Merge nodes

This guarantees that the tree always satisfies the B+ Tree properties.

---

# Range Search

Range searches take advantage of linked leaf nodes.

Example

```
SELECT
WHERE id BETWEEN 100 AND 200
```

The algorithm performs

```
Find first leaf

↓

Walk linked leaves

↓

Stop at 200
```

No unnecessary nodes are visited.

---

# Secondary Indexes

The project also maintains two secondary indexes.

```
Age

↓

vector<ID>
```

and

```
Name

↓

vector<ID>
```

Both indexes use

```
std::unordered_map
```

---

## Why Hash Tables?

Unlike IDs,

names and ages are not unique.

Multiple records can share

- the same name
- the same age

A hash table allows each value to point to multiple IDs.

Example

```
Age

22

↓

1

↓

14

↓

31

↓

57
```

The Database then performs

```
ID

↓

Primary Index

↓

RecordLocation

↓

Record
```

---

## Why Store IDs Instead of Records?

The secondary indexes intentionally store IDs instead of complete records.

```
Age

↓

vector<ID>
```

instead of

```
Age

↓

vector<Record>
```

This avoids storing duplicate copies of records.

The actual record exists in only one place.

```
Pages
```

Every index simply references it.

---

# Index Maintenance

Indexes are updated whenever records change.

## INSERT

```
Insert Record

↓

Primary Index

↓

Secondary Indexes
```

---

## UPDATE

Updating requires

```
Remove old values

↓

Modify record

↓

Insert new values
```

Both name and age indexes are updated if necessary.

---

## DELETE

Deleting a record removes

- the primary index entry
- the secondary index entries

The record itself remains on disk until VACUUM.

This is known as **soft deletion**.

---

## VACUUM

VACUUM rebuilds every index from scratch.

```
Compact Pages

↓

Clear Indexes

↓

Scan Every Record

↓

Rebuild Primary Index

↓

Rebuild Secondary Indexes
```

Rebuilding guarantees every index remains consistent with the compacted pages.

---

# Time Complexity

| Operation | Complexity |
|------------|-----------|
| FIND | O(log n) |
| INSERT | O(log n) |
| DELETE | O(log n) |
| UPDATE | O(log n) |
| SELECT WHERE ID BETWEEN | O(log n + k) |
| SELECT WHERE age | O(1) average + matches |
| SELECT WHERE name | O(1) average + matches |

where **k** is the number of matching records.

---

# Summary

The indexing subsystem combines two complementary approaches.

The B+ Tree provides efficient ordered access using unique IDs.

The hash tables provide extremely fast lookups for non-unique fields such as names and ages.

Together, these indexes eliminate the need for full table scans in the most common queries while keeping only one physical copy of each record.

# 6. Database Operations

This chapter describes what happens internally when each supported command is executed.

Although each command performs a different task, they all follow the same high-level execution pipeline.

```
User Input

↓

Parser

↓

Command

↓

Database

↓

Indexes / Pages

↓

Storage Engine

↓

Disk
```

The Parser validates syntax.

The Database performs the requested operation.

If data changes, the StorageEngine immediately saves the updated pages to disk.

---

# INSERT

Example

```sql
INSERT INTO records VALUES Alice 22
```

Execution flow

```
User Input

↓

Parser

↓

Database::insert()

↓

Generate new ID

↓

Create Record

↓

PageManager::insertRecord()

↓

Update Indexes

↓

StorageEngine::save()

↓

Success
```

### Step 1

The parser validates the SQL syntax.

If the syntax is incorrect, execution stops immediately.

---

### Step 2

The Database assigns the next available ID.

```
nextID = largestExistingID + 1
```

The user never specifies an ID.

---

### Step 3

A Record object is created.

```
Record

ID

Name

Age

Deleted = false
```

---

### Step 4

The PageManager searches for a page with enough free space.

```
Page 0

↓

Fits?

↓

No

↓

Page 1

↓

Fits?

↓

Yes

↓

Insert
```

If no page has enough room,

a new page is automatically created.

---

### Step 5

After insertion,

all indexes are updated.

```
Primary Index

↓

ID

↓

RecordLocation
```

```
Secondary Indexes

↓

Age

↓

ID
```

```
Name

↓

ID
```

---

### Step 6

The StorageEngine writes every page back to disk.

The insert is now persistent.

---

# FIND

Example

```sql
FIND 42
```

Execution flow

```
Parser

↓

Primary Index

↓

RecordLocation

↓

Page

↓

Record

↓

Print
```

The database does **not** scan every page.

Instead,

the B+ Tree immediately returns the record's location.

---

# SELECT

Several SELECT variants are supported.

---

## SELECT *

```
SELECT * FROM records
```

Execution flow

```
PageManager

↓

All Pages

↓

All Records

↓

Skip Deleted Records

↓

Print
```

Every active record is printed.

---

## SELECT WHERE name

```
SELECT * FROM records
WHERE name = Alice
```

Execution flow

```
Hash Table

↓

Matching IDs

↓

Primary Index

↓

RecordLocation

↓

Record

↓

Print
```

No table scan is required.

---

## SELECT WHERE age

The process is identical.

```
Age

↓

Hash Table

↓

IDs

↓

Primary Index

↓

Records
```

---

## SELECT WHERE id BETWEEN

Execution flow

```
Primary Index

↓

Range Search

↓

RecordLocations

↓

Records

↓

Print
```

The linked leaf nodes of the B+ Tree allow the range to be scanned efficiently.

---

## SELECT ORDER BY

Execution flow

```
All Records

↓

Temporary Vector

↓

std::sort()

↓

Print
```

Sorting is performed on a temporary copy of the records.

The records stored inside the database remain unchanged.

---

# UPDATE

Example

```sql
UPDATE records
SET name = Bob age = 30
WHERE id = 5
```

Execution flow

```
Primary Index

↓

Locate Record

↓

Update Page

↓

Update Secondary Indexes

↓

Save Database
```

---

### Step 1

The B+ Tree locates the record.

---

### Step 2

The Page checks whether the updated record still fits.

If the updated record would exceed the page size,

the update is rejected.

---

### Step 3

Old secondary index entries are removed.

```
Old Name

↓

Remove ID
```

```
Old Age

↓

Remove ID
```

---

### Step 4

New entries are inserted.

```
New Name

↓

ID
```

```
New Age

↓

ID
```

---

### Step 5

The updated pages are written back to disk.

---

# DELETE

Example

```sql
DELETE FROM records
WHERE id = 8
```

Execution flow

```
Primary Index

↓

Locate Record

↓

Remove Index Entries

↓

Mark Deleted

↓

Save
```

Records are **not** physically removed.

Instead,

```
deleted = true
```

This is known as **soft deletion**.

The physical space remains allocated until VACUUM.

---

# VACUUM

VACUUM permanently removes deleted records.

Execution flow

```
Read Active Records

↓

Delete Existing Pages

↓

Create Fresh Pages

↓

Reinsert Records

↓

Rebuild Indexes

↓

Save Database
```

After VACUUM,

- deleted records disappear permanently
- pages become compact
- indexes are rebuilt

---

# DROP DATABASE

Execution flow

```
Clear Pages

↓

Clear Indexes

↓

Reset nextID

↓

Save Empty Database
```

After DROP,

the database still exists,

but contains zero records.

---

# STATS

The STATS command does not modify the database.

It calculates

- total records
- active records
- deleted records
- page count
- page size
- total database size

using the pages currently loaded in memory.

---

# HELP

HELP simply prints the list of supported commands.

No pages or indexes are accessed.

---

# EXIT

EXIT immediately terminates the program.

Because salahaDB saves after every modifying operation,

there is nothing additional to write during exit.

The command only ends the application.

---

# Persistence Strategy

Every modifying command

- INSERT
- UPDATE
- DELETE
- VACUUM
- DROP

calls

```
Database::save()
```

immediately after completing successfully.

This approach prioritizes reliability over write performance.

If the application crashes,

all previously completed operations have already been written to disk.

---

# Summary

Every command follows the same general execution model.

```
Parser

↓

Database

↓

Pages

↓

Indexes

↓

Storage Engine

↓

Disk
```

Each subsystem performs only one responsibility, making the execution pipeline predictable, modular, and easy to extend.

# 7. Design Decisions and Trade-offs

Every major component of salahaDB was designed by considering multiple possible implementations.

In many cases there was no single "correct" solution. Instead, the chosen design balanced simplicity, correctness, maintainability, and performance.

This section explains the reasoning behind those decisions.

---

# Why Binary Storage Instead of Text?

The earliest versions of the project stored records as text.

Example

```
1 Alice 22 false
```

Although text files are easy to debug, they have several disadvantages.

- Larger file sizes
- Slower reading and writing
- Difficult to support pages
- Difficult to detect corruption
- Not representative of real databases

Binary storage was chosen because it

- reduces storage size
- allows fixed-size pages
- provides predictable layouts
- more closely resembles production database engines

---

# Why Fixed-Size Pages?

Instead of storing one record after another, salahaDB stores records inside fixed-size pages.

```
Database

↓

Page 0

↓

Page 1

↓

Page 2
```

Using pages provides several benefits.

- Simplifies disk I/O
- Makes free-space tracking easier
- Supports future caching
- Matches the architecture of modern database systems

The page size was chosen to be

```
4096 bytes
```

because this is a common memory page size used by operating systems.

---

# Why Separate StorageEngine and Serializer?

Initially, serialization logic could have been placed directly inside the StorageEngine.

Instead, serialization was moved into its own class.

```
Record

↓

Serializer

↓

StorageEngine
```

This separation has several advantages.

- StorageEngine only performs file operations.
- Serializer only converts objects into bytes.
- Record remains a simple data structure.

Each class has one responsibility.

---

# Why Use a B+ Tree?

Several indexing strategies were considered.

| Structure | Lookup | Ordered | Range Queries |
|-----------|---------|----------|---------------|
| Vector | O(n) | Yes | Slow |
| Hash Table | O(1) average | No | Poor |
| Binary Search Tree | O(log n) average | Yes | Good |
| B+ Tree | O(log n) | Yes | Excellent |

The B+ Tree was selected because it supports both

- fast primary-key lookups
- efficient range searches

This allows commands such as

```
FIND 100
```

and

```
SELECT * FROM records
WHERE id BETWEEN 50 AND 100
```

to use the same index efficiently.

---

# Why Use Hash Tables for Name and Age?

Unlike IDs,

names and ages are not unique.

A B+ Tree would work, but it would introduce unnecessary complexity.

Instead,

```
unordered_map

↓

vector<ID>
```

provides very fast average-case lookups while allowing multiple records to share the same key.

This makes

```
SELECT WHERE age

SELECT WHERE name
```

very efficient.

---

# Why Store RecordLocation Instead of Record?

The primary index stores

```
RecordLocation

↓

Page Index

Slot Index
```

instead of storing entire records.

This avoids duplicating data.

There is always exactly one copy of each record stored inside the pages.

The indexes simply point to it.

---

# Why Use Soft Deletes?

Deleting a record does not immediately remove it from disk.

Instead,

```
deleted = true
```

is set.

This approach has several advantages.

- No page shifting
- Simpler implementation
- Faster deletes
- Easier debugging

The downside is wasted space.

To recover unused space,

the database provides

```
VACUUM
```

which permanently removes deleted records.

---

# Why Rebuild Indexes During VACUUM?

After compacting pages,

record locations change.

Because every RecordLocation stores

```
Page

↓

Slot
```

all indexes must be updated.

Rather than updating every location individually,

salahaDB clears every index and rebuilds them by scanning the compacted pages.

Although rebuilding takes longer,

the implementation is simpler and guarantees consistency.

---

# Why Save After Every Modification?

Whenever one of the following commands succeeds

- INSERT
- UPDATE
- DELETE
- VACUUM
- DROP

the database immediately writes all pages to disk.

Advantages

- Excellent reliability
- Very little risk of data loss
- Simple implementation

Disadvantages

- More disk writes
- Slower bulk inserts

This trade-off was intentionally chosen because correctness was prioritized over performance.

Future versions could improve performance using

- write buffering
- transactions
- write-ahead logging (WAL)

---

# Why Validate Commands in the Parser?

The Parser performs syntax validation before the Database receives a command.

Instead of passing raw strings,

it returns a validated Command object.

Advantages

- Database code becomes simpler.
- Invalid syntax is rejected immediately.
- Parsing logic is isolated in one place.
- Easier to add new commands later.

This follows the fail-fast principle.

---

# Why Separate Responsibilities?

Every major class performs exactly one task.

| Class | Responsibility |
|--------|----------------|
| Parser | Validate commands |
| Database | Execute operations |
| PageManager | Manage pages |
| Page | Store records |
| Serializer | Convert binary data |
| StorageEngine | Read and write files |
| B+ Tree | Primary indexing |

Keeping responsibilities separate reduces coupling and makes each component easier to understand, debug, and test.

---

# Current Limitations

Although salahaDB implements many core database concepts, several features were intentionally left out to keep the project manageable.

Current limitations include

- Single table only
- No SQL joins
- No transactions
- No concurrency control
- No write-ahead logging
- Entire database loaded into memory
- Fixed page size
- Single-threaded execution

These limitations also provide opportunities for future improvements.

---

# Future Improvements

Possible future extensions include

- Multiple tables
- SQL joins
- Transactions
- Write-ahead logging (WAL)
- LRU page cache
- Disk-based B+ Tree
- Secondary indexes stored on disk
- Variable page sizes
- Query optimizer
- Concurrency using locks or MVCC

These additions would move the project closer to the architecture of a production database system while preserving the modular design established in the current implementation.

# 8. Complexity Analysis

Understanding the time complexity of each operation is important when evaluating the scalability of a database system.

The following estimates describe the average-case performance of the current salahaDB implementation.

---

# Command Complexity

| Command | Time Complexity | Notes |
|----------|-----------------|-------|
| INSERT | O(log n) | B+ Tree insert plus page insertion and index updates |
| FIND | O(log n) | Uses the primary B+ Tree index |
| UPDATE | O(log n) | Find record, update page, update secondary indexes |
| DELETE | O(log n) | Remove from indexes and mark record as deleted |
| SELECT * | O(n) | Visits every active record |
| SELECT WHERE id BETWEEN | O(log n + k) | B+ Tree range search where *k* is the number of matching records |
| SELECT WHERE name | O(1) average + k | Hash lookup followed by record retrieval |
| SELECT WHERE age | O(1) average + k | Hash lookup followed by record retrieval |
| SELECT ORDER BY | O(n log n) | Copies records and sorts using `std::sort()` |
| VACUUM | O(n log n) | Rebuilds pages and recreates all indexes |
| DROP DATABASE | O(1) | Clears all in-memory structures and saves an empty database |
| HELP | O(1) | Prints static text |
| STATS | O(n) | Counts records by scanning pages |

---

# Data Structure Complexity

## B+ Tree

| Operation | Complexity |
|-----------|------------|
| Search | O(log n) |
| Insert | O(log n) |
| Delete | O(log n) |
| Range Search | O(log n + k) |

The B+ Tree remains balanced after every insertion and deletion, ensuring logarithmic performance even as the database grows.

---

## Secondary Hash Indexes

Both secondary indexes use `std::unordered_map`.

| Operation | Average Case |
|-----------|--------------|
| Insert | O(1) |
| Lookup | O(1) |
| Delete | O(1) |

These indexes map names and ages to vectors of record IDs.

Because names and ages are not unique, multiple IDs may be associated with the same key.

---

## Page Operations

| Operation | Complexity |
|-----------|------------|
| Check available space | O(1) |
| Insert record | O(1) |
| Update record | O(1) |
| Serialize page | O(m) |
| Deserialize page | O(m) |

Where **m** is the number of records stored in the page.

Since page sizes are fixed at 4096 bytes, **m** is bounded by the maximum page capacity.

---

## PageManager

| Operation | Complexity |
|-----------|------------|
| Insert record | O(p) |
| Get page | O(1) |
| Load pages | O(p) |
| Compact pages | O(n) |

Where:

- **p** = number of pages
- **n** = total number of records

Currently, the PageManager performs a linear search to find a page with available space.

A future optimization could maintain a free-space map to reduce insertion time.

---

# Storage Complexity

Each page occupies exactly

```
4096 bytes
```

The total database size is

```
Database Header

+

(number of pages × 4096 bytes)
```

Deleted records continue occupying space until a VACUUM operation is performed.

---

# Memory Usage

At startup, the entire database is loaded into memory.

Memory usage consists of

- all Pages
- all Records
- the B+ Tree
- secondary hash indexes

This simplifies implementation and improves lookup performance, but limits scalability for very large databases.

Future versions could load pages on demand instead of loading the entire database.

---

# Performance Summary

The most common operations (`FIND`, `INSERT`, `UPDATE`, and `DELETE`) all execute in logarithmic time due to the use of the B+ Tree.

Queries by name and age benefit from constant average-time hash table lookups.

The only operations that require scanning every record are:

- `SELECT *`
- `SELECT ORDER BY`
- `VACUUM`
- `STATS`

For the current scope of the project, this provides a good balance between simplicity, readability, and performance.

# 9. Lessons Learned

Building salahaDB provided practical experience implementing many of the concepts typically hidden behind commercial database systems. While the project began as a simple persistent storage engine, it gradually evolved into a modular database architecture with page-based storage, indexing, binary serialization, and SQL-like command parsing.

The project reinforced several important software engineering principles.

---

# Separation of Responsibilities

One of the biggest lessons learned was the importance of separating responsibilities between classes.

Rather than creating one large class responsible for every operation, each component was given a single responsibility.

For example:

- The Parser only validates user input.
- The Database coordinates operations.
- The Page stores records.
- The Serializer converts records to and from binary.
- The StorageEngine performs file I/O.
- The B+ Tree manages the primary index.

Keeping responsibilities separate made the codebase significantly easier to debug, extend, and reason about.

---

# Why Database Systems Use Pages

Before this project, storing records directly in a file seemed like the simplest solution.

Implementing fixed-size pages demonstrated why nearly every modern database organizes data this way.

Pages provide predictable storage, simplify disk access, improve space management, and create a foundation for future features such as page caching and buffer pools.

This project showed that pages are one of the fundamental building blocks of a database engine.

---

# The Importance of Indexing

Initially, every lookup required scanning every record.

As the number of records increased, this approach quickly became inefficient.

Implementing a B+ Tree for primary key lookups and hash-based secondary indexes demonstrated how indexing dramatically improves query performance.

The project also highlighted that different query types benefit from different indexing strategies.

There is no single index that is optimal for every workload.

---

# Designing for Extensibility

Several features required very little modification to existing code because the architecture was designed to be modular.

For example:

- Adding new commands only required extending the Parser and Database.
- Adding secondary indexes did not require modifying the StorageEngine.
- Binary serialization was introduced without changing the Record structure.

This reinforced the value of designing components with clear interfaces and minimal coupling.

---

# Working with Binary Data

Implementing binary serialization provided experience working directly with memory layouts and file formats.

Topics explored during development included:

- fixed-size headers
- binary file structures
- serialization and deserialization
- variable-length strings
- page padding
- file validation
- corruption detection

These concepts are common in systems programming but are often abstracted away by higher-level libraries.

---

# Building Complex Data Structures

The B+ Tree was the most technically challenging component of the project.

Insertion required node splitting and separator promotion.

Deletion required borrowing, merging, and tree rebalancing while preserving the properties of a balanced tree.

Implementing these algorithms provided a much deeper understanding of balanced search trees than simply using a standard library container.

---

# Error Handling

Throughout development, the project gradually adopted a fail-fast philosophy.

Examples include:

- rejecting malformed commands during parsing
- validating numeric input
- detecting corrupted database files
- checking page sizes during loading
- rejecting oversized records
- preventing page overflow during updates

Detecting errors as early as possible simplified debugging and helped keep the database in a consistent state.

---

# Future Improvements

Although the current implementation supports the core functionality of a simple database engine, there are several areas that could be expanded in future versions.

Potential improvements include:

- Multiple tables
- SQL joins
- Transactions
- Write-ahead logging (WAL)
- Buffer pool and page caching
- Concurrency control
- Disk-resident indexes
- Query optimization
- Free-space management for pages
- Background checkpointing

Many of these features build naturally on the modular architecture already established by the project.

---

# Final Thoughts

Building salahaDB transformed many database concepts from theory into practical implementation.

The project provided experience designing a storage engine, implementing binary persistence, managing fixed-size pages, maintaining multiple indexes, and organizing a moderately sized C++ codebase using object-oriented design principles.

More importantly, it demonstrated that complex systems can be built incrementally by separating responsibilities, validating assumptions early, and designing each component to perform one well-defined task.

# Appendix A - Internal Algorithms

This appendix summarizes the core algorithms used throughout salahaDB.

Rather than showing implementation details, it focuses on the high-level steps performed by each operation.

---

# Database Startup

When salahaDB starts, it reconstructs its in-memory state from the database file.

Algorithm

```
Load Database Header

↓

Validate Header

↓

Read Every Page

↓

Deserialize Every Record

↓

Insert Active Records Into Indexes

↓

Determine Largest Record ID

↓

nextID = largestID + 1
```

At this point, the database is ready to accept commands.

---

# INSERT Algorithm

```
Validate Input

↓

Create Record

↓

Assign nextID

↓

Find Page With Space

↓

Insert Record

↓

Update Primary Index

↓

Update Secondary Indexes

↓

Save Database
```

---

# FIND Algorithm

```
Search B+ Tree

↓

RecordLocation

↓

Locate Page

↓

Locate Slot

↓

Return Record
```

No linear scan is required.

---

# SELECT WHERE name

```
Hash Table

↓

Matching IDs

↓

B+ Tree

↓

RecordLocation

↓

Record

↓

Print
```

---

# SELECT WHERE age

The algorithm is identical.

```
Age Hash Table

↓

Matching IDs

↓

Primary Index

↓

Record

↓

Print
```

---

# SELECT WHERE id BETWEEN

```
Locate First Leaf

↓

Traverse Linked Leaves

↓

Stop At Maximum ID

↓

Return Records
```

This is why the B+ Tree is well suited for range queries.

---

# UPDATE Algorithm

```
Locate Record

↓

Create Updated Record

↓

Verify Record Fits Page

↓

Replace Record

↓

Remove Old Index Entries

↓

Insert New Index Entries

↓

Save Database
```

---

# DELETE Algorithm

```
Locate Record

↓

Remove Primary Index Entry

↓

Remove Secondary Index Entries

↓

Mark Record Deleted

↓

Save Database
```

Records remain inside the page until VACUUM.

---

# VACUUM Algorithm

```
Collect Active Records

↓

Delete Every Page

↓

Create New Pages

↓

Reinsert Active Records

↓

Rebuild Every Index

↓

Save Database
```

VACUUM permanently removes deleted records and reclaims storage.

---

# DROP DATABASE Algorithm

```
Delete All Pages

↓

Clear Primary Index

↓

Clear Secondary Indexes

↓

Reset nextID

↓

Write Empty Database
```

---

# Page Serialization

```
Write Page Header

↓

Write Record 1

↓

Write Record 2

↓

...

↓

Write Padding

↓

Exactly 4096 Bytes
```

Every page always occupies the same amount of disk space.

---

# Record Serialization

Each record is written in the following order.

```
ID

↓

Age

↓

Deleted Flag

↓

Name Length

↓

Name Characters
```

Variable-length names are supported by storing the string length before the characters.

---

# Index Rebuilding

Whenever the database is loaded or compacted, every index is rebuilt.

```
Clear Indexes

↓

Scan Every Page

↓

Scan Every Record

↓

Insert Into B+ Tree

↓

Insert Into Hash Tables
```

This guarantees that every index accurately reflects the current state of the database.

---

# Overall Execution Pipeline

Every command ultimately follows the same execution model.

```
User

↓

main.cpp

↓

Parser

↓

Command

↓

Database

↓

PageManager

↓

Page

↓

Serializer

↓

StorageEngine

↓

salaha.db
```

This layered architecture keeps responsibilities separate and makes the system easier to maintain and extend.