# SQLite Address Database Migration Plan

## Recent Updates (2025-12-24)

### SQLite Address Database Migration - COMPLETED ✅

The project has migrated from TOML-based address databases to SQLite. This does **not** affect the patch options design, but is important context for understanding the current state of the codebase.

**What Changed:**
- Address databases (`AddressDatabases/`) now use SQLite (`.db` files) instead of TOML (`.toml` files)
- `GameVersion.cpp` refactored to use SQLite prepared statements instead of hash maps
- `PatchApplicator.cs` now deploys `addresses.db` and `sqlite3.dll` to game directory
- Eliminated 478KB toml++ header dependency from patch DLLs

**What Stayed the Same:**
- Patch configuration (`patch_config.toml`) still uses TOML format
- Hook definitions (`hooks.toml`) still use TOML format
- Manifest files (`manifest.toml`) still use TOML format
- This patch options design is still valid and ready for implementation

**Next Steps for Project:**
1. **Verification Phase** (User's immediate next steps)
   - Build C++ projects in Visual Studio (KotorPatcher.dll, patch DLLs)
   - Test patch installation with SQLite databases
   - Verify game launches and patches work correctly
   - Review all code changes from SQLite migration

2. **Ghidra Database Population**
   - Export function addresses from Ghidra for KOTOR 1 GOG
   - Import into `kotor1_gog_103.db` using SQL or DB Browser for SQLite
   - Populate 24,000+ labeled functions to take full advantage of SQLite scalability

---


## Executive Summary

This document outlines the plan to migrate from TOML-based address databases to SQLite for the KotOR Patch Manager's GameVersion address lookup system. This migration addresses scalability and maintainability concerns as the address database grows from ~90 entries to potentially 24,000+ labeled functions.

## Current State Analysis

### TOML Implementation (Current)

**Files:**
- `AddressDatabases/kotor1_gog_103.toml` (~4.4KB, 168 lines)
- `AddressDatabases/kotor1_steam_103.toml` (~3.8KB, 147 lines)
- `AddressDatabases/kotor2_gog_aspyr.toml` (~3.6KB, 143 lines)
- `AddressDatabases/kotor1_crack_103.toml` 

**Data Volume:**
- 3 game versions
- ~90 addresses per version (15 global pointers, 35-40 functions, 30-35 offsets)
- ~270 total addresses across all versions

**Dependencies:**
- toml++ header-only library (478KB, 17,888 lines) in `Patches/Common/toml.hpp`
- Embedded in every patch DLL that uses GameAPI

**Runtime Architecture:**
- One-time initialization in `DLL_PROCESS_ATTACH`
- Parses entire TOML file into three `std::unordered_map` instances
- Lookups via `GetFunctionAddress(class, name)`, `GetGlobalPointer(name)`, `GetOffset(class, property)`
- Hash map lookups: O(1) average case

### Pain Points

1. **Header Bloat**: 478KB toml++ embedded in every patch DLL
   - Currently 3 patches = 3 copies
   - Future patches will duplicate further
   - Build time impact from parsing 17,888-line header

2. **Manual Maintenance**: TOML editing becomes difficult at scale
   - Finding entries requires scrolling or Ctrl+F
   - No schema validation
   - Easy to introduce formatting errors
   - No tooling for bulk operations

3. **Scalability Concerns**: Planning for 24,000+ functions
   - Multiple games (KOTOR 1 & 2)
   - Hundreds of datatypes with member offsets
   - Multiple versions per game

4. **Duplication**: Same toml++ header in `KPatchCore` and `Patches/Common`

## Proposed SQLite Solution

### Design Principles

1. **Per-version database files**: Separate `.db` file per game version
2. **Shared dependency**: Use `sqlite3.dll`, not embedded amalgamation
3. **Direct queries**: Prepared statements, no in-memory caching
4. **Read-only optimization**: Database opened with read-only flags
5. **Maintain API compatibility**: Minimal changes to existing GameAPI code

### Architecture Decision Rationale

#### Per-Version Database Files

**Decision**: `kotor1_gog_103.db`, `kotor1_steam_103.db`, etc.

**Rationale:**
- Simpler queries (no version filter needed)
- Smaller file sizes per database
- Clear separation of game versions
- Easier to distribute/update specific versions
- Matches current TOML structure (one file per version)

**Alternative Considered**: Single unified database with `game_version` column
- Pros: Single file to manage
- Cons: More complex queries, larger files, version coupling

#### Shared sqlite3.dll

**Decision**: Link against `sqlite3.dll`, distributed with KotorPatcher.dll

**Rationale:**
- Multiple patch DLLs can share the same SQLite library
- Easy to update SQLite version independently
- Smaller patch DLL sizes (~5KB overhead vs ~250KB for amalgamation)
- Standard Windows DLL distribution model

**Deployment:**
- `sqlite3.dll` placed alongside `KotorPatcher.dll`
- C# launcher ensures DLL is present before injection
- Version: SQLite 3.x (latest stable at implementation time)

**Alternative Considered**: Embedded amalgamation
- Pros: No external dependency
- Cons: 250KB added to each patch DLL, harder to update SQLite

#### Direct SQLite Queries (No Cache)

**Decision**: Use prepared statements for every lookup, rely on SQLite's internal caching

**Rationale:**
- Simpler code (no hash map initialization)
- Lower memory footprint per DLL
- SQLite's page cache is highly optimized
- Prepared statements are reused and very fast
- Actual lookup pattern: mostly one-time per class initialization

**Performance Comparison:**
- Current: Hash map lookup ~10-20 nanoseconds
- SQLite prepared statement: ~100-500 nanoseconds
- Impact: Negligible (lookups happen once per class, not per frame)

**Alternative Considered**: Load-all-at-startup cache
- Pros: Matches current behavior exactly
- Cons: More complex code, higher memory usage, defeats purpose of SQLite

#### Integer IDs with Indexed Class Names

**Decision**: Use auto-incrementing integer IDs, index class/function names

**Rationale:**
- Integer comparisons faster than string comparisons for future joins
- Allows for potential relationship tables (e.g., inheritance hierarchies)
- Standard database practice
- Minimal storage overhead

**Alternative Considered**: Composite PRIMARY KEY (class, name)
- Pros: Most natural for current use case
- Cons: Less flexible for future schema evolution

## Database Schema

### Core Schema (Version 1.0)

```sql
-- Database version metadata
CREATE TABLE schema_version (
    version INTEGER PRIMARY KEY,
    applied_date TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);
INSERT INTO schema_version (version) VALUES (1);

-- Game version identification
CREATE TABLE game_version (
    id INTEGER PRIMARY KEY CHECK (id = 1), -- Singleton table
    sha256_hash TEXT NOT NULL UNIQUE,
    game_name TEXT NOT NULL,            -- 'KOTOR1', 'KOTOR2'
    version_string TEXT NOT NULL,       -- 'GOG 1.03', 'Steam 1.03', etc.
    description TEXT
);

-- Function addresses
CREATE TABLE functions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    class_name TEXT NOT NULL,
    function_name TEXT NOT NULL,
    address INTEGER NOT NULL,           -- Stored as integer, displayed as hex
    notes TEXT,                         -- Optional documentation
    UNIQUE(class_name, function_name)
);

-- Global pointer addresses
CREATE TABLE global_pointers (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    pointer_name TEXT NOT NULL UNIQUE,
    address INTEGER NOT NULL,
    notes TEXT
);

-- Class member offsets
CREATE TABLE offsets (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    class_name TEXT NOT NULL,
    member_name TEXT NOT NULL,
    offset INTEGER NOT NULL,
    notes TEXT,
    UNIQUE(class_name, member_name)
);

-- Indices for fast lookups
CREATE INDEX idx_functions_class ON functions(class_name);
CREATE INDEX idx_functions_name ON functions(function_name);
CREATE INDEX idx_offsets_class ON offsets(class_name);
CREATE INDEX idx_offsets_member ON offsets(member_name);
```

### Schema Design Notes

**Integer vs Text Addresses:**
- Addresses stored as `INTEGER` (e.g., `0x005ed700` → `6215424`)
- Display/export formatted as hex strings
- Rationale: Integer operations faster, smaller storage, easier arithmetic

**Notes Column:**
- Optional documentation for each entry
- Can include IDA/Ghidra references, parameter info, etc.
- Useful for maintainability

**Singleton game_version Table:**
- Ensures each database file represents exactly one game version
- SHA-256 hash for validation (matching current TOML behavior)
- `CHECK (id = 1)` constraint enforces singleton pattern

### Example Data

```sql
-- Game version metadata
INSERT INTO game_version (id, sha256_hash, game_name, version_string, description)
VALUES (
    1,
    'a1b2c3d4e5f6...',
    'KOTOR1',
    'GOG 1.03',
    'Star Wars: Knights of the Old Republic - Good Old Games version 1.03'
);

-- Global pointers
INSERT INTO global_pointers (pointer_name, address, notes)
VALUES
    ('AppManager', 0x00903658, 'CAppManager* singleton'),
    ('ClientExoApp', 0x00913C9C, 'CClientExoApp* singleton'),
    ('ServerExoApp', 0x00913CA0, 'CServerExoApp* singleton');

-- Functions
INSERT INTO functions (class_name, function_name, address, notes)
VALUES
    ('CSWSCreatureStats', 'SetCONBase', 0x005ed700, 'void SetCONBase(unsigned char)'),
    ('CVirtualMachine', 'RunScript', 0x005a3b80, 'int RunScript(CExoString*, uint32_t, int)'),
    ('CResRef', 'FromString', 0x004a2c10, 'void FromString(const char*)');

-- Offsets
INSERT INTO offsets (class_name, member_name, offset, notes)
VALUES
    ('CSWSCreatureStats', 'Constitution', 8, 'unsigned char m_nConstitution'),
    ('CSWGuiObject', 'Extent', 52, 'CExoRect m_rExtent'),
    ('CExoString', 'Text', 0, 'char* m_sString');
```

## Example Queries

### Lookup Queries (Production Use)

```sql
-- Get function address
SELECT address
FROM functions
WHERE class_name = 'CSWSCreatureStats'
  AND function_name = 'SetCONBase';

-- Get global pointer
SELECT address
FROM global_pointers
WHERE pointer_name = 'AppManager';

-- Get member offset
SELECT offset
FROM offsets
WHERE class_name = 'CSWSCreatureStats'
  AND member_name = 'Constitution';

-- Verify game version
SELECT sha256_hash
FROM game_version
WHERE id = 1;
```

### Maintenance Queries (Development Use)

```sql
-- List all functions for a class
SELECT function_name, printf('0x%08X', address) as hex_address, notes
FROM functions
WHERE class_name = 'CSWSCreatureStats'
ORDER BY function_name;

-- Find all entries with notes containing specific text
SELECT 'function' as type, class_name, function_name, notes
FROM functions
WHERE notes LIKE '%parameter%'
UNION ALL
SELECT 'offset', class_name, member_name, notes
FROM offsets
WHERE notes LIKE '%parameter%';

-- Count entries by class
SELECT class_name, COUNT(*) as function_count
FROM functions
GROUP BY class_name
ORDER BY function_count DESC;

-- Bulk update addresses (e.g., after ASLR adjustment)
UPDATE functions
SET address = address + 0x10000
WHERE class_name = 'CVirtualMachine';
```

## C++ API Changes

### Current Implementation (TOML)

```cpp
// GameVersion.h
class GameVersion {
public:
    static bool Initialize();
    static void* GetFunctionAddress(const std::string& className, const std::string& functionName);
    static void* GetGlobalPointer(const std::string& pointerName);
    static int GetOffset(const std::string& className, const std::string& propertyName);

private:
    static std::unordered_map<std::string, void*> functionAddresses;
    static std::unordered_map<std::string, void*> globalPointers;
    static std::unordered_map<std::string, int> offsets;
    static bool initialized;
};
```

### Proposed Implementation (SQLite)

```cpp
// GameVersion.h
#include <sqlite3.h>

class GameVersion {
public:
    static bool Initialize();
    static void Shutdown();  // New: cleanup SQLite resources
    static void* GetFunctionAddress(const std::string& className, const std::string& functionName);
    static void* GetGlobalPointer(const std::string& pointerName);
    static int GetOffset(const std::string& className, const std::string& propertyName);

private:
    static sqlite3* db;
    static sqlite3_stmt* stmt_function;
    static sqlite3_stmt* stmt_pointer;
    static sqlite3_stmt* stmt_offset;
    static bool initialized;

    static bool PrepareStatements();
    static void FinalizeStatements();
};
```

### Implementation Example

```cpp
// GameVersion.cpp
#include "GameVersion.h"
#include <stdexcept>
#include <sstream>

sqlite3* GameVersion::db = nullptr;
sqlite3_stmt* GameVersion::stmt_function = nullptr;
sqlite3_stmt* GameVersion::stmt_pointer = nullptr;
sqlite3_stmt* GameVersion::stmt_offset = nullptr;
bool GameVersion::initialized = false;

bool GameVersion::Initialize() {
    if (initialized) return true;

    // Get database path from environment variable
    const char* versionSha = std::getenv("KOTOR_VERSION_SHA");
    if (!versionSha) {
        return false;
    }

    // Construct database filename (e.g., "addresses.db")
    std::string dbPath = "addresses.db";

    // Open database with read-only flags
    int flags = SQLITE_OPEN_READONLY | SQLITE_OPEN_NOMUTEX;
    int rc = sqlite3_open_v2(dbPath.c_str(), &db, flags, nullptr);
    if (rc != SQLITE_OK) {
        return false;
    }

    // Optional: Enable memory-mapped I/O for performance
    // sqlite3_exec(db, "PRAGMA mmap_size=268435456", nullptr, nullptr, nullptr); // 256MB

    // Verify game version SHA matches
    sqlite3_stmt* stmt = nullptr;
    rc = sqlite3_prepare_v2(db, "SELECT sha256_hash FROM game_version WHERE id = 1", -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        sqlite3_close(db);
        return false;
    }

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return false;
    }

    const char* dbHash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    if (!dbHash || std::string(dbHash) != std::string(versionSha)) {
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return false;
    }
    sqlite3_finalize(stmt);

    // Prepare statements for reuse
    if (!PrepareStatements()) {
        sqlite3_close(db);
        return false;
    }

    initialized = true;
    return true;
}

bool GameVersion::PrepareStatements() {
    // Function address lookup
    const char* sql_function = "SELECT address FROM functions WHERE class_name = ? AND function_name = ?";
    if (sqlite3_prepare_v2(db, sql_function, -1, &stmt_function, nullptr) != SQLITE_OK) {
        return false;
    }

    // Global pointer lookup
    const char* sql_pointer = "SELECT address FROM global_pointers WHERE pointer_name = ?";
    if (sqlite3_prepare_v2(db, sql_pointer, -1, &stmt_pointer, nullptr) != SQLITE_OK) {
        sqlite3_finalize(stmt_function);
        return false;
    }

    // Offset lookup
    const char* sql_offset = "SELECT offset FROM offsets WHERE class_name = ? AND member_name = ?";
    if (sqlite3_prepare_v2(db, sql_offset, -1, &stmt_offset, nullptr) != SQLITE_OK) {
        sqlite3_finalize(stmt_function);
        sqlite3_finalize(stmt_pointer);
        return false;
    }

    return true;
}

void GameVersion::FinalizeStatements() {
    if (stmt_function) {
        sqlite3_finalize(stmt_function);
        stmt_function = nullptr;
    }
    if (stmt_pointer) {
        sqlite3_finalize(stmt_pointer);
        stmt_pointer = nullptr;
    }
    if (stmt_offset) {
        sqlite3_finalize(stmt_offset);
        stmt_offset = nullptr;
    }
}

void GameVersion::Shutdown() {
    if (!initialized) return;

    FinalizeStatements();

    if (db) {
        sqlite3_close(db);
        db = nullptr;
    }

    initialized = false;
}

void* GameVersion::GetFunctionAddress(const std::string& className, const std::string& functionName) {
    if (!initialized) {
        throw std::runtime_error("GameVersion not initialized");
    }

    // Bind parameters
    sqlite3_reset(stmt_function);
    sqlite3_bind_text(stmt_function, 1, className.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt_function, 2, functionName.c_str(), -1, SQLITE_TRANSIENT);

    // Execute query
    int rc = sqlite3_step(stmt_function);
    if (rc != SQLITE_ROW) {
        std::stringstream ss;
        ss << "Function address not found: " << className << "::" << functionName;
        throw std::runtime_error(ss.str());
    }

    // Get result (stored as integer)
    sqlite3_int64 address = sqlite3_column_int64(stmt_function, 0);
    return reinterpret_cast<void*>(static_cast<uintptr_t>(address));
}

void* GameVersion::GetGlobalPointer(const std::string& pointerName) {
    if (!initialized) {
        return nullptr;
    }

    // Bind parameter
    sqlite3_reset(stmt_pointer);
    sqlite3_bind_text(stmt_pointer, 1, pointerName.c_str(), -1, SQLITE_TRANSIENT);

    // Execute query
    int rc = sqlite3_step(stmt_pointer);
    if (rc != SQLITE_ROW) {
        return nullptr;
    }

    // Get result
    sqlite3_int64 address = sqlite3_column_int64(stmt_pointer, 0);
    return reinterpret_cast<void*>(static_cast<uintptr_t>(address));
}

int GameVersion::GetOffset(const std::string& className, const std::string& propertyName) {
    if (!initialized) {
        throw std::runtime_error("GameVersion not initialized");
    }

    // Bind parameters
    sqlite3_reset(stmt_offset);
    sqlite3_bind_text(stmt_offset, 1, className.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt_offset, 2, propertyName.c_str(), -1, SQLITE_TRANSIENT);

    // Execute query
    int rc = sqlite3_step(stmt_offset);
    if (rc != SQLITE_ROW) {
        std::stringstream ss;
        ss << "Offset not found: " << className << "::" << propertyName;
        throw std::runtime_error(ss.str());
    }

    // Get result
    return sqlite3_column_int(stmt_offset, 0);
}
```

### DLL_PROCESS_ATTACH Integration

```cpp
// In patch DLL main entry point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            if (!GameVersion::Initialize()) {
                return FALSE;
            }
            // ... rest of initialization
            break;

        case DLL_PROCESS_DETACH:
            GameVersion::Shutdown();  // New: cleanup
            break;
    }
    return TRUE;
}
```

## Migration Tooling

### TOML to SQLite Converter (C#)

A C# command-line tool to convert existing TOML files to SQLite databases:

```csharp
// TomlToSqlite/Program.cs
using System;
using System.IO;
using Microsoft.Data.Sqlite;
using Tomlyn;
using Tomlyn.Model;

class Program {
    static void Main(string[] args) {
        if (args.Length != 2) {
            Console.WriteLine("Usage: TomlToSqlite <input.toml> <output.db>");
            return;
        }

        string tomlPath = args[0];
        string dbPath = args[1];

        // Delete existing database
        if (File.Exists(dbPath)) {
            File.Delete(dbPath);
        }

        // Parse TOML
        var tomlContent = File.ReadAllText(tomlPath);
        var tomlTable = Toml.ToModel(tomlContent);

        // Create SQLite database
        using var connection = new SqliteConnection($"Data Source={dbPath}");
        connection.Open();

        // Create schema
        CreateSchema(connection);

        // Insert game version metadata
        InsertGameVersion(connection, tomlTable, Path.GetFileNameWithoutExtension(tomlPath));

        // Insert data
        InsertGlobalPointers(connection, tomlTable);
        InsertFunctions(connection, tomlTable);
        InsertOffsets(connection, tomlTable);

        Console.WriteLine($"Successfully converted {tomlPath} to {dbPath}");
    }

    static void CreateSchema(SqliteConnection connection) {
        string schema = @"
            CREATE TABLE schema_version (
                version INTEGER PRIMARY KEY,
                applied_date TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
            );
            INSERT INTO schema_version (version) VALUES (1);

            CREATE TABLE game_version (
                id INTEGER PRIMARY KEY CHECK (id = 1),
                sha256_hash TEXT NOT NULL UNIQUE,
                game_name TEXT NOT NULL,
                version_string TEXT NOT NULL,
                description TEXT
            );

            CREATE TABLE functions (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                class_name TEXT NOT NULL,
                function_name TEXT NOT NULL,
                address INTEGER NOT NULL,
                notes TEXT,
                UNIQUE(class_name, function_name)
            );

            CREATE TABLE global_pointers (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                pointer_name TEXT NOT NULL UNIQUE,
                address INTEGER NOT NULL,
                notes TEXT
            );

            CREATE TABLE offsets (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                class_name TEXT NOT NULL,
                member_name TEXT NOT NULL,
                offset INTEGER NOT NULL,
                notes TEXT,
                UNIQUE(class_name, member_name)
            );

            CREATE INDEX idx_functions_class ON functions(class_name);
            CREATE INDEX idx_functions_name ON functions(function_name);
            CREATE INDEX idx_offsets_class ON offsets(class_name);
            CREATE INDEX idx_offsets_member ON offsets(member_name);
        ";

        using var cmd = connection.CreateCommand();
        cmd.CommandText = schema;
        cmd.ExecuteNonQuery();
    }

    static void InsertGameVersion(SqliteConnection connection, TomlTable tomlTable, string fileName) {
        var versionTable = (TomlTable)tomlTable["version"];
        string sha256 = (string)versionTable["sha256"];

        // Parse game name and version from filename (e.g., "kotor1_gog_103")
        var parts = fileName.Split('_');
        string gameName = parts[0].ToUpper();
        string versionString = string.Join(" ", parts.Skip(1));

        using var cmd = connection.CreateCommand();
        cmd.CommandText = @"
            INSERT INTO game_version (id, sha256_hash, game_name, version_string)
            VALUES (1, @sha256, @game, @version)
        ";
        cmd.Parameters.AddWithValue("@sha256", sha256);
        cmd.Parameters.AddWithValue("@game", gameName);
        cmd.Parameters.AddWithValue("@version", versionString);
        cmd.ExecuteNonQuery();
    }

    static void InsertGlobalPointers(SqliteConnection connection, TomlTable tomlTable) {
        if (!tomlTable.ContainsKey("global_pointers")) return;

        var pointers = (TomlTable)tomlTable["global_pointers"];
        using var cmd = connection.CreateCommand();
        cmd.CommandText = @"
            INSERT INTO global_pointers (pointer_name, address)
            VALUES (@name, @address)
        ";

        foreach (var kvp in pointers) {
            long address = ParseAddress(kvp.Value);
            cmd.Parameters.Clear();
            cmd.Parameters.AddWithValue("@name", kvp.Key);
            cmd.Parameters.AddWithValue("@address", address);
            cmd.ExecuteNonQuery();
        }
    }

    static void InsertFunctions(SqliteConnection connection, TomlTable tomlTable) {
        if (!tomlTable.ContainsKey("functions")) return;

        var functionsTable = (TomlTable)tomlTable["functions"];
        using var cmd = connection.CreateCommand();
        cmd.CommandText = @"
            INSERT INTO functions (class_name, function_name, address)
            VALUES (@class, @function, @address)
        ";

        foreach (var classKvp in functionsTable) {
            string className = classKvp.Key;
            var classTable = (TomlTable)classKvp.Value;

            foreach (var funcKvp in classTable) {
                string functionName = funcKvp.Key;
                long address = ParseAddress(funcKvp.Value);

                cmd.Parameters.Clear();
                cmd.Parameters.AddWithValue("@class", className);
                cmd.Parameters.AddWithValue("@function", functionName);
                cmd.Parameters.AddWithValue("@address", address);
                cmd.ExecuteNonQuery();
            }
        }
    }

    static void InsertOffsets(SqliteConnection connection, TomlTable tomlTable) {
        if (!tomlTable.ContainsKey("offsets")) return;

        var offsetsTable = (TomlTable)tomlTable["offsets"];
        using var cmd = connection.CreateCommand();
        cmd.CommandText = @"
            INSERT INTO offsets (class_name, member_name, offset)
            VALUES (@class, @member, @offset)
        ";

        foreach (var classKvp in offsetsTable) {
            string className = classKvp.Key;
            var classTable = (TomlTable)classKvp.Value;

            foreach (var memberKvp in classTable) {
                string memberName = memberKvp.Key;
                long offset = Convert.ToInt64(memberKvp.Value);

                cmd.Parameters.Clear();
                cmd.Parameters.AddWithValue("@class", className);
                cmd.Parameters.AddWithValue("@member", memberName);
                cmd.Parameters.AddWithValue("@offset", offset);
                cmd.ExecuteNonQuery();
            }
        }
    }

    static long ParseAddress(object value) {
        string str = value.ToString();
        if (str.StartsWith("0x")) {
            return Convert.ToInt64(str.Substring(2), 16);
        }
        return Convert.ToInt64(str);
    }
}
```

### Batch Conversion Script

```bash
#!/bin/bash
# convert_all_databases.sh

for toml in AddressDatabases/*.toml; do
    db="${toml%.toml}.db"
    echo "Converting $toml -> $db"
    dotnet run --project tools/TomlToSqlite -- "$toml" "$db"
done
```

### SQL Maintenance Scripts

**export_to_sql.sql** - Export database to SQL statements for version control:
```sql
.output kotor1_gog_103.sql
.dump
.output stdout
```

**import_from_ghidra.sql** - Template for importing Ghidra function exports:
```sql
-- After exporting function list from Ghidra as CSV: class_name,function_name,address
.mode csv
.import ghidra_functions.csv temp_functions

INSERT INTO functions (class_name, function_name, address)
SELECT class_name, function_name, CAST(address AS INTEGER)
FROM temp_functions
WHERE NOT EXISTS (
    SELECT 1 FROM functions
    WHERE functions.class_name = temp_functions.class_name
      AND functions.function_name = temp_functions.function_name
);

DROP TABLE temp_functions;
```

## Developer Tooling

### Recommended Tools

#### Primary: DB Browser for SQLite
- **Website**: https://sqlitebrowser.org/
- **Platform**: Windows, macOS, Linux
- **License**: Free, open-source
- **Features**:
  - Visual table editor
  - SQL query execution
  - Schema designer
  - CSV import/export
  - Hex editor for BLOB data
- **Use Case**: Daily development, manual edits, browsing data

#### Secondary: SQL Scripts + Any Viewer
- **Approach**: Maintain `.sql` scripts for reproducible schema/data changes
- **Version Control**: Commit SQL scripts, not binary `.db` files (unless using Git LFS)
- **Tools**: Any text editor + `sqlite3` CLI
- **Use Case**: Automation, batch operations, code review

### Workflow Recommendations

**For manual additions:**
1. Open database in DB Browser for SQLite
2. Navigate to "Browse Data" tab
3. Select table (functions, global_pointers, offsets)
4. Click "New Record" button
5. Fill in fields (address can be entered as hex: `0x005ed700`)
6. Save changes

**For bulk imports:**
1. Export data from IDA/Ghidra as CSV
2. Write SQL import script (see `import_from_ida.sql` above)
3. Execute via DB Browser or `sqlite3` CLI
4. Verify results with SELECT queries

**For version control:**
1. Option A: Commit `.db` files directly (if small)
2. Option B: Use Git LFS for `.db` files
3. Option C: Export to `.sql` and commit scripts (most diff-friendly)

**Best Practice**: Hybrid approach
- Commit `.db` files with Git LFS
- Also commit periodic `.sql` dumps for human review
- Use SQL scripts for major schema changes

## Build System Changes

### C++ Project Changes

**KotorPatcher.vcxproj**:
```xml
<ItemGroup>
  <None Include="sqlite3.dll">
    <CopyToOutputDirectory>PreserveNewest</CopyToOutputDirectory>
  </None>
</ItemGroup>

<ItemGroup>
  <ClInclude Include="sqlite3.h" />
</ItemGroup>

<ItemGroup>
  <Library Include="sqlite3.lib" />
</ItemGroup>
```

**Include Paths**:
- Add `$(ProjectDir)lib\sqlite3\` to Additional Include Directories

**Linker**:
- Add `sqlite3.lib` to Additional Dependencies
- Add `$(ProjectDir)lib\sqlite3\` to Additional Library Directories

### File Structure

```
KotorPatchManager/
├── lib/
│   └── sqlite3/
│       ├── sqlite3.h         # Header file (from sqlite.org)
│       ├── sqlite3.lib       # Import library
│       └── sqlite3.dll       # Runtime DLL
├── AddressDatabases/
│   ├── kotor1_gog_103.db     # SQLite databases (replacing .toml)
│   ├── kotor1_steam_103.db
│   └── kotor2_gog_aspyr.db
├── tools/
│   └── TomlToSqlite/         # Converter tool
│       ├── TomlToSqlite.csproj
│       └── Program.cs
└── scripts/
    ├── convert_all_databases.sh
    ├── export_to_sql.sql
    └── import_from_ida.sql
```

### Deployment Changes

**KPatchCore** (C# installer):
- Copy `sqlite3.dll` to game directory alongside `KotorPatcher.dll`
- Copy appropriate `addresses.db` (renamed from version-specific .db)
- Verify SQLite version compatibility

**PatchApplicator.cs** changes:
```csharp
public PatchResult Apply(Patch patch, string gamePath) {
    // ...existing code...

    // Copy SQLite DLL if not present
    string sqliteDll = Path.Combine(gamePath, "sqlite3.dll");
    if (!File.Exists(sqliteDll)) {
        File.Copy(
            Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "sqlite3.dll"),
            sqliteDll
        );
    }

    // Copy appropriate addresses database
    string gameVersion = DetectGameVersion(gamePath);
    string sourceDb = Path.Combine("AddressDatabases", $"{gameVersion}.db");
    string targetDb = Path.Combine(gamePath, "addresses.db");
    File.Copy(sourceDb, targetDb, overwrite: true);

    // ...existing code...
}
```

## Performance Considerations

### Read-Only Optimization Flags

```cpp
// GameVersion::Initialize()
int flags = SQLITE_OPEN_READONLY | SQLITE_OPEN_NOMUTEX;
int rc = sqlite3_open_v2(dbPath.c_str(), &db, flags, nullptr);
```

**SQLITE_OPEN_READONLY**:
- Prevents accidental writes
- Allows shared cache across connections (if enabled)
- Database opened in immutable mode

**SQLITE_OPEN_NOMUTEX**:
- Skips mutex locking overhead
- Safe because we never write from runtime code
- Single-threaded access pattern (patches run on game main thread)

**Benefit**: ~10-15% faster queries on read-only workloads

### Memory-Mapped I/O (Optional)

```cpp
// After opening database
sqlite3_exec(db, "PRAGMA mmap_size=268435456", nullptr, nullptr, nullptr); // 256MB
```

**How it works**:
- Database file mapped directly into process memory
- OS page cache handles I/O automatically
- Reduces syscall overhead

**When to use**:
- Databases > 10MB
- Frequent random access patterns
- Sufficient RAM available

**Benchmarks** (estimated for 24,000 entries):
- Database size: ~1.5MB
- Memory-mapped: Likely minimal benefit at this size
- Recommendation: **Optional, enable if database exceeds 10MB**

### Prepared Statement Reuse

```cpp
// One-time preparation
sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

