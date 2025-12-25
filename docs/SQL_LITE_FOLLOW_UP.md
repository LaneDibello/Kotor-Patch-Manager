# SQLite Database Evolution - Remaining Work

## Completed Work ✓

**Phase 1: SqliteTools Foundation** ✅ COMPLETE
- Created `tools/SqliteTools/` multi-command CLI tool
- Implemented commands: `toml-to-sqlite`, `import-ghidra`, `migrate`, `validate`
- Added to `KotorPatchManager.sln` under "Tools" folder
- All C# projects build successfully

**Phase 2: Schema Migration** ✅ COMPLETE
- Created migration `002_add_function_metadata.sql`
- Added columns: `calling_convention` (TEXT), `param_size_bytes` (INTEGER)
- Migrated all 4 databases from schema v1 → v2
- Backward compatible: NULL-able columns work with existing C++ code

**Phase 3: Ghidra CSV Import** ✅ COMPLETE
- Implemented `ImportGhidraCommand` with upsert logic (ON CONFLICT DO UPDATE)
- CSV format: `class_name`, `function_name`, `address`, `calling_convention`, `param_size_bytes`, `notes`
- Tested successfully with sample data
- Ready for bulk Ghidra exports

**Current Database Status:**
- All databases at schema v2
- Ready to receive Ghidra imports with metadata
- Validated and working

---

## Usage Reference

### Import Ghidra CSV Export
```bash
# From Windows Command Prompt or PowerShell:
cd "C:\Users\laned\source\Repos\KotOR Patch Manager"
dotnet run --project tools\SqliteTools -- import-ghidra --csv "path\to\functions.csv" --database "AddressDatabases\kotor1_gog_103.db"
```

**CSV Format:**
```csv
class_name,function_name,address,calling_convention,param_size_bytes,notes
CSWSCreatureStats,SetCONBase,0x005ed700,__thiscall,4,void SetCONBase(unsigned char value)
CVirtualMachine,RunScript,0x005a3b80,__thiscall,12,int RunScript(CExoString* script uint32_t objId int unknown)
Global,TestFunction,0x00401000,__cdecl,8,int TestFunction(int a int b)
```

**Notes:**
- Duplicate class_name + function_name combinations will **UPDATE** existing entries (not create duplicates)
- You can re-import CSV files multiple times safely
- Optional `--mode replace` will delete all functions before importing

### Validate Database
```bash
dotnet run --project tools\SqliteTools -- validate --database "AddressDatabases\kotor1_gog_103.db"
```

---

## Remaining Work

### Phase 4: Multi-Version Database Support (OPTIONAL - Not Started)

**Status:** Deferred until needed

**Goal:** Allow multiple game versions (GOG/Steam/Crack) to share the same address database.

**Why:** KOTOR1 Steam = GOG after DRM removal, Crack version is 99.9% identical. Currently maintaining 3 separate databases with identical function addresses.

#### Implementation Steps:

**1. Create Migration 003**

File: `tools/SqliteTools/Migrations/003_allow_multiple_game_versions.sql`

```sql
-- Migration v2 → v3: Allow multiple game versions per database

-- Step 1: Create new table without CHECK constraint
CREATE TABLE game_version_new (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    sha256_hash TEXT NOT NULL UNIQUE,
    game_name TEXT NOT NULL,
    version_string TEXT NOT NULL,
    description TEXT
);

-- Step 2: Copy existing data
INSERT INTO game_version_new (id, sha256_hash, game_name, version_string, description)
SELECT id, sha256_hash, game_name, version_string, description FROM game_version;

-- Step 3: Drop old table
DROP TABLE game_version;

-- Step 4: Rename new table
ALTER TABLE game_version_new RENAME TO game_version;

-- Update schema version
INSERT INTO schema_version (version) VALUES (3);
```

**2. Update MigrateSchemaCommand.cs**

Add migration 003 to the sequence (around line 226):

```csharp
// Apply migrations in sequence
if (currentVersion < 2) {
    ApplyMigration(connection, "002_add_function_metadata.sql");
    Console.WriteLine("Applied migration 002: Add function metadata");
}

if (currentVersion < 3) {
    ApplyMigration(connection, "003_allow_multiple_game_versions.sql");
    Console.WriteLine("Applied migration 003: Allow multiple game versions");
}
```

**3. Update PatchApplicator.cs (line 421)**

Change query from singleton to SHA lookup:

**Current:**
```csharp
command.CommandText = "SELECT sha256_hash FROM game_version WHERE id = 1";
```

**New:**
```csharp
command.CommandText = "SELECT sha256_hash FROM game_version WHERE sha256_hash = @targetSha";
command.Parameters.AddWithValue("@targetSha", gameVersion.Hash);
```

**4. Update GameVersion.cpp (lines 70-98)**

Change query from singleton to SHA lookup:

**Current:**
```cpp
rc = sqlite3_prepare_v2(db, "SELECT sha256_hash FROM game_version WHERE id = 1", -1, &stmt, nullptr);
```

**New:**
```cpp
std::string sql = "SELECT sha256_hash FROM game_version WHERE sha256_hash = ?";
rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
if (rc != SQLITE_OK) { /* error handling */ }

sqlite3_bind_text(stmt, 1, versionSha.c_str(), -1, SQLITE_TRANSIENT);

rc = sqlite3_step(stmt);
if (rc == SQLITE_ROW) {
    OutputDebugStringA(("[GameVersion] Version SHA validated: " + versionSha.substr(0, 16) + "...\n").c_str());
} else {
    OutputDebugStringA("[GameVersion] ERROR: Game version SHA not found in database\n");
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    db = nullptr;
    return false;
}
```

