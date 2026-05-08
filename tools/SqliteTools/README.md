# SqliteTools

Database management utility for KotOR Patch Manager address databases.

## Quick Reference

### Convert TOML to SQLite

Creates a new SQLite database from a TOML address file.

```powershell
dotnet run -- toml-to-sqlite "C:\path\to\input.toml" "C:\path\to\output.db"
```

**Example:**
```powershell
dotnet run -- toml-to-sqlite "..\..\AddressDatabases\kotor1_0_3.toml" "..\..\AddressDatabases\kotor1_0_3.db"
```

---

### Import Ghidra Functions

Imports function data from a Ghidra CSV export into an existing database.

```powershell
dotnet run -- import-ghidra --csv "C:\path\to\functions.csv" --database "C:\path\to\database.db"
```

**With replace mode** (deletes all existing functions first):
```powershell
dotnet run -- import-ghidra --csv "C:\path\to\functions.csv" --database "C:\path\to\database.db" --mode replace
```

**CSV Format:**
```csv
class_name,function_name,address,calling_convention,param_size_bytes,notes
CMyClass,MyFunction,0x00401000,__thiscall,8,"Some notes"
```

Required columns: `class_name`, `function_name`, `address`
Optional columns: `calling_convention`, `param_size_bytes`, `notes`

---

### Import Offsets

Imports class member offsets from a CSV file into an existing database.

```powershell
dotnet run -- import-offsets --csv "C:\path\to\offsets.csv" --database "C:\path\to\database.db"
```

**With replace mode** (deletes all existing offsets first):
```powershell
dotnet run -- import-offsets --csv "C:\path\to\offsets.csv" --database "C:\path\to\database.db" --mode replace
```

**Example:**
```powershell
dotnet run -- import-offsets --csv "C:\Users\laned\offsets.csv" --database "..\..\AddressDatabases\kotor1_0_3.db"
```

**CSV Format:**
```csv
class_name,member_name,offset,notes
CMyClass,m_myMember,16,"Optional notes here"
CMyClass,m_otherMember,24,
```

Required columns: `class_name`, `member_name`, `offset`
Optional columns: `notes`

---

### Migrate Database Schema

Runs schema migrations on a database (e.g., adds new columns for Ghidra import support).

```powershell
dotnet run -- migrate --database "C:\path\to\database.db"
```

**Example:**
```powershell
dotnet run -- migrate --database "..\..\AddressDatabases\kotor1_0_3.db"
```

---

### Validate Database

Checks database schema and data integrity.

```powershell
dotnet run -- validate --database "C:\path\to\database.db"
```

**Example:**
```powershell
dotnet run -- validate --database "..\..\AddressDatabases\kotor1_0_3.db"
```

---

## Modes

Both `import-ghidra` and `import-offsets` support a `--mode` argument:

| Mode | Behavior |
|------|----------|
| `append` | **(Default)** Insert new records, update existing ones (upsert) |
| `replace` | Delete all existing records first, then insert |

---

## Verbose Output

Add `--verbose` to any command to see detailed error information:

```powershell
dotnet run -- import-offsets --csv "C:\path\to\offsets.csv" --database "C:\path\to\database.db" --verbose
```
