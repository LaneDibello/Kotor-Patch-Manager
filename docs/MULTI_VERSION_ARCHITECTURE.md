# Multi-Version Game Function Architecture

**Status**: Implementation Phase
**Version**: 2.0
**Last Updated**: 2025-11-07

## Table of Contents
1. [Overview](#overview)
2. [Problem Statement](#problem-statement)
3. [Architecture Design](#architecture-design)
4. [Component Specifications](#component-specifications)
5. [Implementation Plan](#implementation-plan)
6. [Migration Guide](#migration-guide)
7. [Examples](#examples)

---

## Overview

This document describes the architecture for supporting multiple game versions (KotOR 1 GoG, KotOR 1 Steam, KotOR 2 variants) within a single patch system. The design addresses three key challenges:

1. **Organization**: Refactor flat function declarations into class-based abstractions
2. **Version Support**: Enable runtime selection of version-specific addresses
3. **Maintainability**: Centralize address databases and eliminate magic numbers

### Key Design Decisions

- **Hybrid Version Strategy**: Single compiled DLL per patch that loads version-specific addresses at runtime
- **Data-Driven Addresses**: Function addresses and offsets stored in TOML files per game version
- **Low-Level API**: Wrapper classes mirror game API 1:1 for maximum control
- **Deployment Architecture**: C# installer copies address database to game directory; KotorPatcher passes SHA via environment variable
- **Separation of Concerns**: Patches directory is development-only; deployed patches use addresses.toml in game directory

---

## Problem Statement

### Current Issues

**1. Flat Organization**
All functions declared in `Kotor1Functions.h` (154 lines) with no clear boundaries between game classes.

```cpp
// Current approach - flat and verbose
extern CSWSCreatureStatsHasFeat creatureStatsHasFeat;
extern CVirtualMachineStackPopInteger vmStackPopInteger;
// ... 35+ more declarations
```

**2. Magic Numbers Everywhere**
Property offsets scattered throughout patches as undocumented constants:

```cpp
// What is 0xa74? Where is this documented?
void* creatureStats = getObjectProperty<void*>(creature, 0xa74);
```

**3. Version-Specific Hardcoding**
All addresses valid only for KotOR 1 GoG. Supporting other versions requires:
- Duplicating entire patches
- Manual address updates for each version
- No shared code between versions

**4. Maintenance Burden**
Adding a new function requires changes in 4+ places:
- Define typedef
- Declare const address
- Extern declare function pointer
- Initialize in .cpp file

### Requirements

✅ **REQ-1**: Class-based organization mirroring game structure
✅ **REQ-2**: Version-aware address resolution at runtime
✅ **REQ-3**: Single DLL works across multiple game versions
✅ **REQ-4**: Documented, centralized address databases
✅ **REQ-5**: Backwards compatibility during migration
✅ **REQ-6**: Type-safe property access (no raw offsets in patch code)
✅ **REQ-7**: Support multiple hooks.toml files per patch (version-specific hooks)

---

## Architecture Design

### System Architecture Diagram

```
┌────────────────────────────────────────────────────────────────────┐
│                    Patch Installation (C#)                         │
│  ┌──────────────┐   ┌──────────────┐   ┌─────────────────────┐    │
│  │   Manifest   │   │ Game Version │   │ PatchApplicator     │    │
│  │   Parser     │──>│  Detector    │──>│                     │    │
│  └──────────────┘   └──────────────┘   │ - Validates SHA     │    │
│         │                    │          │ - Selects *.hooks   │    │
│         │                    │          │ - Copies addresses  │    │
│         │                    │          └─────────────────────┘    │
│         │                    │                     │                │
│         │                    ▼                     ▼                │
│         │          SHA: 9C10E0450A...    ┌─────────────────┐       │
│         │                                 │ patch_config.toml│       │
│         ├─ Finds ALL matching *.hooks    │ └─────────────────┘      │
│         │  files for this SHA                    │                 │
│         │                                        │                 │
│         └─────────────────────────────────┐      │                 │
│                                           │      │                 │
│  ┌────────────────────────────────────┐   │      │                 │
│  │ AddressDatabases/ (repo root)      │   │      │                 │
│  │  - kotor1_gog_103.toml            │───┼──────┼─> Copies        │
│  │  - kotor1_steam_103.toml          │   │      │   matching      │
│  │  (versions_sha field matches)     │   │      │   file to:      │
│  └────────────────────────────────────┘   │      │                 │
│                                           │      ▼                 │
│                                           │  <GameDir>/            │
│                                           │    addresses.toml      │
└───────────────────────────────────────────┼────────────────────────┘
                                           │
                          Launches KPatchLauncher.exe
                                           │
                                           ▼
┌────────────────────────────────────────────────────────────────────┐
│                    Runtime Execution (C++)                         │
│  ┌──────────────┐                                                  │
│  │ KotorPatcher │ Reads patch_config.toml                         │
│  │    DLL       │────> target_version_sha = "9C10E0450A..."       │
│  │              │                                                  │
│  │              │ Sets environment variable:                       │
│  │              │ KOTOR_VERSION_SHA = "9C10E0450A..."             │
│  └──────┬───────┘                                                  │
│         │ LoadLibrary(patch DLLs)                                 │
│         ▼                                                          │
│  ┌──────────────────────────────────────────┐                     │
│  │ Patch DLL (e.g., EnableScriptAurPostString.dll) │            │
│  │                                          │                     │
│  │  DllMain(DLL_PROCESS_ATTACH):            │                     │
│  │    GameVersion::Initialize()             │                     │
│  │      ├─ Reads env var KOTOR_VERSION_SHA  │                     │
│  │      ├─ Loads addresses.toml from CWD    │                     │
│  │      └─ Validates SHA matches            │                     │
│  │                                          │                     │
│  │  Hook functions:                          │                     │
│  │    GameVersion::GetFunctionAddress()     │                     │
│  │    GameVersion::GetOffset()               │                     │
│  └──────────────────────────────────────────┘                     │
│                                                                    │
│  ┌─────────────────────────────────────────────────────────┐     │
│  │          GameAPI Wrapper Classes (in patch DLL)         │     │
│  │  ┌──────────────┐  ┌──────────────┐  ┌───────────┐    │     │
│  │  │CVirtualMachine│  │CServerExoApp │  │CCreature  │    │     │
│  │  │              │  │              │  │  Stats    │    │     │
│  │  │StackPop*()  │  │GetObjectArr()│  │HasFeat()  │    │     │
│  │  │StackPush*() │  │GetCreature() │  │AddFeat()  │    │     │
│  │  └──────────────┘  └──────────────┘  └───────────┘    │     │
│  └─────────────────────────────────────────────────────────┘     │
└────────────────────────────────────────────────────────────────────┘
```

### Data Flow

**Installation Time (C#)**:
1. User runs `kpatch install <patch> <game>`
2. C# detects game version via `GameDetector.DetectVersion(exePath)` → returns `GameVersion` object with SHA-256 hash
3. C# reads `manifest.toml` → validates `GameVersion.Hash` is in `[patch.supported_versions]` values
4. C# finds ALL `*.hooks.toml` files where `[metadata].target_versions` is empty OR contains this SHA
5. C# merges all matching hooks files into patch configuration
6. C# finds matching address database in `AddressDatabases/` directory (by `versions_sha` field)
7. C# copies matched address database → `<GameDir>/addresses.toml` (generic name, no version in filename)
8. C# writes `patch_config.toml` with `target_version_sha = "9C10E0450A..."`
9. User later launches game via `KPatchLauncher.exe`

**Runtime (C++)**:
1. `KotorPatcher.dll` loads and reads `patch_config.toml` from its directory
2. `KotorPatcher` extracts `target_version_sha` field
3. `KotorPatcher` sets environment variable: `SetEnvironmentVariable("KOTOR_VERSION_SHA", sha)`
4. `KotorPatcher` calls `LoadLibrary()` on each patch DLL
5. Patch DLL's `DllMain(DLL_PROCESS_ATTACH)` calls `GameVersion::Initialize()`:
   - Reads SHA from `GetEnvironmentVariable("KOTOR_VERSION_SHA")`
   - Loads `addresses.toml` from current working directory
   - Validates `versions_sha` field matches environment variable SHA
   - Caches all function addresses and offsets in static maps
6. Patch hook functions call `GameVersion::GetFunctionAddress()` / `GetOffset()`
7. Wrapper classes use GameVersion to access version-specific addresses

---

## Component Specifications

### 1. Version Configuration Files

#### 1.1 Manifest Enhancement

**File**: `Patches/<PatchName>/manifest.toml`

```toml
[patch]
id = "additional-console-commands"
name = "Additional Console Commands"
version = "1.0.0"
author = "Lane"
description = "Adds additional commands to the in-game cheat console"

requires = []
conflicts = []

[patch.supported_versions]
kotor1_gog_103 = "9C10E0450A6EECA417E036E3CDE7474FED1F0A92AAB018446D156944DEA91435"
kotor1_steam_103 = "34E6D971C034222A417995D8E1E8FDD9F8781795C9C289BD86C499A439F34C88"
# Key = friendly name (for humans), Value = SHA-256 hash (for matching)
```

**Schema**:
- `[patch.supported_versions]` (TOML table, required): Maps friendly version names to SHA-256 hashes
  - Key: Human-readable version identifier (e.g., "kotor1_gog_103", "kotor2_legacy_10b")
  - Value: SHA-256 hash of the game executable (definitive identifier)
  - Must have at least one entry
  - **This format already exists in current codebase**

#### 1.2 Hooks Configuration Enhancement

**File**: `Patches/<PatchName>/hooks.toml` (can have multiple)

**Naming Convention**:
No hard and fast naming convention other than the file name must end with `.hooks.toml`. Suggested patterns:
- `default.hooks.toml` - Version-agnostic (applies to all supported_versions)
- `kotor1_gog_103.hooks.toml` - Version-specific (single version)
- `console_commands.hooks.toml` - Organizational (logical grouping of related hooks)
- `steam_only.hooks.toml` - Multi-version (applies to multiple Steam versions)

**IMPORTANT**: Patches can have multiple hooks files for the SAME version. All matching files are loaded and merged.

```toml
[metadata]
target_versions = ["9C10E0450A6EECA417E036E3CDE7474FED1F0A92AAB018446D156944DEA91435"]  # NEW: Versions this hooks file targets
                               # Empty array = all supported_versions

[[hooks]]
address = 0x0044d490
type = "DETOUR"
# ... existing hook config
```

**Schema**:
- `[metadata]` section (required)
  - `target_versions` (array of strings): Versions this file applies to
    - Empty array: applies to ALL versions in `supported_versions`
    - Non-empty: applies only to listed versions
- Hook addresses can differ between files for different versions

**Selection Logic** (C#):
```csharp
// Find ALL applicable hooks files (not just first match!)
var hooksFiles = Directory.GetFiles(patchDir, "*.hooks.toml");
var gameVersionSha = detectedGameVersion.Hash; // SHA-256 from GameVersion object
var selectedFiles = new List<string>();

foreach (var file in hooksFiles) {
    var metadata = HooksParser.ParseMetadata(file);

    // Include if: no target specified OR our SHA is in the target list
    if (metadata.TargetVersions.Count == 0 ||
        metadata.TargetVersions.Contains(gameVersionSha)) {
        selectedFiles.Add(file);
    }
}

// Parse and merge all selected hooks files
var allHooks = new List<Hook>();
foreach (var file in selectedFiles) {
    allHooks.AddRange(HooksParser.Parse(file));
}
```

#### 1.3 Patch Config Enhancement

**File**: Generated at `<InstallDir>/patches/<PatchName>/patch.toml`

```toml
[patch]
name = "Additional Console Commands"
version = "1.0.0"
target_version_sha = "9C10E0450A6EECA417E036E3CDE7474FED1F0A92AAB018446D156944DEA91435"  # NEW
dll_path = "AdditionalConsoleCommands.dll"
# ... existing fields
```

**Schema**:
- `target_version_sha` (string, required): SHA-256 hash of the game version this patch installation targets
  - Written by C# installer from GameVersion.Hash property
  - Read by C++ runtime to find matching address database TOML (via `versions_sha` field)
  - This is the PRIMARY version identifier throughout the system

---

### 2. Function Address Database

#### 2.1 TOML Structure

**Location**: `AddressDatabases/<game>_<version>.toml` (repo root, **separate from Patches/**)

**Deployment**: C# installer copies matching TOML to `<GameDir>/addresses.toml` (generic name)

**Files**:
Exact naming isn't important - the `versions_sha` field in the file is the definitive identifier.
- `kotor1_gog_103.toml` - KotOR 1 GoG version 1.0.3
- `kotor1_steam_103.toml` - KotOR 1 Steam version 1.0.3
- `kotor2_gog_aspyr.toml` - KotOR 2 GoG Aspyr version
- `kotor2_steam_aspyr.toml` - KotOR 2 Steam Aspyr version
- `kotor2_legacy_10.toml` - KotOR 2 Legacy 1.0
- `kotor2_legacy_10b.toml` - KotOR 2 Legacy 1.0b

**Why separate directory?**
- Address databases are NOT part of patches - they're shared infrastructure
- Multiple patches use the same address database
- Makes it clear these are deployment artifacts, not development code

**Schema**:
```toml
# Game Version
versions_sha = "9C10E0450A6EECA417E036E3CDE7474FED1F0A92AAB018446D156944DEA91435" # (k1 gog)

# Global memory pointers
[global_pointers]
VIRTUAL_MACHINE_PTR = 0x007a3a00
APP_MANAGER_PTR = 0x007a39fc
OBJECT_DEFAULT = 0x7f000000

# Function addresses organized by class
[functions.CServerExoApp]
GetObjectArray = 0x004aed70
GetCreatureByGameObjectID = 0x004ae770
GetPlayerCreatureId = 0x004aea40

[functions.CGameObjectArray]
GetGameObject = 0x004d8230

[functions.CSWSCreatureStats]
HasFeat = 0x005a6630
HasSpell = 0x005a6e70
AddFeat = 0x005aa810
AddKnownSpell = 0x005aa9b0
RemoveFeat = 0x005aa940
SetSTRBase = 0x005a9fe0
SetDEXBase = 0x005aa020
SetCONBase = 0x005aa060
SetINTBase = 0x005aa0f0
SetWISBase = 0x005aa130
SetCHABase = 0x005aa170
GetSkillRank = 0x005aa570
SetSkillRank = 0x005a54c0
SetMovementRate = 0x005a5680
GetClass = 0x005a4e90

[functions.CVirtualMachine]
StackPopInteger = 0x005d1000
StackPopFloat = 0x005d1040
StackPopVector = 0x005d1080
StackPopString = 0x005d11e0
StackPopEngineStructure = 0x005d1220
StackPopObject = 0x005d1260
StackPopCommand = 0x005d12a0
StackPushInteger = 0x005d10d0
StackPushFloat = 0x005d1110
StackPushVector = 0x005d1150
StackPushString = 0x005d1190
StackPushEngineStructure = 0x005d11d0
StackPushObject = 0x005d1200
RunScript = 0x005d0fc0

[functions.CSWSObject]
AddActionToFront = 0x004ccb00

[functions.CSWSCreature]
GetClientCreature = 0x004efb20

[functions.CSWItem]
GetBaseItem = 0x005b4790

[functions.CSWInventory]
GetItemInSlot = 0x005a4c20

[functions.CExoString]
Constructor_Empty = 0x005b3190
Constructor_WithLength = 0x005e5b70
Constructor_FromCStr = 0x005e5a90
Destructor = 0x005e5c20

[functions.ConsoleFunc]
Constructor_1 = 0x0044c620
Constructor_2 = 0x0044c5c0
Constructor_3 = 0x0044c560
Constructor_4 = 0x0044bcf0

[functions.Other]
AurPostString = 0x0044d490

# Object memory offsets
[offsets.CAppManager]
Client = 0x0
Server = 0x4

[offsets.ServerCreature]
CreatureStats = 0xa74
Inventory = 0xa2c
Position = 0x90
Orientation = 0x9c
AreaId = 0x8c

[offsets.CreatureStats]
ClassCount = 0x89
STRBase = 0xe9
DEXBase = 0xeb
CONBase = 0xed
INTBase = 0xef
WISBase = 0xf1
CHABase = 0xf3

[offsets.ClientCreature]
Running = 0x3e0
Stealth = 0x194

[offsets.CombatRound]
PlayerCreature = 0x9b4
OnHandAttacks = 0x990

[offsets.BaseItem]
WeaponWield = 0x8
```

**Notes**:
- Addresses are in hexadecimal with `0x` prefix
- Section names use dot notation: `[functions.ClassName]`
- Consistent naming: CamelCase for functions, PascalCase for offsets
- Comments document purpose where non-obvious

#### 2.2 TOML Parsing

**Dependencies**: Use existing TOML parser (Tomlyn) in KotorPatcher

**Implementation**: See GameVersion Manager (Section 3.1)

---

### 3. GameAPI Wrapper Classes

#### 3.1 GameVersion Manager

**Purpose**: Centralized address resolution for patch DLLs

**Files**:
- `Patches/Common/GameAPI/GameVersion.h`
- `Patches/Common/GameAPI/GameVersion.cpp`
- `Patches/Common/toml.hpp` (copied dependency, not relative import)

**Interface**:
```cpp
#pragma once
#include <string>
#include <unordered_map>

class GameVersion {
public:
    // Initialize from environment variable and addresses.toml in CWD
    // NO PARAMETERS - reads KOTOR_VERSION_SHA env var set by KotorPatcher
    static bool Initialize();

    // Check if initialized
    static bool IsInitialized();

    // Get current game version SHA
    static std::string GetVersionSha();

    // Function address lookup
    static void* GetFunctionAddress(const std::string& className,
                                    const std::string& functionName);

    // Global pointer lookup
    static void* GetGlobalPointer(const std::string& pointerName);

    // Offset lookup
    static int GetOffset(const std::string& className,
                        const std::string& propertyName);

    // Check if function/offset available for this version
    static bool HasFunction(const std::string& className,
                           const std::string& functionName);
    static bool HasOffset(const std::string& className,
                         const std::string& propertyName);

    // Cleanup
    static void Reset();

private:
    static bool s_initialized;
    static std::string s_versionSha;
    static std::unordered_map<std::string, void*> s_functionAddresses;
    static std::unordered_map<std::string, void*> s_globalPointers;
    static std::unordered_map<std::string, int> s_offsets;

    static bool LoadAddressDatabase();  // Loads from addresses.toml in CWD
    static std::string MakeKey(const std::string& className, const std::string& memberName);
};
```

**Usage Pattern**:
```cpp
// In patch DLL's DllMain
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            // Simple - no parameters needed!
            if (!GameVersion::Initialize()) {
                OutputDebugStringA("[MyPatch] GameVersion initialization failed\n");
                return FALSE;
            }

            // Now query addresses
            g_myFunction = (MyFuncPtr)GameVersion::GetFunctionAddress("MyClass", "MyFunc");
            break;

        case DLL_PROCESS_DETACH:
            GameVersion::Reset();
            break;
    }
    return TRUE;
}
```

**How it works**:
1. KotorPatcher sets `KOTOR_VERSION_SHA` environment variable before loading patch DLLs
2. Patch DLL calls `GameVersion::Initialize()` with no arguments
3. `Initialize()` reads SHA from environment variable
4. `Initialize()` loads `addresses.toml` from current working directory (game directory)
5. Validates `versions_sha` field in TOML matches environment variable
6. Caches all addresses in static maps

**Error Handling**:
- `Initialize()` returns false if env var missing, file not found, or SHA mismatch
- `GetFunctionAddress()` returns `nullptr` if not found or not initialized
- Wrapper classes check for `nullptr` before calling
- Debug output for troubleshooting

#### 3.2 Wrapper Class Structure

**Directory**: `Patches/Common/GameAPI/`

**Files** (one pair per game class):
- `CServerExoApp.h` / `CServerExoApp.cpp`
- `CGameObjectArray.h` / `CGameObjectArray.cpp`
- `CVirtualMachine.h` / `CVirtualMachine.cpp`
- `CCreatureStats.h` / `CCreatureStats.cpp`
- `CServerCreature.h` / `CServerCreature.cpp`
- `CClientCreature.h` / `CClientCreature.cpp`
- `CServerObject.h` / `CServerObject.cpp`
- `CInventory.h` / `CInventory.cpp`
- `CItem.h` / `CItem.cpp`
- `CExoString.h` / `CExoString.cpp`

**Class Template**:
```cpp
// Example: CVirtualMachine.h
#pragma once
#include "GameVersion.h"
#include "../Common.h"  // For CExoString, Vector, etc.

class CVirtualMachine {
public:
    // Constructor takes raw game object pointer
    explicit CVirtualMachine(void* vmPtr);

    // Static instance getter (for singleton VM)
    static CVirtualMachine* GetInstance();

    // Low-level API mirroring game functions
    void StackPopInteger(int* output);
    void StackPopFloat(float* output);
    void StackPopVector(Vector* output);
    void StackPopString(CExoString* output);
    void StackPopEngineStructure(int structType, void** output);
    void StackPopObject(DWORD* output);
    void StackPopCommand(void** output);

    void StackPushInteger(int value);
    void StackPushFloat(float value);
    void StackPushVector(Vector value);
    void StackPushString(CExoString* str);
    void StackPushEngineStructure(int structType, void* ptr);
    void StackPushObject(DWORD objectId);

    void RunScript(CExoString* scriptName, DWORD objectId, ...);

    // Check if function available for this version
    bool CanStackPopInteger() const;

private:
    void* m_vmPtr;  // Raw game object pointer

    // Function pointers loaded from GameVersion
    typedef void (__thiscall *StackPopIntFn)(void*, int*);
    typedef void (__thiscall *StackPopFloatFn)(void*, float*);
    // ... more typedefs

    StackPopIntFn m_stackPopInt;
    StackPopFloatFn m_stackPopFloat;
    // ... more function pointers

    // Initialize function pointers from GameVersion
    void InitializeFunctions();
};
```

**Implementation Pattern**:
```cpp
// CVirtualMachine.cpp
#include "CVirtualMachine.h"

CVirtualMachine::CVirtualMachine(void* vmPtr)
    : m_vmPtr(vmPtr), m_stackPopInt(nullptr), /* ... */
{
    InitializeFunctions();
}

void CVirtualMachine::InitializeFunctions() {
    m_stackPopInt = (StackPopIntFn)GameVersion::GetFunctionAddress(
        "CVirtualMachine", "StackPopInteger"
    );
    m_stackPopFloat = (StackPopFloatFn)GameVersion::GetFunctionAddress(
        "CVirtualMachine", "StackPopFloat"
    );
    // ... load all function pointers
}

void CVirtualMachine::StackPopInteger(int* output) {
    if (m_stackPopInt) {
        m_stackPopInt(m_vmPtr, output);
    } else {
        // Handle unavailable function
        *output = 0;
        // Optional: Log warning
    }
}

CVirtualMachine* CVirtualMachine::GetInstance() {
    void* vmPtr = GameVersion::GetGlobalPointer("VIRTUAL_MACHINE_PTR");
    if (vmPtr) {
        void* vm = *(void**)vmPtr;  // Dereference pointer
        return new CVirtualMachine(vm);
    }
    return nullptr;
}

bool CVirtualMachine::CanStackPopInteger() const {
    return m_stackPopInt != nullptr;
}
```

#### 3.3 Property Access Wrappers

**Purpose**: Hide offset-based property access behind getters/setters

**Example**: `CServerCreature.h`
```cpp
class CServerCreature {
public:
    explicit CServerCreature(void* creaturePtr);

    // Property access (no magic offsets)
    CCreatureStats* GetCreatureStats();
    CInventory* GetInventory();

    Vector GetPosition();
    void SetPosition(const Vector& pos);

    Vector GetOrientation();
    void SetOrientation(const Vector& orient);

    DWORD GetAreaId();

    // Method wrappers
    CClientCreature* GetClientCreature();
    void AddActionToFront(void* action);

private:
    void* m_creaturePtr;

    // Cached offsets (loaded once)
    static int s_offsetCreatureStats;
    static int s_offsetInventory;
    static int s_offsetPosition;
    static int s_offsetOrientation;
    static int s_offsetAreaId;

    static void InitializeOffsets();
};
```

**Implementation**:
```cpp
// CServerCreature.cpp
int CServerCreature::s_offsetCreatureStats = -1;
int CServerCreature::s_offsetInventory = -1;
// ... more static offsets

CServerCreature::CServerCreature(void* creaturePtr)
    : m_creaturePtr(creaturePtr)
{
    if (s_offsetCreatureStats == -1) {
        InitializeOffsets();
    }
}

void CServerCreature::InitializeOffsets() {
    s_offsetCreatureStats = GameVersion::GetOffset(
        "ServerCreature", "CreatureStats"
    );
    s_offsetInventory = GameVersion::GetOffset(
        "ServerCreature", "Inventory"
    );
    // ... load all offsets
}

CCreatureStats* CServerCreature::GetCreatureStats() {
    if (s_offsetCreatureStats >= 0) {
        void* ptr = getObjectProperty<void*>(m_creaturePtr, s_offsetCreatureStats);
        return new CCreatureStats(ptr);
    }
    return nullptr;
}

Vector CServerCreature::GetPosition() {
    if (s_offsetPosition >= 0) {
        return getObjectProperty<Vector>(m_creaturePtr, s_offsetPosition);
    }
    return Vector{0, 0, 0};
}

void CServerCreature::SetPosition(const Vector& pos) {
    if (s_offsetPosition >= 0) {
        setObjectProperty<Vector>(m_creaturePtr, s_offsetPosition, pos);
    }
}
```

---

### 4. C# Integration

#### 4.1 PatchConfig Enhancement

**File**: `KPatchCore/Models/PatchConfig.cs`

```csharp
public class PatchConfig {
    public string Name { get; set; }
    public string Version { get; set; }
    public string TargetVersionSha { get; set; }  // NEW - SHA-256 hash
    public string DllPath { get; set; }
    // ... existing properties
}
```

**Serialization**: Already handled by existing TOML serialization

#### 4.2 Game Version Detection

**File**: `KPatchCore/Detectors/GameDetector.cs` (**already exists and fully implemented**)

**Current Implementation**:
```csharp
public static class GameDetector {
    // Already implemented - returns PatchResult<GameVersion>
    public static PatchResult<GameVersion> DetectVersion(string exePath);

    // GameVersion class has these properties:
    // - Platform (Windows/macOS/Linux)
    // - Distribution (GOG/Steam/Physical/Other)
    // - Version (string like "1.0.3" or "2 1.0.2 (Aspyr)")
    // - Architecture (x86/x86_64/ARM64)
    // - FileSize (long)
    // - Hash (string - SHA-256, THIS IS THE PRIMARY IDENTIFIER)
}
```

**No changes needed** - simply use `gameVersion.Hash` as the version identifier.

#### 4.3 Installer Enhancement

**File**: `KPatchCore/Services/PatchApplicator.cs`

```csharp
public class PatchApplicator {
    public PatchResult ApplyPatch(string patchPath, string gameExePath) {
        // Existing: Parse manifest, validate, etc.

        // Detect version using existing GameDetector
        var versionResult = GameDetector.DetectVersion(gameExePath);
        if (!versionResult.Success) {
            return PatchResult.Fail("Could not detect game version");
        }

        GameVersion gameVersion = versionResult.Value;
        string targetVersionSha = gameVersion.Hash;  // SHA-256 is the identifier

        // Validate version supported (check SHA against manifest's supported_versions VALUES)
        var manifest = /* ... parsed manifest ... */;
        if (!manifest.SupportedVersions.Values.Contains(targetVersionSha)) {
            return PatchResult.Fail(
                $"Patch does not support game version {gameVersion.DisplayName} " +
                $"(SHA: {targetVersionSha.Substring(0, 16)}...)"
            );
        }

        // Load ALL matching hooks files (not just one!)
        var allHooksFiles = SelectAllHooksFiles(patchPath, targetVersionSha);
        var allHooks = new List<Hook>();

        foreach (var hooksFile in allHooksFiles) {
            var hooks = HooksParser.Parse(hooksFile);
            allHooks.AddRange(hooks);
        }

        // Write target_version_sha to patch config
        var config = new PatchConfig {
            Name = manifest.Name,
            Version = manifest.Version,
            TargetVersionSha = targetVersionSha,  // NEW property
            DllPath = /* ... */,
            // ... existing fields
        };

        WritePatchConfig(config, installPath);

        // Continue with installation using allHooks list...
    }

    // Returns ALL hooks files that match this version (not just first!)
    private List<string> SelectAllHooksFiles(string patchPath, string targetVersionSha) {
        var hooksFiles = Directory.GetFiles(patchPath, "*.hooks.toml");
        var selectedFiles = new List<string>();

        foreach (var file in hooksFiles) {
            var metadata = HooksParser.ParseMetadata(file);

            // Include if: no target specified OR our SHA is in target list
            if (metadata.TargetVersions.Count == 0 ||
                metadata.TargetVersions.Contains(targetVersionSha)) {
                selectedFiles.Add(file);
            }
        }

        if (selectedFiles.Count == 0) {
            throw new Exception("No matching hooks files found");
        }

        return selectedFiles;
    }
}
```

#### 4.4 Hooks Parser Enhancement

**File**: `KPatchCore/Parsers/HooksParser.cs`

```csharp
public class HooksMetadata {
    public List<string> TargetVersions { get; set; } = new List<string>();
}

public class HooksParser {
    public static HooksMetadata ParseMetadata(string hooksFilePath) {
        var toml = Toml.ToModel(File.ReadAllText(hooksFilePath));

        var metadata = new HooksMetadata();

        if (toml.ContainsKey("metadata")) {
            var metadataTable = toml["metadata"] as TomlTable;
            if (metadataTable?.ContainsKey("target_versions") == true) {
                var versions = metadataTable["target_versions"] as TomlArray;
                if (versions != null) {
                    metadata.TargetVersions = versions
                        .Select(v => v.ToString())
                        .ToList();
                }
            }
        }

        return metadata;
    }

    // Existing Parse() method unchanged
}
```

---

## Implementation Plan

### Phase 1: Foundation

**Goal**: Build core infrastructure without breaking existing patches

**Tasks**:
1. ✅ Create this design document
2. Create `GameVersion` manager class (header + implementation)
3. Create first address database: `Kotor1_GoG.toml`
4. Write TOML loading logic in `GameVersion::LoadAddressDatabase()`
5. Unit test GameVersion with mock TOML

**Deliverable**: GameVersion system that can load and query addresses

---

### Phase 2: Wrapper Classes

**Goal**: Create wrapper classes for all game classes

**Tasks**:
1. Create directory structure: `Patches/Common/GameAPI/`
2. Implement `CVirtualMachine` wrapper (most complex, use as template)
3. Implement `CServerExoApp` wrapper
4. Implement `CCreatureStats` wrapper
5. Implement `CServerCreature` wrapper (property access example)
6. Implement remaining wrappers:
   - `CGameObjectArray`
   - `CClientCreature`
   - `CServerObject`
   - `CInventory`
   - `CItem`
   - `CExoString`
7. Add initialization call in KotorPatcher DllMain

**Deliverable**: Complete GameAPI library with all wrapper classes

---

### Phase 3: C# Integration

**Goal**: Update C# side to support version-aware configuration

**Tasks**:
1. Add `TargetVersion` property to `PatchConfig` model
2. Add `GetVersionString()` to `GameDetector`
3. Enhance `PatchApplicator` to detect version and select hooks file
4. Enhance `HooksParser` to parse `[metadata]` section
5. Update serialization to write `target_version` in patch.toml
6. Add validation for `supported_versions` in manifest

**Deliverable**: Installer that generates version-aware configs

---

### Phase 4: Migration 

**Goal**: Migrate existing patches to new system

**Tasks**:
1. Choose pilot patch: **AdditionalConsoleCommands** (most complex)
2. Update pilot patch to use wrapper classes
3. Test pilot patch thoroughly
4. Migrate remaining patches:
   - EnableScriptAurPostString
   - RepeatingBlasterFix
   - FeatAndSpells
   - CreatureStats
5. Update patch documentation with new patterns

**Deliverable**: All patches using GameAPI system

---

### Phase 5: Cleanup

**Goal**: Remove deprecated code and finalize documentation

**Tasks**:
1. Remove `Kotor1Functions.h` and `Kotor1Functions.cpp`
2. Remove direct `getObjectProperty` usage (kept as private helper)
3. Update all documentation
4. Add developer guide for creating new patches
5. Add guide for adding new game versions

**Deliverable**: Clean codebase with comprehensive documentation

---

### Phase 6: Multi-Version Support (Future)

**Goal**: Add support for additional game versions

**Tasks**:
1. Reverse engineer Steam KotOR 1 addresses
2. Create `Kotor1_Steam.toml`
3. Test with Steam version
4. Create version-specific hooks files where needed
5. Repeat for KotOR 2 versions

**Deliverable**: Working multi-version support

---

## Migration Guide

### For Patch Developers

#### Before (Old System)
```cpp
// Old: Direct function pointer usage with magic offsets
#include "../Common/Kotor1Functions.h"

void myHook(void* creature) {
    // Magic offset 0xa74
    void* creatureStats = getObjectProperty<void*>(creature, 0xa74);

    // Direct function pointer call
    int hasFeat = creatureStatsHasFeat(creatureStats, 123);

    // Magic offset 0xe9
    BYTE str = getObjectProperty<BYTE>(creatureStats, 0xe9);
    setObjectProperty<BYTE>(creatureStats, 0xe9, str + 2);
}
```

#### After (New System)
```cpp
// New: Wrapper classes with named methods
#include "../Common/GameAPI/CServerCreature.h"
#include "../Common/GameAPI/CCreatureStats.h"

void myHook(void* creaturePtr) {
    // No magic numbers - clear API
    CServerCreature creature(creaturePtr);
    CCreatureStats* stats = creature.GetCreatureStats();

    // Type-safe method calls
    bool hasFeat = stats->HasFeat(123);

    // Property access via getters/setters
    BYTE str = stats->GetSTRBase();
    stats->SetSTRBase(str + 2);

    delete stats;  // Clean up wrapper
}
```

### Benefits of Migration

✅ **Readability**: `GetCreatureStats()` vs `getObjectProperty<void*>(obj, 0xa74)`
✅ **Type Safety**: Compiler catches errors at compile time
✅ **Documentation**: IntelliSense shows available methods
✅ **Version Agnostic**: Same code works across all game versions
✅ **Maintainable**: Offset changes happen in one place (TOML)

---

## Examples

### Example 1: Simple Hook Migration

**Old Code** (`EnableScriptAurPostString`):
```cpp
void hook_AurPostString(const char* message) {
    void* vm = *(void**)VIRTUAL_MACHINE_PTR;
    CExoString cexoStr;
    constructExoStringFromCStr(&cexoStr, message);
    vmStackPushString(vm, &cexoStr);
    destructExoString(&cexoStr);
}
```

**New Code**:
```cpp
void hook_AurPostString(const char* message) {
    CVirtualMachine* vm = CVirtualMachine::GetInstance();
    if (!vm) return;

    CExoString cexoStr(message);  // Constructor wrapper
    vm->StackPushString(&cexoStr);
    // Destructor called automatically

    delete vm;
}
```

---

### Example 2: Complex Property Access

**Old Code** (`repeatingBlaster.cpp`):
```cpp
void* playerCreature = getObjectProperty<void*>(combatRound, 0x9b4);
void* inventory = getObjectProperty<void*>(playerCreature, 0xa2c);
void* item = sWInventoryGetItemInSlot(inventory, 0x10);
void* baseitem = sWItemGetBaseItem(item);
BYTE weaponWield = getObjectProperty<BYTE>(baseitem, 0x8);

int onHandAttacks = getObjectProperty<int>(combatRound, 0x990);
setObjectProperty<int>(combatRound, 0x990, onHandAttacks + 1);
```

**New Code**:
```cpp
CCombatRound combatRound(combatRoundPtr);
CServerCreature* player = combatRound.GetPlayerCreature();
CInventory* inventory = player->GetInventory();
CItem* item = inventory->GetItemInSlot(0x10);
CBaseItem* baseitem = item->GetBaseItem();
BYTE weaponWield = baseitem->GetWeaponWield();

int onHandAttacks = combatRound.GetOnHandAttacks();
combatRound.SetOnHandAttacks(onHandAttacks + 1);

// Cleanup wrappers
delete baseitem;
delete item;
delete inventory;
delete player;
```

---

### Example 3: Version-Specific Hooks

**Scenario**: Hook address differs between GoG and Steam

**Structure**:
```
Patches/MyPatch/
├── manifest.toml              # supported_versions = ["gog_1.0", "steam_1.0"]
├── hooks_gog_1.0.toml         # target_versions = ["gog_1.0"]
├── hooks_steam_1.0.toml       # target_versions = ["steam_1.0"]
└── MyPatch.dll                # Same DLL for both!
```

**`hooks_gog_1.0.toml`**:
```toml
[metadata]
target_versions = ["gog_1.0"]

[[hooks]]
address = 0x0044d490  # GoG address
type = "DETOUR"
function = "hook_AurPostString"
```

**`hooks_steam_1.0.toml`**:
```toml
[metadata]
target_versions = ["steam_1.0"]

[[hooks]]
address = 0x0044e500  # Steam address (different!)
type = "DETOUR"
function = "hook_AurPostString"
```

**DLL Code** (same for both):
```cpp
// No version awareness needed - GameAPI handles it
void hook_AurPostString(const char* message) {
    CVirtualMachine* vm = CVirtualMachine::GetInstance();
    vm->StackPushString(message);
    delete vm;
}
```

---

## Appendix

### A. Version Identification System

**Primary Identifier**: SHA-256 hash of the game executable

**Why SHA-256?**
- Definitive - no ambiguity about which exact build
- Foolproof - handles confusing KotOR 2 version naming
- Already computed by GameDetector
- Stored in GameVersion.Hash property

**Examples of Known Versions** (from GameDetector.cs):
- `9C10E0450A...` - KotOR 1 GoG 1.0.3
- `34E6D971C0...` - KotOR 1 Steam 1.0.3
- `777BEE235A...` - KotOR 2 GoG Aspyr
- `6A522E7163...` - KotOR 2 Steam Aspyr
- `92D7800687...` - KotOR 2 Legacy 1.0
- `0912D1942D...` - KotOR 2 Legacy 1.0b

**Friendly Names**: Used in manifest.toml keys for human readability only

---

### B. Backwards Compatibility

**During Migration**:
- Keep `Kotor1Functions.h/cpp` until all patches migrated
- Old and new systems can coexist
- GameAPI uses same underlying `getObjectProperty` helper

**After Migration**:
- Remove old files completely
- `getObjectProperty` becomes private implementation detail

---

### C. Performance Considerations

**Function Pointer Caching**:
- Addresses loaded once at initialization
- Cached in static members of wrapper classes
- No runtime overhead for lookups

**Wrapper Object Lifetime**:
- Wrappers are lightweight (just raw pointer + cached function pointers)
- Create on stack or heap as needed
- Delete when done (similar to smart pointers)

**Memory Footprint**:
- Address database: ~2KB per version (TOML file)
- Runtime cache: ~1KB in memory (address map)
- Negligible impact

---

### D. Future Enhancements

**Potential Improvements**:
1. **Smart Pointers**: Wrap game objects in RAII wrappers
2. **High-Level APIs**: Add convenience methods on top of low-level API
3. **Auto-Detection**: Let DLL detect version from exe instead of config
4. **Hot-Reload**: Reload address database without restarting game
5. **Validation**: Check function availability before installation

---

**End of Document**
