# Next Steps - KotOR Patch Manager

**Last Updated:** 2025-12-24
**Current Phase:** SQLite Migration - Verification Stage

---

## What Was Completed Today (2025-12-24)

### ✅ SQLite Address Database Migration - Core Implementation

We successfully migrated the address database system from TOML to SQLite. All code changes are complete and C# projects build successfully.

**Summary of Changes:**
1. Downloaded and configured SQLite (32-bit x86) - `lib/sqlite3.dll`, `.lib`, `.h`
2. Created TOML→SQLite converter tool at `tools/TomlToSqlite/`
3. Converted all 4 game databases to SQLite format (`.db` files in `AddressDatabases/`)
4. Refactored `GameVersion.cpp/.h` to use SQLite prepared statements
5. Updated all build configurations (C++ vcxproj, patch build script)
6. Updated `PatchApplicator.cs` to deploy SQLite databases and DLL

**Benefits Achieved:**
- Eliminated 478KB toml++ header from every patch DLL
- Ready to scale to 24,000+ function addresses
- Can use DB Browser for SQLite to manage addresses
- Cleaner architecture with shared sqlite3.dll

---

## Your Immediate Next Steps

### Phase 1: Verification & Testing 🔍

**You need to verify the migration works correctly before moving forward.**

#### Step 1: Build C++ Projects (Visual Studio Required)

1. Open `KotorPatchManager.sln` in Visual Studio
2. Build `KotorPatcher` project:
   ```
   Build → KotorPatcher (Debug, Win32)
   Build → KotorPatcher (Release, Win32)
   ```
3. Check that `bin/Debug/` and `bin/Release/` contain:
   - `KotorPatcher.dll`
   - `sqlite3.dll` (should be copied automatically)

**Expected Result:** Both Debug and Release builds succeed with no errors.

#### Step 2: Rebuild Existing Patch DLLs

Navigate to each patch directory and rebuild with new GameVersion code:

```cmd
cd Patches\ScriptExtender
..\create-patch.bat

cd ..\EnableScriptAurPostString
..\create-patch.bat

cd ..\AdditionalConsoleCommands
..\create-patch.bat

cd ..\LevelUpLimit
..\create-patch.bat
```

**Expected Result:** All patches build successfully and create `.kpatch` files.

**What to Check:**
- No compilation errors about missing sqlite3.h
- DLLs link successfully against sqlite3.lib
- Build log shows sqlite3.lib in linker command

#### Step 3: Test Patch Installation

1. Launch `KPatchLauncher.exe`
2. Select your KOTOR game path
3. Select one or more patches to install
4. Click "Install Patches"

**What to Verify:**
- Installation completes without errors
- Game directory now contains:
  - `addresses.db` (not `addresses.toml`)
  - `sqlite3.dll`
  - `KotorPatcher.dll`
  - `patch_config.toml`
  - `patches/` directory with patch DLLs

**Check Installation Messages:**
- Should see: "Copied: kotor1_gog_103.db -> addresses.db"
- Should see: "✓ Copied sqlite3.dll to game directory"

#### Step 4: Test Game Launch

1. Launch the game through KPatchLauncher
2. Check for errors during injection
3. Watch for debug output (if using DebugView or similar)

**What to Look For in Debug Output:**
```
[GameVersion] Opening SQLite database: addresses.db
[GameVersion] Version SHA validated: 9C10E0450A6EE...
[GameVersion] Initialized successfully with SQLite database
[PatchName] Function address found: 0xXXXXXXXX
```

**Red Flags:**
```
[GameVersion] ERROR: Failed to open addresses.db
[GameVersion] ERROR: No game version found in database
[GameVersion] ERROR: Version SHA mismatch
Function address not found: ClassName::FunctionName
```

#### Step 5: Test Patch Functionality

Once the game launches:
- Test the actual patch features (e.g., script commands, level cap, etc.)
- Verify patches are actually working, not just loading

**If Everything Works:** ✅ Migration successful! Move to Phase 2.

**If There Are Issues:** 🔧 Debug and iterate. Common issues:
- Wrong architecture SQLite DLL (need x86, not x64)
- Missing sqlite3.dll in game directory
- Database version SHA doesn't match game executable
- Linker errors in patch DLLs

---

### Phase 2: Code Review & Iteration 📝

**After basic functionality is verified, review the code changes.**

#### Files to Review:

**C++ Changes:**
- `Patches/Common/GameAPI/GameVersion.h` - New SQLite-based API
- `Patches/Common/GameAPI/GameVersion.cpp` - Implementation
- `src/KotorPatcher/KotorPatcher.vcxproj` - Build config
- `Patches/create-patch.bat` - Build script updates

**C# Changes:**
- `src/KPatchCore/Applicators/PatchApplicator.cs` - Deployment logic
- `src/KPatchCore/KPatchCore.csproj` - Added Microsoft.Data.Sqlite

**Build System:**
- `.gitignore` - Allows lib/ files to be checked in
- `lib/` - Contains sqlite3.dll, sqlite3.lib, sqlite3.h

**Tools:**
- `tools/TomlToSqlite/` - Converter tool (may need again for new versions)

**Databases:**
- `AddressDatabases/*.db` - New SQLite databases
- `AddressDatabases/*.toml` - Old TOML files (can delete after verification)

#### Review Questions:

1. Does the SQLite approach feel cleaner than TOML?
2. Are there edge cases not handled?
3. Should we add error handling anywhere?
4. Documentation clear enough?

**If You Find Issues:** Create GitHub issues or make notes for iteration.

**If Everything Looks Good:** ✅ Proceed to Phase 3.

---

### Phase 3: Ghidra Database Population 🗄️

**Goal:** Import 24,000+ labeled function addresses from Ghidra into SQLite database.

#### Step 1: Export from Ghidra

In Ghidra with KOTOR 1 GOG 1.03 loaded:

1. Open Script Manager
2. Create or use existing script to export functions
3. Export to CSV format with columns:
   ```
   class_name,function_name,address
   ```

**Example CSV:**
```csv
CExoString,Constructor,0x004a2c10
CVirtualMachine,RunScript,0x005a3b80
CSWSCreatureStats,SetCONBase,0x005ed700
```

#### Step 2: Import into SQLite

**Option A: Using DB Browser for SQLite (GUI)**
1. Download from https://sqlitebrowser.org/
2. Open `AddressDatabases/kotor1_gog_103.db`
3. File → Import → Table from CSV file
4. Select your CSV
5. Import into `functions` table
6. Verify data looks correct

**Option B: Using SQL Script**
```sql
-- Create temporary table for import
CREATE TEMP TABLE temp_import (
    class_name TEXT,
    function_name TEXT,
    address TEXT
);

-- Import CSV (command-line sqlite3)
.mode csv
.import ghidra_export.csv temp_import

-- Insert into functions table
INSERT INTO functions (class_name, function_name, address, notes)
SELECT
    class_name,
    function_name,
    CAST(address AS INTEGER),
    'Imported from Ghidra export ' || date('now')
FROM temp_import
WHERE NOT EXISTS (
    SELECT 1 FROM functions f
    WHERE f.class_name = temp_import.class_name
      AND f.function_name = temp_import.function_name
);

-- Clean up
DROP TABLE temp_import;
```

#### Step 3: Verify Import

```sql
-- Check count
SELECT COUNT(*) FROM functions;
-- Should see ~24,000+

-- Spot check some addresses
SELECT class_name, function_name, printf('0x%08X', address) as hex_addr
FROM functions
LIMIT 10;

-- Check for duplicates
SELECT class_name, function_name, COUNT(*) as count
FROM functions
GROUP BY class_name, function_name
HAVING count > 1;
```

#### Step 4: Rebuild and Test

1. Run converter to regenerate `.db` from updated TOML (if still using TOML as source)
   OR
2. Directly edited `.db`, so just test it
3. Reinstall patches with new database
4. Test that new function addresses are accessible

**Success Criteria:**
- 24,000+ functions in database
- No duplicate entries
- Existing patches still work
- Can reference new functions in patch development

---

## Phase 4: Future Work (Not Immediate)

### After Verification Complete:

1. **Documentation Cleanup**
   - Update patch development guide
   - Remove TOML references from developer docs
   - Document SQLite workflow

2. **Consider Removing TOML Files**
   - After verification, can delete `AddressDatabases/*.toml`
   - Keep `.db` files as primary source
   - Update `.csproj` to only copy `.db` files

3. **Patch Options Feature**
   - See `docs/PATCH_OPTIONS_DESIGN.md`
   - Implement after SQLite is stable
   - Allows configurable patch parameters (e.g., custom max level)

4. **Additional Game Versions**
   - KOTOR 1 Steam 1.03
   - KOTOR 1 Cracked 1.03
   - KOTOR 2 GOG Aspyr
   - Populate their databases similarly

---

## Quick Reference: Key Locations

**SQLite Migration Files:**
- Converter Tool: `tools/TomlToSqlite/`
- Databases: `AddressDatabases/*.db`
- SQLite Library: `lib/sqlite3.*`
- GameVersion Code: `Patches/Common/GameAPI/GameVersion.cpp`
- PatchApplicator: `src/KPatchCore/Applicators/PatchApplicator.cs`

**Documentation:**
- Migration Plan: `docs/SQLITE_ADDRESS_DATABASE_MIGRATION.md`
- This File: `docs/NEXT_STEPS.md`
- Patch Options Design: `docs/PATCH_OPTIONS_DESIGN.md`

**Build Commands:**
```cmd
# C# Projects (from repo root)
dotnet build KotorPatchManager.sln -c Debug
dotnet build KotorPatchManager.sln -c Release

# C++ Projects (requires Visual Studio)
# Open KotorPatchManager.sln in Visual Studio
# Build → Build Solution

# Rebuild Patch
cd Patches\[PatchName]
..\create-patch.bat

# Run Converter
dotnet run --project tools/TomlToSqlite -- input.toml output.db
```

---

## Contact / Notes

- SQLite migration code changes reviewed: **Pending**
- Basic functionality tested: **Pending**
- Ghidra import completed: **Pending**
- Ready for patch options feature: **No** (after verification)

**When you return:**
1. Start with Phase 1 verification
2. Build C++ projects first
3. Test thoroughly before moving to Ghidra import
4. Document any issues you encounter
5. Update this file with findings

**Need to revisit the code?**
- All changes from 2025-12-24 session
- Check git log for commits on this date
- Review files listed in "Key Locations" section above

Good luck! 🚀
