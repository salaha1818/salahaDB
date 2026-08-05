# Examples

This folder contains example files for trying out **salahaDB**.

## Files

### `demo.db`

A pre-populated database containing sample records.

### `demo_script.txt`

A script demonstrating the main database commands, including:

- INSERT
- SELECT
- FIND
- UPDATE
- DELETE
- STATS
- VACUUM

## Usage

The application expects the following files to exist in the project root:

- `salaha.db`
- `code.txt`

Before running salahaDB for the first time, copy the example files:

### Windows (PowerShell)

```powershell
Copy-Item examples\demo.db salaha.db
Copy-Item examples\demo_script.txt code.txt
```

After copying the files:

- Select **Interactive Mode** to use the sample database.
- Select **Script Mode** to execute the demo script.

The copied files (`salaha.db` and `code.txt`) are development files and are ignored by Git. The files in this folder remain unchanged and can be copied again whenever you want to reset your working database or script.