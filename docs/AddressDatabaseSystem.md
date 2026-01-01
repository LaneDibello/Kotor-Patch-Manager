# Address Database System

## What It Is

The Address Database system stores version-specific memory addresses for KotOR game functions, global pointers, and class member offsets. This allows patch code to work across different game versions without hardcoding addresses.

## Why It Exists

Different KotOR distributions (GOG, Steam, cracked versions) have different memory layouts. The same function might be at address `0x401000` in the GOG version but `0x402500` in the Steam version. Without an address database, every patch would need separate hooks for each version.

## How It Works

### Database Files

Each game version has a SQLite database in `AddressDatabases/`:
- `kotor1_0_3.db` - KotOR 1 version 1.03
- `kotor2_gog_aspyr.db` - KotOR 2 GOG/Aspyr version

These databases contain three types of entries:

**Functions**: Class methods and standalone functions
```
class_name      | function_name    | address
----------------|------------------|---------
CServerExoApp   | MainLoop         | 0x4C3520
```

**Global Pointers**: Static/global variable addresses
```
pointer_name         | address
---------------------|----------
g_pServerExoApp      | 0x6A1E60
```

**Offsets**: Class member offsets from `this` pointer
```
class_name      | member_name      | offset
----------------|------------------|---------
CServerExoApp   | m_bGamePaused    | 0x124
```

### Runtime Usage

When patches are installed:
1. The correct `.db` file is identified by matching the game executable's SHA-256 hash
2. The database is copied to the game directory as `addresses.db`
3. The `KOTOR_VERSION_SHA` environment variable is set

When the game runs with patches:
1. Patch DLLs call `GameVersion::Initialize()` which opens `addresses.db`
2. The database SHA is verified against `KOTOR_VERSION_SHA`
3. Patches query addresses by name instead of hardcoding them:

```cpp
// Version-agnostic code
auto* mainLoop = GameVersion::GetFunctionAddress("CServerExoApp", "MainLoop");
auto* exoApp = GameVersion::GetGlobalPointer("g_pServerExoApp");
int pausedOffset = GameVersion::GetOffset("CServerExoApp", "m_bGamePaused");
```

This single code works across all game versions because the database provides the correct addresses.

## Managing Address Databases

The `SqliteTools` utility (`tools/SqliteTools/`) manages address databases:

### Import from Ghidra

Export function data from Ghidra as CSV, then import:
```bash
SqliteTools import-ghidra --csv functions.csv --database kotor1_0_3.db --mode append
```

### Schema Migration

Update database schema to latest version:
```bash
SqliteTools migrate --database kotor1_0_3.db
```

### Validate Database

Check database integrity:
```bash
SqliteTools validate --database kotor1_0_3.db
```

## Database Schema

**game_version** table:
- `sha256_hash` - Game executable hash (identifies version)

**functions** table:
- `class_name` - Class name (or "Global" for standalone functions)
- `function_name` - Function name
- `address` - Virtual memory address
- `calling_convention` - Optional (e.g., "thiscall", "cdecl")
- `param_size_bytes` - Optional stack parameter size
- `notes` - Optional documentation

**global_pointers** table:
- `pointer_name` - Global variable name
- `address` - Virtual memory address

**offsets** table:
- `class_name` - Class name
- `member_name` - Member variable name
- `offset` - Byte offset from `this` pointer

**schema_version** table:
- `version` - Schema version number for migrations

## Benefits

1. **Version Agnostic Patches**: One patch works across all versions
2. **Maintainability**: Update addresses in database, not in code
3. **Reverse Engineering Integration**: Direct import from Ghidra
4. **Validation**: Runtime verification ensures correct database
5. **Documentation**: Database serves as documentation of game structure
