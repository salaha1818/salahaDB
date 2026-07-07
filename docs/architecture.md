salahaDB Architecture Notes

Version 1 Goal:
Build a simple persistent database that can store records, load them from disk, and print them back after restarting the program.

Record:
A record represents one row of data.
For version 1, each record contains:
- id
- name
- age

The id will be assigned automatically by the database.

Design Decision:
The Record object should only store data.
It should not save itself to disk.

A separate StorageEngine class will be responsible for writing records to disk and reading records back.

Storage Plan:
Version 1 will store records in a simple file.
Later versions will organize records into fixed-size pages.

Pages are useful because databases usually read and write data in chunks.
Pages also make indexing, caching, and faster lookups easier later.

Tracking current ID:
Read every record and find the largest ID, slow but good enough for version 1.

Command Flow:
User input should not go directly to the StorageEngine.

The command should first be read by main.cpp.
Then a Parser should separate the command into its parts.
The Database class should decide what action to take.
The StorageEngine class should only handle saving and loading records from disk.

Delete Strategy:
For version 1, deleted records will be marked as deleted instead of rewriting the entire file.

This is simpler and easier to debug, but it wastes space.

In a future version, AtlasDB can track deleted record slots using a free list.
When inserting new records, the database can reuse free slots before appending to the end of the file.

Parser Design:
The parser will convert raw user input into a Command object instead of returning loose strings.

This makes the system easier to extend later because new commands can be added without changing the entire command flow.

A Command will contain:
- command type
- command arguments
- validation result if needed

Example:
insert Alice 21

becomes:

Command
- type: INSERT
- name: Alice
- age: 21

Input Validation:
The Parser is responsible for validating user input.

If a command is invalid or missing required arguments, the Parser will reject it immediately and return an error message.

The Database class should only receive valid commands.

This follows the "fail fast" principle by preventing invalid data from propagating through the system.

Persistence Strategy:
For version 1, AtlasDB will write changes to disk immediately after each insert or delete.

This is safer than only saving on exit because data is less likely to be lost if the program crashes.

Although saving after every command may be slower for many inserts, performance is not the priority in version 1.
The priority is correctness and persistence.

StorageEngine Design:
Long term, the StorageEngine should save and load raw bytes so it is not tightly coupled to one specific record type.

For version 1, AtlasDB may still pass Record objects into the StorageEngine for simplicity.

In a future version, Records will be serialized into bytes before being given to the StorageEngine.
This will make the storage layer more reusable and easier to extend.

Class Usage: 

Record
- Represents one row of data.
- Contains id, name, and age.
- No saving, parsing, or command logic.

Command
- Represents one valid user command.
- Contains command type and arguments.
- Does not execute anything.

Parser
- Takes raw user input.
- Validates it early.
- Converts it into a Command object.
- Rejects invalid commands.

Database
- The brain of AtlasDB.
- Receives valid Commands.
- Decides what action to perform.
- Assigns IDs.
- Calls StorageEngine when data needs saving/loading.

StorageEngine
- Handles reading/writing to disk.
- v1 can work with simple text storage.
- Long term should store raw bytes, not care about record meaning.

Serializer
- Converts Records into text/bytes.
- Converts text/bytes back into Records.
- Keeps Record separate from storage format.