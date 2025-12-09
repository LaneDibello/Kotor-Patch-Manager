# KotOR Patch Manager

A runtime patching system for Star Wars: Knights of the Old Republic and Knights of the Old Republic II: The Sith Lords. This tool enables developers to create and distribute patches that modify game behavior without permanently altering the game executable.

## Overview

KotOR Patch Manager provides a complete solution for creating, installing, and running patches for KOTOR games. The system consists of three main components:

- **KPatchCore** (C#): Patch management library that handles installation, validation, and configuration generation
- **KPatchLauncher** (C#/Avalonia): Dual-mode application providing both GUI and CLI interfaces for patch management and game launching
- **KotorPatcher** (C++): Runtime DLL that gets injected into the game process to apply patches at runtime

Unlike traditional patching tools that modify game files directly, this system uses DLL injection to apply patches in memory, leaving the original game files untouched. Patches are packaged as `.kpatch` archives containing compiled DLLs, hook configurations, and metadata.

## Features

- **Non-invasive patching**: Patches are applied at runtime via DLL injection, preserving original game files
- **Multi-version support**: Automatic game version detection with support for GOG, Steam, and physical releases
- **Version-aware addressing**: Centralized address databases enable patches to work across different game builds
- **Two hook types**: SIMPLE hooks for byte replacement, DETOUR hooks for function interception with parameter extraction
- **Dependency management**: Patch manifests support requirements and conflict declarations
- **Backup system**: Automatic backup creation before installation
- **Developer-friendly**: Automated patch building and packaging tools

## Architecture

### Installation Flow

When a user installs patches:

1. **KPatchLauncher** (GUI mode) or **KPatchCore** (programmatic) scans a directory for `.kpatch` files
2. Each patch's `manifest.toml` is parsed to extract metadata, supported versions, and dependencies
3. The game executable is analyzed to detect version (via SHA-256 hash)
4. Version compatibility is validated against each patch's `supported_versions`
5. Matching `*.hooks.toml` files are selected and merged based on version targeting
6. Patch DLLs and configuration files are extracted to the game directory
7. A unified `patch_config.toml` is generated with all active patches
8. The appropriate address database is copied to `addresses.toml` in the game directory

### Runtime Flow

When the game is launched:

1. **KPatchLauncher** (CLI mode) injects `KotorPatcher.dll` into the suspended game process
2. `KotorPatcher.dll` reads `patch_config.toml` and extracts the target version SHA
3. An environment variable `KOTOR_VERSION_SHA` is set with the version identifier
4. Each patch DLL is loaded via `LoadLibrary`
5. Patch DLLs call `GameVersion::Initialize()` which:
   - Reads the version SHA from the environment variable
   - Loads `addresses.toml` from the game directory
   - Validates SHA matches and caches function addresses/offsets
6. `KotorPatcher` applies hooks according to configuration:
   - **SIMPLE hooks**: Direct byte replacement in memory
   - **DETOUR hooks**: JMP trampolines redirecting to wrapper stubs that call patch functions
7. The game process resumes and runs with patches active

### Component Details

#### KPatchCore

Core library providing patch management functionality:

- **PatchRepository**: Scans directories for `.kpatch` files and loads patch metadata
- **PatchApplicator**: Orchestrates installation, validation, and configuration generation
- **PatchRemover**: Handles patch uninstallation and cleanup
- **GameDetector**: Identifies game versions by hashing executables and matching against known versions
- **Parsers**: TOML parsers for manifests, hooks, and configuration files
- **Validators**: Dependency resolution, conflict detection, and version compatibility checking

#### KPatchLauncher

Dual-mode application built with Avalonia UI:

- **GUI Mode** (no arguments): Visual interface for browsing patches, selecting game installation, and managing patch installations
- **CLI Mode** (with arguments): Command-line launcher that injects patches and starts the game

The launcher handles process creation with `CREATE_SUSPENDED`, thread hijacking for DLL injection, and process resumption. It supports different injection methods based on game distribution (Steam, GOG, etc.).

#### KotorPatcher

C++ runtime DLL that performs the actual patching:

- **Config Reader**: Parses `patch_config.toml` using Tomlyn (C++/CLI wrapper)
- **Patcher**: Applies hooks to game memory with proper protection handling
- **Trampoline System**: Generates JMP instructions to redirect execution
- **Wrapper Generator**: Creates runtime stubs for DETOUR hooks that:
  - Save CPU state (registers + flags)
  - Extract parameters from registers/stack based on configuration
  - Call patch functions with extracted parameters
  - Execute "stolen bytes" (original instructions overwritten by JMP)
  - Restore CPU state and return to game code

## Project Structure

```text
Kotor-Patch-Manager/
├── src/
│   ├── KPatchCore/          # C# patch management library
│   │   ├── Applicators/     # Installation/removal logic
│   │   ├── Detectors/       # Game version detection
│   │   ├── Managers/        # Patch repository and orchestration
│   │   ├── Models/          # Data models (PatchManifest, GameVersion, etc.)
│   │   ├── Parsers/         # TOML parsing for manifests/hooks/configs
│   │   └── Validators/      # Dependency and compatibility validation
│   │
│   ├── KPatchLauncher/      # C# Avalonia GUI/CLI application
│   │   ├── Models/          # Application settings
│   │   ├── ViewModels/      # MVVM view models
│   │   ├── Views/           # Avalonia UI definitions
│   │   └── ProcessInjector.cs  # DLL injection implementation
│   │
│   └── KotorPatcher/        # C++ runtime DLL
│       ├── src/             # Source files
│       │   ├── dllmain.cpp  # DLL entry point
│       │   ├── config_reader.cpp  # TOML config parsing
│       │   ├── patcher.cpp  # Hook application logic
│       │   └── trampoline.cpp  # JMP generation
│       └── include/         # Headers
│
├── Patches/                 # Example patches and shared code
│   ├── Common/              # Shared GameAPI wrapper classes
│   │   └── GameAPI/         # C++ wrappers for game classes
│   ├── AdditionalConsoleCommands/  # Example DETOUR patch
│   ├── SemiTransparentLetterbox/  # Example SIMPLE patch
│   └── create-patch.bat    # Automated patch builder
│
├── AddressDatabases/         # Version-specific address databases
│   ├── kotor1_gog_103.toml
│   ├── kotor1_steam_103.toml
│   └── kotor2_gog_aspyr.toml
│
└── docs/                    # Architecture and design documents
```

## Building from Source

### Prerequisites

- **.NET 8.0 SDK** for C# components
- **Visual Studio 2022** with C++ desktop development workload
- **Windows SDK 10.0**
- **Avalonia UI** (automatically restored via NuGet)

### Building KPatchCore and KPatchLauncher

```powershell
cd src/KPatchCore
dotnet build -c Release

cd ../KPatchLauncher
dotnet publish -c Release -r win-x86 --self-contained -p:PublishSingleFile=true
```

### Building KotorPatcher

The C++ DLL must be built as 32-bit (x86) to match KOTOR's architecture:

```powershell
cd src/KotorPatcher
msbuild KotorPatcher.vcxproj /p:Configuration=Release /p:Platform=Win32
```

### Building Patches

Each patch directory contains source files and a `create-patch.bat` script that:

1. Validates required files (`manifest.toml`, `*.hooks.toml`)
2. Detects patch type (SIMPLE vs DETOUR based on `.cpp` files)
3. Compiles DLLs if needed (auto-detects Visual Studio)
4. Packages everything into a `.kpatch` ZIP archive

Run from within a patch directory:

```cmd
create-patch.bat
```

## Creating Patches

### Patch Structure

A patch consists of:

- **manifest.toml**: Metadata, version support, dependencies
- ***.hooks.toml**: Hook definitions (one or more files, can be version-specific)
- **Patch DLL** (for DETOUR hooks): Compiled C++ DLL with exported functions
- **Additional files** (optional): Game assets, scripts, etc.

### Manifest Format

```toml
[patch]
id = "my-patch"
name = "My Patch"
version = "1.0.0"
author = "Your Name"
description = "What this patch does"

requires = ["other-patch-id"]  # Optional dependencies
conflicts = ["incompatible-patch"]  # Optional conflicts

[patch.supported_versions]
kotor1_gog_103 = "9C10E0450A6EECA417E036E3CDE7474FED1F0A92AAB018446D156944DEA91435"
kotor1_steam_103 = "34E6D971C034222A417995D8E1E8FDD9F8781795C9C289BD86C499A439F34C88"
```

### Hook Types

#### SIMPLE Hooks

Direct byte replacement - no DLL required:

```toml
[[hooks]]
address = 0x006a89b2
type = "simple"
original_bytes = [0x68, 0x00, 0x00, 0x80, 0x3f]  # PUSH 1.0f
replacement_bytes = [0x68, 0x00, 0x00, 0x00, 0x3f]  # PUSH 0.5f
```

#### DETOUR Hooks

Function interception with parameter extraction:

```toml
[[hooks]]
address = 0x005cb41c
type = "detour"
function = "MyPatchFunction"
original_bytes = [0x8d, 0x4c, 0x24, 0x0c, 0xc7, 0x44, 0x24, 0x1c, 0xff, 0xff, 0xff, 0xff]

[[hooks.parameters]]
source = "eax"
type = "pointer"

[[hooks.parameters]]
source = "esp+0"
type = "int"
```

Corresponding C++ function:

```cpp
extern "C" __declspec(dllexport)
void MyPatchFunction(void* stringParam, int x) {
    // Parameters automatically extracted by wrapper system
    // No assembly required
}
```

### Version-Specific Hooks

Patches can have multiple hooks files for different versions:

```text
MyPatch/
├── manifest.toml
├── default.hooks.toml          # Applies to all versions
├── kotor1_gog_103.hooks.toml   # GoG-specific hooks
└── MyPatch.dll                 # Same DLL for all versions
```

Hooks files can target specific versions:

```toml
[metadata]
target_versions = ["9C10E0450A6EECA417E036E3CDE7474FED1F0A92AAB018446D156944DEA91435"]

[[hooks]]
# ... hooks for this version only
```

Empty `target_versions` means the file applies to all supported versions.

### Using GameAPI Wrappers

The `Patches/Common/GameAPI/` directory provides C++ wrapper classes that abstract version-specific addresses:

```cpp
#include "../Common/GameAPI/CSWSCreature.h"
#include "../Common/GameAPI/CCreatureStats.h"

void MyHook(void* creaturePtr) {
    CSWSCreature creature(creaturePtr);
    CCreatureStats* stats = creature.GetCreatureStats();
    
    // Type-safe API - no magic offsets
    bool hasFeat = stats->HasFeat(123);
    BYTE str = stats->GetSTRBase();
    stats->SetSTRBase(str + 2);
    
    delete stats;
}
```

Wrappers automatically resolve addresses from `addresses.toml` at runtime, making patches version-agnostic.

## Usage

### GUI Mode

1. Launch `KPatchLauncher.exe` (no arguments)
2. Select your KOTOR installation directory
3. Select a directory containing `.kpatch` files
4. Browse available patches and their descriptions
5. Select patches to install and click "Install"
6. Launch the game through the launcher

### CLI Mode

Launch game with patches:

```cmd
KPatchLauncher.exe "C:\Games\KOTOR\swkotor.exe"
```

The launcher automatically detects if patches are installed and injects them.

### Programmatic Usage

```csharp
using KPatchCore.Managers;

var orchestrator = new PatchOrchestrator(@"C:\Patches");
var result = orchestrator.InstallPatches(
    gameExePath: @"C:\Games\KOTOR\swkotor.exe",
    patchIds: new[] { "additional-console-commands", "script-extender" },
    createBackup: true
);

if (result.Success) {
    Console.WriteLine($"Installed {result.InstalledPatches.Count} patches");
}
```

## Address Databases

Address databases (`AddressDatabases/*.toml`) contain version-specific function addresses and memory offsets. They're organized by game class:

```toml
versions_sha = "9C10E0450A6EECA417E036E3CDE7474FED1F0A92AAB018446D156944DEA91435"

[global_pointers]
VIRTUAL_MACHINE_PTR = "0x007a3a00"
APP_MANAGER_PTR = "0x007a39fc"

[functions.CServerExoApp]
GetPlayerCreatureId = "0x004aea40"

[functions.CVirtualMachine]
StackPopInteger = "0x005d1000"
StackPushString = "0x005d1190"

[offsets.CSWSCreature]
CreatureStats = 0xa74
Inventory = 0xa2c
```

During installation, the matching database is copied to the game directory as `addresses.toml`. Patch DLLs load this at runtime via `GameVersion::Initialize()`.

## Technical Details

### DLL Injection Method

The launcher uses a thread hijacking approach:

1. Create process with `CREATE_SUSPENDED` flag
2. Allocate memory in target process for DLL path
3. Write DLL path to allocated memory
4. Get address of `LoadLibraryA` in `kernel32.dll`
5. Create remote thread calling `LoadLibraryA` with DLL path
6. Wait for thread completion
7. Resume main thread

This method works across different game distributions and doesn't require modifying the executable.

### Hook Application

**SIMPLE hooks** are straightforward memory writes:

1. Verify original bytes match expected values
2. Change memory protection to `PAGE_EXECUTE_READWRITE`
3. Write replacement bytes
4. Restore original protection
5. Flush instruction cache

**DETOUR hooks** require more complexity:

1. Load patch DLL and get function address
2. Verify original bytes
3. Disassemble original instructions to determine boundaries
4. Allocate executable memory for wrapper stub
5. Generate wrapper code that:
   - Saves CPU state
   - Extracts parameters (registers/stack)
   - Calls patch function
   - Executes stolen bytes
   - Restores state and returns
6. Write 5-byte JMP instruction to wrapper
7. Update memory protection and flush cache

### Parameter Extraction

The wrapper system supports extracting parameters from:

- **Registers**: `eax`, `ebx`, `ecx`, `edx`, `esi`, `edi`, `ebp`
- **Stack**: `esp+N` (offset from stack pointer)

Parameter types:

- `int`: 32-bit integer
- `float`: 32-bit floating point
- `pointer`: Memory address (void*)

The wrapper generates x86 machine code at runtime to push extracted values onto the stack before calling the patch function, allowing patch code to use normal C function signatures.

## Known Limitations

- **Windows only**: DLL injection and x86 architecture limit this to Windows
- **32-bit only**: KOTOR games are 32-bit, so all components must be x86
- **Version coverage**: Address databases exist for limited game versions; adding new versions requires reverse engineering
- **Hook complexity**: DETOUR hooks require careful handling of instruction boundaries and CPU state
- **No hot-reload**: Patches are applied at process start; changes require game restart

## Future Enhancements

Planned improvements documented in `docs/`:

- Multi-version architecture refactoring (in progress)
- SQLite address database migration for better querying
- Enhanced patch options and configuration
- Improved UI/UX for patch management
- Additional GameAPI wrapper classes

## License

See repository for license information.

## Credits

Developed by Lane Duncan. Built for the KOTOR modding community.
