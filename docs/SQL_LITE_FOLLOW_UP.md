# Phase 4: Multi-Version Database Support

## Overview
Enable multiple game versions (with the same Version string) to share a single address database. This eliminates database duplication for KOTOR 1 GOG/Steam/Crack versions which are nearly identical.

## Completed Prerequisites
- ✅ Phase 1: SqliteTools Foundation - Multi-command CLI tool
- ✅ Phase 2: Schema Migration - Automated migration system
- ✅ Phase 3: Ghidra Import - CSV import with metadata

## Current Scope
Focus on KOTOR 1 versions that share Version="1.0.3":
- GOG: `9C10E0450A6EECA417E036E3CDE7474FED1F0A92AAB018446D156944DEA91435`
- Crack: `761F9466F456A83909036BAEBB5C43167D722387BE66E54617BA20A8C49E9886`
- Steam: `34E6D971C034222A417995D8E1E8FDD9F8781795C9C289BD86C499A439F34C88`

---

## Problem Statement

**Current Limitation:**
- `game_version` table has `CHECK (id = 1)` constraint - only 1 row allowed
- Databases can only represent ONE game version
- KOTOR 1 has 3 nearly-identical versions (GOG/Steam/Crack) requiring 3 separate databases

**Current Queries:**
- **C# PatchApplicator.cs:421**: `SELECT sha256_hash FROM game_version WHERE id = 1`
- **C++ GameVersion.cpp:70**: `SELECT sha256_hash FROM game_version WHERE id = 1`

Both query for `id = 1`, expecting exactly one row per database.

**Desired Behavior:**
- Remove singleton constraint
- Allow multiple SHAs in one database
- Query: `SELECT sha256_hash FROM game_version WHERE sha256_hash = ?`
- Database returns match if it supports that specific game version

**Safety Net:**
Existing `original_bytes` validation in hook application catches any real differences between game versions. If addresses differ, hooks fail safely without being applied.

---

## Implementation Steps

### Step 1: Create Migration 003

**File:** `tools/SqliteTools/Migrations/003_allow_multiple_game_versions.sql`

**Purpose:** Remove the `CHECK (id = 1)` constraint to allow multiple game_version rows.

**SQL Migration:**
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

**Why this approach?**
- SQLite doesn't support `ALTER TABLE ... DROP CONSTRAINT`
- Must recreate table without the constraint
- Preserves all existing data via INSERT...SELECT

---

### Step 2: Update MigrateSchemaCommand

**File:** `tools/SqliteTools/Commands/MigrateSchemaCommand.cs`

**Location:** Add after migration 002 check (around line 30)

**Change:**
```csharp
// Apply migrations in sequence
if (currentVersion < 2) {
    ApplyMigration(connection, "002_add_function_metadata.sql");
    Console.WriteLine("Applied migration 002: Add function metadata columns");
}

// NEW: Add this block
if (currentVersion < 3) {
    ApplyMigration(connection, "003_allow_multiple_game_versions.sql");
    Console.WriteLine("Applied migration 003: Allow multiple game versions");
}
```

---

### Step 3: Update C# PatchApplicator Query

**File:** `src/KPatchCore/Applicators/PatchApplicator.cs`

**Line:** 421

**Current Code:**
```csharp
command.CommandText = "SELECT sha256_hash FROM game_version WHERE id = 1";
var sha = command.ExecuteScalar() as string;

if (sha == gameVersion.Hash)
{
    matchingAddressDb = dbFile;
    break;
}
```

**New Code:**
```csharp
command.CommandText = "SELECT sha256_hash FROM game_version WHERE sha256_hash = @targetSha";
command.Parameters.AddWithValue("@targetSha", gameVersion.Hash);

var result = command.ExecuteScalar();

if (result != null)  // If we get a row, this database supports our game version
{
    matchingAddressDb = dbFile;
    break;
}
```

**Logic Change:**
- **Before:** "Get the one SHA from this DB and compare"
- **After:** "Check if this DB contains a row matching our SHA"