// Repeated use
for (int i = 0; i < 1000; i++) {
    sqlite3_reset(stmt);
    sqlite3_bind_text(stmt, 1, param, -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
}

// Cleanup
sqlite3_finalize(stmt);
```

**Benefit**: ~50x faster than re-parsing SQL each time

**Current Design**: Prepared statements created once in `Initialize()`, reused for all lookups

### Performance Benchmark (Estimated)

Comparing TOML vs SQLite for 1,000 lookups:

| Operation | TOML (Current) | SQLite (Proposed) | Notes |
|-----------|----------------|-------------------|-------|
| Initialization | ~0.5ms | ~1.5ms | One-time cost, negligible |
| Single lookup | ~15ns | ~200ns | Hash map vs prepared statement |
| 1,000 lookups | ~15μs | ~200μs | Still sub-millisecond |
| Memory usage | ~10KB | ~5KB + SQLite | SQLite page cache adds ~2MB |
| DLL size overhead | +478KB (toml++) | +5KB (linking) | Major improvement |

**Verdict**: Slightly slower lookups (~10x), but still imperceptible. Massive reduction in DLL bloat.

## Migration Timeline

### Phase 1: Foundation (Week 1)
- ✅ Document SQLite migration plan (this document)
- Create TomlToSqlite converter tool
- Convert existing 3 TOML files to SQLite
- Test databases with SQLite CLI and DB Browser

### Phase 2: C++ Refactor (Week 2)
- Download and integrate sqlite3.dll/lib/header
- Refactor GameVersion class to use SQLite
- Update build system (vcxproj, include paths)
- Test with existing patches (EnableScriptAurPostString, etc.)

### Phase 3: C# Integration (Week 3)
- Update PatchApplicator to deploy sqlite3.dll
- Update ConfigGenerator to reference .db files
- Test full install/uninstall workflow
- Verify game version detection with SQLite

### Phase 4: Cleanup & Documentation (Week 4)
- Remove toml++ dependency from Patches/Common
- Update patch development documentation
- Create developer tooling guide
- Write migration guide for custom patches

### Phase 5: Testing & Rollout
- Test with all existing patches
- Verify performance (benchmarks)
- Test on multiple game versions
- Release updated patch manager

## Backward Compatibility

### Supporting Both TOML and SQLite (Optional)

If gradual migration is desired:

```cpp
bool GameVersion::Initialize() {
    // Try SQLite first
    if (InitializeSqlite()) {
        return true;
    }

    // Fall back to TOML
    return InitializeToml();
}
```

**Recommendation**: Clean break, no dual support needed
- TOML files small enough to convert in one go
- Only 3 official game versions
- Custom patches can migrate with documentation

## Security Considerations

### SQL Injection Prevention

**Not applicable**: All inputs are hardcoded class/function names in C++ code, no user input.

**Safe practices**:
- Always use parameterized queries (we do: `sqlite3_bind_text`)
- Never concatenate user input into SQL strings
- Read-only mode prevents modification attacks

### File Integrity

**Current**: SHA-256 validation of game executable
**Proposed**: Also validate database file

```cpp
bool GameVersion::Initialize() {
    // ...existing code...

    // Optional: Verify database integrity
    sqlite3_exec(db, "PRAGMA integrity_check", nullptr, nullptr, nullptr);

    // ...rest of initialization...
}
```

**Recommendation**: Optional, adds ~5ms to initialization

## Git Configuration

### Git LFS Setup (Recommended)

**.gitattributes**:
```
*.db filter=lfs diff=lfs merge=lfs -text
```

**Installation**:
```bash
git lfs install
git lfs track "*.db"
git add .gitattributes
git commit -m "Configure Git LFS for SQLite databases"
```

**Rationale**: Binary .db files don't diff well, LFS prevents repo bloat

### Alternative: SQL Dumps

**Periodic export**:
```bash
sqlite3 AddressDatabases/kotor1_gog_103.db .dump > AddressDatabases/kotor1_gog_103.sql
```

**Commit both**:
- `.db` for runtime use (via LFS)
- `.sql` for human review and diffing

## Future Enhancements

### Advanced Schema (v2.0+)

**Class Inheritance Hierarchy**:
```sql
CREATE TABLE classes (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE,
    parent_class_id INTEGER,
    FOREIGN KEY (parent_class_id) REFERENCES classes(id)
);

-- Functions now reference class IDs
ALTER TABLE functions ADD COLUMN class_id INTEGER REFERENCES classes(id);
```

**Benefits**:
- Queries like "get all functions for CSWSObject and its subclasses"
- Better modeling of actual C++ inheritance
- Reduce duplication of inherited offsets

**Parameter Metadata**:
```sql
CREATE TABLE function_parameters (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    function_id INTEGER NOT NULL,
    parameter_index INTEGER NOT NULL,
    parameter_type TEXT NOT NULL,
    parameter_name TEXT,
    FOREIGN KEY (function_id) REFERENCES functions(id)
);
```

**Benefits**:
- Document function signatures
- Automated wrapper generation
- Better IDE integration

### Multi-Game Support

**Unified database with game filtering**:
```sql
CREATE TABLE games (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE  -- 'KOTOR1', 'KOTOR2'
);

CREATE TABLE game_versions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    game_id INTEGER NOT NULL,
    version_string TEXT NOT NULL,
    sha256_hash TEXT NOT NULL UNIQUE,
    FOREIGN KEY (game_id) REFERENCES games(id)
);

