# salahaDB

A lightweight database engine written in **C++20** that implements persistent page-based storage, binary serialization, B+ Tree indexing, secondary hash indexes, and a SQL-like command interface.

This project was built to better understand how database systems work internally by implementing many of the core components from scratch rather than relying on existing database libraries.

---

## Features

- Persistent binary storage
- Fixed-size page architecture (4096-byte pages)
- B+ Tree primary index for efficient ID lookups
- Hash-based secondary indexes for name and age searches
- SQL-inspired command syntax
- Interactive and script execution modes
- Soft deletes with database compaction (`VACUUM`)
- Automatic database loading and saving
- Database statistics
- Binary serialization/deserialization
- Database file validation and corruption detection

---

## Supported Commands

### Data Manipulation

```sql
INSERT INTO records VALUES <name> <age>

SELECT * FROM records

SELECT * FROM records WHERE name = <name>

SELECT * FROM records WHERE age = <age>

SELECT * FROM records WHERE id BETWEEN <min> AND <max>

SELECT * FROM records ORDER BY name

SELECT * FROM records ORDER BY age

UPDATE records SET name = <name> age = <age> WHERE id = <id>

DELETE FROM records WHERE id = <id>

FIND <id>
```

### Database Commands

```text
VACUUM
STATS
HELP
DROP DATABASE CONFIRM
EXIT
```

---

## Architecture

salahaDB is divided into small components, each responsible for one part of the database engine.

| Component | Responsibility |
|----------|----------------|
| Record | Represents one database record |
| Command | Represents a parsed user command |
| Parser | Validates and parses SQL-like commands |
| Database | Coordinates database operations |
| Page | Stores records inside fixed-size pages |
| PageManager | Manages all database pages |
| Serializer | Converts records to/from binary |
| StorageEngine | Reads and writes the database file |
| BPlusTree | Primary index used for ID lookups |
| RecordLocation | Stores page and slot information |

Detailed documentation is available in:

```
docs/architecture.md
```

---

## Project Structure

```text
salahaDB/
│
├── docs/
│   └── architecture.md
│
├── examples/
│   ├── demo.db
│   ├── demo_script.txt
│   └── README.md
│
├── include/
├── src/
│
├── CMakeLists.txt
├── LICENSE
└── README.md
```

---

## Building

Clone the repository.

```bash
git clone <repository-url>
cd salahaDB
```

Build using CMake.

```bash
cmake -B build
cmake --build build
```

Or compile manually with your preferred C++20 compiler.

---

## Running

When the program starts, choose one of two modes.

### Interactive Mode

Allows commands to be entered one at a time.

Example:

```text
INSERT INTO records VALUES Alice 22

SELECT * FROM records

FIND 1
```

### Script Mode

Reads commands from `code.txt`.

Example:

```text
INSERT INTO records VALUES Alice 22
INSERT INTO records VALUES Bob 31
SELECT * FROM records
EXIT
```

---

## Example Files

The repository includes example files in:

```
examples/
```

Before using them, copy them into the project root.

### Windows (PowerShell)

```powershell
Copy-Item examples\demo.db salaha.db
Copy-Item examples\demo_script.txt code.txt
```

- `demo.db` is a pre-populated database.
- `demo_script.txt` demonstrates the supported commands.

---

## Storage Design

- Database Header
- Fixed-size Pages (4096 bytes)
- Binary Record Serialization
- Variable-length Names
- Soft Deletes
- Page Compaction using `VACUUM`

The database rebuilds all indexes automatically when loading from disk.

---

## Indexing

Primary Index

- B+ Tree
- Used for ID lookups
- Supports efficient range queries

Secondary Indexes

- Hash table by name
- Hash table by age

Secondary indexes store record IDs, which are resolved through the primary index.

---

## Error Handling

The database validates:

- Invalid commands
- Invalid numeric values
- Negative IDs and ages
- Corrupted database headers
- Invalid page sizes
- Unsupported database versions
- Oversized records
- Page overflow during updates
- File read/write failures

---

## Future Improvements

Potential future enhancements include:

- Multiple tables
- Transactions
- Write-ahead logging (WAL)
- Buffer pool
- Query optimizer
- Concurrency control
- SQL parser improvements
- Additional indexing strategies