---

### Step 4: Update C++ GameVersion Validation

**File:** `Patches/Common/GameAPI/GameVersion.cpp`

**Lines:** 68-99

**Current Code:**
```cpp
// Verify game version SHA matches
sqlite3_stmt* stmt = nullptr;
rc = sqlite3_prepare_v2(db, "SELECT sha256_hash FROM game_version WHERE id = 1", -1, &stmt, nullptr);
if (rc != SQLITE_OK) {
    OutputDebugStringA(("[GameVersion] ERROR: Failed to prepare version query: " + std::string(sqlite3_errmsg(db)) + "\n").c_str());
    sqlite3_close(db);
    db = nullptr;
    return false;
}

rc = sqlite3_step(stmt);
if (rc != SQLITE_ROW) {
    OutputDebugStringA("[GameVersion] ERROR: No game version found in database\n");
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    db = nullptr;
    return false;
}

const char* dbHash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
if (!dbHash || std::string(dbHash) != versionSha) {
    OutputDebugStringA("[GameVersion] ERROR: Version SHA mismatch!\n");
    OutputDebugStringA(("  Expected (from env): " + versionSha.substr(0, 16) + "...\n").c_str());
    if (dbHash) {
        std::string dbHashStr(dbHash);
        OutputDebugStringA(("  Found (in DB):       " + dbHashStr.substr(0, 16) + "...\n").c_str());
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    db = nullptr;
    return false;
}

OutputDebugStringA(("[GameVersion] Version SHA validated: " + versionSha.substr(0, 16) + "...\n").c_str());
sqlite3_finalize(stmt);
```

**New Code:**
```cpp
// Verify game version SHA is supported by this database
sqlite3_stmt* stmt = nullptr;
std::string sql = "SELECT sha256_hash FROM game_version WHERE sha256_hash = ?";
rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
if (rc != SQLITE_OK) {
    OutputDebugStringA(("[GameVersion] ERROR: Failed to prepare version query: " + std::string(sqlite3_errmsg(db)) + "\n").c_str());
    sqlite3_close(db);
    db = nullptr;
    return false;
}

// Bind the current game's SHA as parameter
rc = sqlite3_bind_text(stmt, 1, versionSha.c_str(), -1, SQLITE_TRANSIENT);
if (rc != SQLITE_OK) {
    OutputDebugStringA(("[GameVersion] ERROR: Failed to bind SHA parameter: " + std::string(sqlite3_errmsg(db)) + "\n").c_str());
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    db = nullptr;
    return false;
}

rc = sqlite3_step(stmt);
if (rc != SQLITE_ROW) {
    // No matching row = this database doesn't support our game version
    OutputDebugStringA("[GameVersion] ERROR: Game version SHA not found in database\n");
    OutputDebugStringA(("  Current game SHA: " + versionSha.substr(0, 16) + "...\n").c_str());
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    db = nullptr;
    return false;
}

// If we got a row, the database supports this game version
OutputDebugStringA(("[GameVersion] Version SHA validated: " + versionSha.substr(0, 16) + "...\n").c_str());
sqlite3_finalize(stmt);

**Logic Change:**
- **Before:** "Get the SHA from id=1 and compare with our game SHA"
- **After:** "Search for a row matching our game SHA - if found, database supports it"

**Key Improvement:** More robust error messages show which SHA we're looking for

---

### Step 5: Create Shared Database for KOTOR 1

**Commands to run:**

```bash
# Navigate to repo root
cd "/mnt/c/Users/laned/source/Repos/KotOR Patch Manager"

# Run migration 003 on GOG database (schema v2 → v3)
"/mnt/c/Program Files/dotnet/dotnet.exe" run --project tools/SqliteTools -- migrate --database AddressDatabases/kotor1_gog_103.db

# Validate the migration
"/mnt/c/Program Files/dotnet/dotnet.exe" run --project tools/SqliteTools -- validate --database AddressDatabases/kotor1_gog_103.db