-- Functions now reference game versions
ALTER TABLE functions ADD COLUMN game_version_id INTEGER REFERENCES game_versions(id);
```

**Rationale**: Only consider if managing 10+ game versions across multiple titles



## Open Questions

1. **SQLite Version**: Which version to ship? Latest stable (3.51.x) or LTS?
   - **Recommendation**: Latest stable, update periodically

2. **32-bit vs 64-bit**: KOTOR is 32-bit, do we need 32-bit SQLite DLL?
   - **Answer**: Yes, must match game architecture

3. **Multiple Game Versions**: Support multiple installed game versions simultaneously?
   - **Current Approach**: One version per installation, selected at install time

4. **Database Updates**: How to handle schema migrations when updating patch manager?
   - **Recommendation**: Schema version table, migration scripts if needed

## Conclusion

Migrating from TOML to SQLite addresses the core concerns:

✅ **Scalability**: SQLite handles 24,000+ entries efficiently
✅ **Maintainability**: DB Browser and SQL scripts simplify bulk edits
✅ **Performance**: Prepared statements fast enough (~200ns per lookup)
✅ **Dependency Reduction**: Eliminates 478KB toml++ header bloat
✅ **Tooling**: Industry-standard SQLite tools available
✅ **File Management**: Single `.db` file per version, optionally with Git LFS

**Primary Benefit**: Eliminates 478KB header duplication across every patch DLL

**Trade-off**: Slightly slower lookups (200ns vs 15ns), but imperceptible in practice

**Recommendation**: Proceed with migration as planned, prioritize during natural refactoring cycle

## References

- SQLite Official Site: https://www.sqlite.org/
- SQLite C/C++ Interface: https://www.sqlite.org/cintro.html
- DB Browser for SQLite: https://sqlitebrowser.org/
- SQLite Performance Tuning: https://www.sqlite.org/speed.html
- Git LFS: https://git-lfs.github.com/

---

**Document Version**: 1.1.0
**Author**: Generated during Claude Code session
**Date**: 2025-12-24
**Status**: Implementation In Progress - Core Migration Complete

## Implementation Status (as of 2025-12-24)

### ✅ Completed Tasks
1. **Downloaded and configured SQLite (32-bit)**
   - sqlite3.dll, sqlite3.lib, sqlite3.h placed in `lib/` directory
   - Updated .gitignore to allow checking in lib dependencies

2. **Created TOML to SQLite converter tool**
   - C# tool at `tools/TomlToSqlite/`
   - Successfully converted all 4 existing TOML databases to SQLite

3. **Updated C++ GameVersion class**
   - Refactored `Patches/Common/GameAPI/GameVersion.cpp` and `.h`
   - Now uses SQLite with prepared statements instead of TOML/hash maps
   - Added `Shutdown()` method for cleanup

4. **Updated C++ build configuration**
   - Modified `src/KotorPatcher/KotorPatcher.vcxproj` to link sqlite3.lib
   - Updated `Patches/create-patch.bat` to include SQLite in patch builds

5. **Updated C# PatchApplicator**
   - Modified to deploy `.db` files instead of `.toml`
   - Added deployment of `sqlite3.dll` alongside `KotorPatcher.dll`
   - Added Microsoft.Data.Sqlite package to KPatchCore

### 🔄 Remaining Tasks

**📋 See [`docs/NEXT_STEPS.md`](NEXT_STEPS.md) for detailed step-by-step instructions on what to do next.**

1. **Build C++ projects** (requires Visual Studio)
   - Build `KotorPatcher.dll` with SQLite support
   - Rebuild existing patch DLLs with new GameVersion code

2. **Testing & Verification**
   - Test patch installation with SQLite databases
   - Verify game launches with patches
   - Confirm all address lookups work correctly
   - Review all code changes from migration

3. **Ghidra Database Population**
   - Export function addresses from Ghidra for KOTOR 1 GOG
   - Import 24,000+ functions into `kotor1_gog_103.db`
   - Test with expanded database

4. **Documentation cleanup**
   - Update patch development guide
   - Remove references to TOML in developer docs