**5. Create Shared Database**

```bash
# Run migration 003 on kotor1_gog_103.db
dotnet run --project tools\SqliteTools -- migrate --database AddressDatabases\kotor1_gog_103.db

# Add Steam SHA (identical to GOG after DRM)
# Use DB Browser for SQLite or SQL command to insert:
INSERT INTO game_version (sha256_hash, game_name, version_string, description)
VALUES ('34E6D971C034222A417995D8E1E8FDD9F8781795C9C289BD86C499A439F34C88',
        'KOTOR1', 'Steam 1.03', 'Identical to GOG after DRM decryption');

# Add Crack SHA (99.9% identical)
INSERT INTO game_version (sha256_hash, game_name, version_string, description)
VALUES ('761F9466F456A83909036BAEBB5C43167D722387BE66E54617BA20A8C49E9886',
        'KOTOR1', 'Crack 1.03', 'CD crack - 99.9% identical to GOG');

# Rename database
# mv AddressDatabases/kotor1_gog_103.db AddressDatabases/kotor1_shared.db

# Delete redundant databases
# rm AddressDatabases/kotor1_steam_103.db
# rm AddressDatabases/kotor1_crack_103.db
```

**6. Test Installation**

- Test patch installation with Steam KOTOR1 executable
- Test patch installation with Crack KOTOR1 executable
- Verify correct database is selected via SHA match
- Confirm patches apply correctly

**Safety Net:**
The existing `original_bytes` validation in patch hooks will catch any differences between game versions. If Steam/GOG/Crack differ at a hook address, the hook will fail safely without being applied.

---

## Implementation Checklist

### Phase 1: SqliteTools Foundation ✅
- [x] Restructure `tools/TomlToSqlite/` → `tools/SqliteTools/`
- [x] Create command framework (Program.cs, ICommand.cs)
- [x] Refactor TomlToSqliteCommand
- [x] Add to KotorPatchManager.sln

### Phase 2: Schema Migration ✅
- [x] Implement MigrateSchemaCommand
- [x] Create migration 002 (add calling_convention, param_size_bytes)
- [x] Run migration on all existing databases
- [x] Test backward compatibility with C++

### Phase 3: Ghidra Import ✅
- [x] Implement ImportGhidraCommand
- [x] Create GhidraFunction model
- [x] Test CSV import with sample data

### Phase 4: Multi-Version Support ❌ (Optional - Deferred)
- [ ] Create migration 003 (remove id=1 constraint)
- [ ] Update MigrateSchemaCommand to include migration 003
- [ ] Update PatchApplicator query logic
- [ ] Update GameVersion.cpp query logic
- [ ] Create kotor1_shared.db with multiple SHAs
- [ ] Test installation with Steam/GOG/Crack versions

### Phase 5: Validation & Documentation
- [x] Implement ValidateCommand
- [ ] Test entire workflow end-to-end with real Ghidra data
- [ ] Update NEXT_STEPS.md with completed work

---

## Files Modified in This Implementation

### Created Files
- `tools/SqliteTools/SqliteTools.csproj`
- `tools/SqliteTools/Program.cs`
- `tools/SqliteTools/Commands/ICommand.cs`
- `tools/SqliteTools/Commands/TomlToSqliteCommand.cs`
- `tools/SqliteTools/Commands/ImportGhidraCommand.cs`
- `tools/SqliteTools/Commands/MigrateSchemaCommand.cs`
- `tools/SqliteTools/Commands/ValidateCommand.cs`
- `tools/SqliteTools/Models/GhidraFunction.cs`
- `tools/SqliteTools/Migrations/002_add_function_metadata.sql`

### Modified Files
- `KotorPatchManager.sln` - Added SqliteTools project under "Tools" folder

### To Modify (Phase 4)
- `tools/SqliteTools/Migrations/003_allow_multiple_game_versions.sql` (create)
- `tools/SqliteTools/Commands/MigrateSchemaCommand.cs` (add migration 003)
- `src/KPatchCore/Applicators/PatchApplicator.cs` (line 421)
- `Patches/Common/GameAPI/GameVersion.cpp` (lines 70-98)

### To Delete (After Phase 4)
- `tools/TomlToSqlite/` (old converter, replaced by SqliteTools)
- `AddressDatabases/kotor1_steam_103.db` (merged into shared)
- `AddressDatabases/kotor1_crack_103.db` (merged into shared)

---

## Notes

**Ghidra Import:**
- Import is **incremental and safe** - duplicate class/function names will update existing entries
- You can re-import multiple times as you label more functions in Ghidra
- Use `--mode append` (default) for upsert behavior, or `--mode replace` to clear all functions first

**Database Schema:**
- All 4 databases currently at v2 (schema_version = 2)
- New columns are NULL-able for backward compatibility
- C++ GameVersion code works with both v1 and v2 schemas

**Phase 4 Decision:**
- Multi-version support is optional but recommended to reduce database duplication
- Can be implemented at any time without affecting existing functionality
- Safety guaranteed by original_bytes hook validation

**Next Immediate Steps:**
1. Export function data from Ghidra to CSV format
2. Import CSV into kotor1_gog_103.db using SqliteTools
3. Test that patches still work with enhanced database
4. Consider Phase 4 if multi-version support becomes desirable
