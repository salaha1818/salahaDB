# salahaDB

A lightweight database engine written in **C++20** that implements persistent page-based storage, binary serialization, B+ Tree indexing, secondary hash indexes, and a SQL-inspired command interface.

The project was developed to explore how database management systems work internally by implementing core database components from scratch rather than relying on existing database libraries.

---

# Features

* Persistent Binary Storage
* Fixed-Size Page Architecture (4096-byte pages)
* B+ Tree Primary Index for ID Lookups
* Efficient ID Range Queries
* Hash-Based Secondary Indexes for Names and Ages
* SQL-Inspired Command Interface
* Interactive and Script Execution Modes
* Binary Serialization and Deserialization
* Variable-Length Record Names
* Soft Record Deletion
* Database Compaction with `VACUUM`
* Automatic Database Loading and Saving
* Database Statistics
* Database File Validation
* Corruption and Version Detection
* Runtime Error Detection and Handling

---

# Architecture

salahaDB is divided into separate components, with each component responsible for part of the database engine.

| Component        | Responsibility                                      |
| ---------------- | --------------------------------------------------- |
| `Record`         | Represents a database record                        |
| `Command`        | Represents a parsed user command                    |
| `Parser`         | Validates and parses SQL-inspired commands          |
| `Database`       | Coordinates database operations                     |
| `Page`           | Stores records inside a fixed-size page             |
| `PageManager`    | Creates and manages database pages                  |
| `Serializer`     | Converts records to and from binary data            |
| `StorageEngine`  | Reads and writes the database file                  |
| `BPlusTree`      | Primary index used for ID lookups and range queries |
| `RecordLocation` | Stores the page and slot location of a record       |

Detailed architecture documentation is available in:

```text
docs/architecture.md
```

---

## Record Structure

Each record contains the following fields:

| Field     | Type          | Description                                  |
| --------- | ------------- | -------------------------------------------- |
| `id`      | `uint32_t`    | Automatically assigned record identifier     |
| `name`    | `std::string` | Variable-length record name                  |
| `age`     | `uint32_t`    | Age associated with the record               |
| `deleted` | `bool`        | Indicates whether the record is soft-deleted |

---

## Storage Layout

The database file is organized into:

* Database Header
* Fixed-Size Pages
* Binary Record Data
* Variable-Length Name Data
* Soft-Deletion Metadata

Each page is **4096 bytes** and can contain multiple serialized records.

The database automatically rebuilds its indexes when data is loaded from disk.

---

# Indexing

## Primary Index

salahaDB uses a **B+ Tree** as its primary index.

The primary index:

* Maps record IDs to their page and slot locations
* Supports efficient ID lookups
* Supports ID range queries
* Avoids scanning every record for primary-key searches

---

## Secondary Indexes

salahaDB also maintains hash-based secondary indexes for:

* Record names
* Record ages

Secondary indexes store record IDs. These IDs are then resolved through the B+ Tree primary index to locate the corresponding records.

---

# Supported Commands

## Insert Records

```sql
INSERT INTO records VALUES <name> <age>
```

Example:

```sql
INSERT INTO records VALUES Alice 22
```

---

## Select Records

Display all active records:

```sql
SELECT * FROM records
```

Search by name:

```sql
SELECT * FROM records WHERE name = <name>
```

Search by age:

```sql
SELECT * FROM records WHERE age = <age>
```

Search within an ID range:

```sql
SELECT * FROM records WHERE id BETWEEN <min> AND <max>
```

Sort records by name:

```sql
SELECT * FROM records ORDER BY name
```

Sort records by age:

```sql
SELECT * FROM records ORDER BY age
```

---

## Update Records

```sql
UPDATE records SET name = <name> age = <age> WHERE id = <id>
```

Example:

```sql
UPDATE records SET name = Alicia age = 23 WHERE id = 1
```

---

## Delete Records

```sql
DELETE FROM records WHERE id = <id>
```

Records are initially soft-deleted and remain in the database file until `VACUUM` is executed.

---

## Find Records

```text
FIND <id>
```

Example:

```text
FIND 1
```

The `FIND` command uses the B+ Tree primary index to locate a record by its ID.

---

## Database Commands

| Command                 | Description                                                        |
| ----------------------- | ------------------------------------------------------------------ |
| `VACUUM`                | Permanently removes soft-deleted records and compacts the database |
| `STATS`                 | Displays database and storage statistics                           |
| `HELP`                  | Displays the supported commands                                    |
| `DROP DATABASE CONFIRM` | Deletes the current database                                       |
| `EXIT`                  | Saves the database and exits the program                           |

---

# Execution Modes

When the program starts, the user can choose between two execution modes.

## Interactive Mode

Interactive mode allows commands to be entered one at a time.

```text
INSERT INTO records VALUES Alice 22
SELECT * FROM records
FIND 1
EXIT
```

---

## Script Mode

Script mode reads and executes commands from `code.txt`.

```text
INSERT INTO records VALUES Alice 22
INSERT INTO records VALUES Bob 31
SELECT * FROM records
SELECT * FROM records ORDER BY age
STATS
EXIT
```

---

# Building

Clone the repository:

```bash
git clone <repository-url>
cd salahaDB
```

Generate the build files using CMake:

```bash
cmake -B build
```

Compile the project:

```bash
cmake --build build
```

The project requires a compiler with **C++20** support.

---

# Running

After building the project, run the generated executable from the build directory.

The exact executable location may depend on the compiler and build system being used.

```bash
./build/salahaDB
```

On Windows, the executable may be located in a configuration directory such as:

```powershell
.\build\Debug\salahaDB.exe
```

---

# Demo Files

Demo files are included in the following directory:

```text
examples/
```

The directory contains:

* `demo.db` — A pre-populated database
* `demo_script.txt` — A script demonstrating the supported commands
* `README.md` — Instructions for using the demo files

Before running the demo, copy the files into the project root.

## Windows PowerShell

```powershell
Copy-Item examples\demo.db salaha.db
Copy-Item examples\demo_script.txt code.txt
```

## Linux or macOS

```bash
cp examples/demo.db salaha.db
cp examples/demo_script.txt code.txt
```

---

# Database Compaction

Deleting a record performs a soft deletion.

Soft-deleted records:

* Are no longer displayed in queries
* Are removed from active indexes
* Continue occupying space inside the database file

Running:

```text
VACUUM
```

rewrites the database, permanently removes deleted records, compacts the remaining data, and rebuilds the indexes.

---

# Database Statistics

The `STATS` command displays information about the current database.

Statistics may include:

* Total record count
* Active record count
* Deleted record count
* Number of pages
* Database file size
* Page utilization
* Index information

---

# Error Handling

The database detects and reports:

* Invalid commands
* Invalid command syntax
* Invalid numeric values
* Negative IDs
* Negative ages
* Missing command arguments
* Invalid database headers
* Unsupported database versions
* Invalid page sizes
* Corrupted database files
* Oversized records
* Page overflow during record updates
* Invalid record locations
* File opening failures
* File reading failures
* File writing failures