# Now the database allows multiple game_version rows
# Insert Steam and Crack SHAs using DB Browser for SQLite or sqlite3
```

**Manual SQL to insert additional versions:**

Using DB Browser for SQLite, execute:
```sql
INSERT INTO game_version (sha256_hash, game_name, version_string, description)
VALUES ('34E6D971C034222A417995D8E1E8FDD9F8781795C9C289BD86C499A439F34C88',
        'KOTOR1', 'Steam 1.03', 'Steam version - identical to GOG after DRM removal');

INSERT INTO game_version (sha256_hash, game_name, version_string, description)
VALUES ('761F9466F456A83909036BAEBB5C43167D722387BE66E54617BA20A8C49E9886',
        'KOTOR1', 'Crack 1.03', 'HellSpawn CD crack - 99.9% identical to GOG');
```

**Verify the database has 3 versions:**
```sql
SELECT * FROM game_version ORDER BY version_string, description;
```

Expected output:
```
id | sha256_hash                                                      | game_name | version_string | description
---|------------------------------------------------------------------|-----------|----------------|-------------------------------------------
1  | 9C10E0450A6EECA417E036E3CDE7474FED1F0A92AAB018446D156944DEA91435 | KOTOR1    | GOG 1.03       | Star Wars KOTOR 1 GOG version
2  | 34E6D971C034222A417995D8E1E8FDD9F8781795C9C289BD86C499A439F34C88 | KOTOR1    | Steam 1.03     | Steam version - identical to GOG after DRM
3  | 761F9466F456A83909036BAEBB5C43167D722387BE66E54617BA20A8C49E9886 | KOTOR1    | Crack 1.03     | HellSpawn CD crack - 99.9% identical to GOG
```

**Rename and cleanup:**
```bash
# Rename the GOG database to shared
mv AddressDatabases/kotor1_gog_103.db AddressDatabases/kotor1_shared.db

# Delete redundant databases
rm AddressDatabases/kotor1_steam_103.db
rm AddressDatabases/kotor1_crack_103.db
```

---

## Testing Strategy

### Test 1: Build Verification

```bash
# Build C# projects to verify no compilation errors
"/mnt/c/Program Files/dotnet/dotnet.exe" build src/KPatchCore/KPatchCore.csproj -c Debug

# Build C++ would require Visual Studio - defer to full solution build
```

### Test 2: Database Validation

```bash
# Validate the shared database
"/mnt/c/Program Files/dotnet/dotnet.exe" run --project tools/SqliteTools -- validate --database AddressDatabases/kotor1_shared.db
```

Expected output:
```
✓ Schema version: 3
✓ Game version: KOTOR1 GOG 1.03
  SHA256: 9C10E0450A6EECA4...
✓ Game version: KOTOR1 Steam 1.03
  SHA256: 34E6D971C034222A...
✓ Game version: KOTOR1 Crack 1.03
  SHA256: 761F9466F456A839...
✓ Functions table: XX entries
✓ Global pointers table: X entries
✓ Offsets table: X entries

Database validation passed.
```

### Test 3: Patch Installation (GOG version)

**Using KPatchLauncher:**
1. Open KPatchLauncher
2. Select KOTOR 1 GOG installation path
3. Install a test patch
4. Verify patch installation succeeds
5. Check that `kotor1_shared.db` was copied to game directory as `addresses.db`
6. Verify no errors in installation log

### Test 4: Patch Installation (Steam version)

**Using KPatchLauncher:**
1. Open KPatchLauncher
2. Select KOTOR 1 Steam installation path
3. Install the same test patch
4. Verify patch installation succeeds
5. Check that `kotor1_shared.db` was copied to game directory as `addresses.db`
6. Verify no errors in installation log

### Test 5: Patch Installation (Crack version)

**Using KPatchLauncher:**
1. Open KPatchLauncher
2. Select KOTOR 1 Crack installation path
3. Install the same test patch
4. Verify patch installation succeeds
5. Check that `kotor1_shared.db` was copied to game directory as `addresses.db`
6. Verify no errors in installation log

### Test 6: Runtime Validation (Any version)

**Launch game with patches:**
1. Use KPatchLauncher to launch KOTOR 1
2. Game should start without errors
3. Check DebugView output for:
   ```
   [GameVersion] Version SHA validated: 9C10E0450A6EECA4...
   ```
4. Verify patches are applied correctly
5. Verify game functionality

---

## Success Criteria

- ✅ Migration 003 creates game_version table without singleton constraint
- ✅ MigrateSchemaCommand successfully applies migration 003
- ✅ kotor1_shared.db contains 3 game_version rows (GOG, Steam, Crack)
- ✅ PatchApplicator.cs compiles without errors
- ✅ GameVersion.cpp compiles without errors (deferred to full build)
- ✅ ValidateCommand shows all 3 versions in kotor1_shared.db
- ✅ Patch installation works for GOG version using kotor1_shared.db
- ✅ Patch installation works for Steam version using kotor1_shared.db
- ✅ Patch installation works for Crack version using kotor1_shared.db
- ✅ Game launches and runs correctly with patches applied

---

## Critical Files Summary

### Files to Create
- `tools/SqliteTools/Migrations/003_allow_multiple_game_versions.sql`

### Files to Modify
1. **tools/SqliteTools/Commands/MigrateSchemaCommand.cs** (line ~30)
   - Add migration 003 to migration sequence

2. **src/KPatchCore/Applicators/PatchApplicator.cs** (line 421-424)
   - Change query from `WHERE id = 1` to `WHERE sha256_hash = @targetSha`
   - Add parameter binding
   - Update condition check

3. **Patches/Common/GameAPI/GameVersion.cpp** (lines 68-99)
   - Change query from `WHERE id = 1` to parameterized `WHERE sha256_hash = ?`
   - Add parameter binding with `sqlite3_bind_text`
   - Update error messages

### Database Files to Modify
- `AddressDatabases/kotor1_gog_103.db` → Migrate to v3, add Steam/Crack SHAs, rename to `kotor1_shared.db`

### Files to Delete (After validation)
- `AddressDatabases/kotor1_steam_103.db` (merged into shared)
- `AddressDatabases/kotor1_crack_103.db` (merged into shared)

---

## Implementation Order

### Part A: Code Changes (Steps 1-4)
1. Create migration 003 SQL file
2. Update MigrateSchemaCommand to include migration 003
3. Update PatchApplicator.cs query logic
4. Update GameVersion.cpp query logic
5. Build and verify compilation

### Part B: Database Migration (Step 5)
6. Run migration 003 on kotor1_gog_103.db
7. Validate migration with ValidateCommand
8. Insert Steam and Crack SHAs via DB Browser
9. Verify all 3 versions present in database
10. Rename database to kotor1_shared.db
11. Delete redundant databases

### Part C: Testing
12. Build verification (C# compilation)
13. Validate kotor1_shared.db shows 3 versions
14. Test patch installation on GOG version
15. Test patch installation on Steam version (if available)
16. Test patch installation on Crack version (if available)
17. Test game launch and runtime validation

---

## Notes

**Safety Net:**
- Existing `original_bytes` validation in hook application catches any real address differences
- If Steam/GOG/Crack actually differ at a hook location, the hook will fail safely

**Backward Compatibility:**
- KOTOR 2 databases can remain at schema v2 (no multi-version need)
- Schema v2 databases continue to work with updated code (query will find the single SHA)
- Only KOTOR 1 benefits from multi-version support currently

**Future Work:**
- Apply same approach to KOTOR 2 if multiple near-identical versions emerge
- Consider auto-populating game_version table based on GameDetector's KnownVersions
- Add SqliteTools command to manage game_version entries (add/remove SHAs)
