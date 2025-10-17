# KPatchCore Implementation Plan

## Current Status
- ✅ **Phase 1 Complete**: Models & Common utilities
- ✅ **Phase 2 Complete**: Parsers (ManifestParser, HooksParser, ExecutableParser)
- ✅ **Phase 3 Complete**: Simple Operations (BackupManager, ConfigGenerator, GameDetector)
- ✅ **Phase 4 Complete**: Validators (all 4 validators)
- ✅ **Phase 5 Complete**: PE Manipulation (LoaderInjector - experimental)
- ✅ **Phase 6 Complete**: Orchestration (PatchRepository, PatchApplicator, PatchRemover, PatchOrchestrator)
- ⏳ **Next**: Phase 7 - Console Application (MVP)

## Overall Architecture

KPatchCore is the C# business logic library that handles patch installation at installation time. It works with the C++ runtime (kotor_patcher.dll) which handles patches at game runtime.

### What KPatchCore Does:
1. Parse patch manifests (.kpatch files)
2. Detect game version and locate installations
3. Validate patches (dependencies, conflicts, version compatibility)
4. Modify game EXE to import `kotor_patcher.dll`
5. Manage backups and restoration
6. Generate `patch_config.toml` for runtime

## Implementation Phases

### ✅ Phase 1: Foundation (COMPLETE)

**Models/** - Data structures
- `GameVersion.cs` - Game edition info (Platform, Distribution, Version, Architecture)
- `Hook.cs` - Single hook point with wrapper system support
- `PatchManifest.cs` - Patch metadata from manifest.toml
- `PatchConfig.cs` - Runtime config structure (mirrors C++ config)
- `BackupInfo.cs` - Backup metadata with verification
- `PatchResult.cs` - Operation result pattern (no exceptions)

**Common/** - Utilities
- `FileHasher.cs` - SHA256 hashing for files
- `PathHelpers.cs` - Safe path operations, backup naming, KOTOR detection

### ✅ Phase 2: Parsers (COMPLETE)

Parse TOML files and executable headers.

**Parsers/**
- `ManifestParser.cs` - Parse patch manifest.toml using Tomlyn
- `HooksParser.cs` - Parse hooks.toml from .kpatch files (includes wrapper system fields)
- `ExecutableParser.cs` - Read PE headers using PeNet (architecture, imports)

**Example manifest.toml structure:**
```toml
[patch]
id = "widescreen-fix"
name = "Widescreen Resolution Fix"
version = "1.2.0"
author = "CommunityModder"
description = "Enables 16:9 and 21:9 resolutions"
requires = []
conflicts = ["old-widescreen-mod"]

[patch.supported_versions]
kotor1_gog_103 = "ABC123..." # SHA256 hash
```

**Example hooks.toml structure:**
```toml
[[hooks]]
address = 0x401234
function = "FixedResolutionHandler"
original_bytes = [0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x20]
type = "inline"
exclude_from_restore = ["eax"]
```

### Phase 3: Simple Operations (CURRENT)

**Applicators/**
- `BackupManager.cs` - Create/restore backups, verify integrity
- `ConfigGenerator.cs` - Generate patch_config.toml for runtime using Tomlyn

**Detectors/**
- `GameDetector.cs` - Hash file and lookup version in known versions database

**Note:** GameLocator removed from MVP - using absolute paths instead. Auto-detection deferred to post-MVP.

**Known Versions Database** (in GameDetector.cs):
```csharp
private static readonly Dictionary<string, GameVersion> KnownVersions = new() {
    ["<SHA256_HASH>"] = new GameVersion {
        Platform = Platform.Windows,
        Distribution = Distribution.GOG,
        Version = "1.03",
        Architecture = Architecture.x86,
        FileSize = 5242880,
        Hash = "<SHA256_HASH>"
    }
};
```

### Phase 4: Validators

Ensure patches are valid and compatible.

**Validators/**
- `PatchValidator.cs` - Validate patch structure/manifest
- `HookValidator.cs` - Check hooks don't overlap, validate addresses
- `DependencyValidator.cs` - Verify dependencies satisfied, no conflicts
- `GameVersionValidator.cs` - Ensure patch supports detected game version

### ✅ Phase 5: PE Manipulation (Complex) - COMPLETE

**Applicators/**
- `LoaderInjector.cs` - ✅ Modify PE import table using PeNet
  - Add `kotor_patcher.dll` to import table
  - This is the ONLY modification to game EXE
  - Must be reversible
  - ⚠️ **Note**: Current implementation is experimental. See `PE_INJECTION_NOTES.md` for limitations and alternative approaches (launcher injection recommended)

### ✅ Phase 6: Orchestration - COMPLETE

High-level operations that tie everything together.

**Applicators/**
- `PatchApplicator.cs` - ✅ Orchestrate full patch installation
  - Detect game version
  - Validate patches (dependencies, conflicts, version compatibility, hook conflicts)
  - Create backup (with automatic restoration on failure)
  - Extract patch DLLs to patches/ subdirectory
  - Generate patch_config.toml
  - Inject loader (optional, experimental)
- `PatchRemover.cs` - ✅ Remove patches/restore backups
  - Find and restore backups
  - Remove all patch files
  - Installation status queries

**Managers/**
- `PatchRepository.cs` - ✅ Manage collection of .kpatch files
  - Scan directories for .kpatch archives
  - Load and parse manifests and hooks
  - Extract patch DLLs
- `PatchOrchestrator.cs` - ✅ Main public API facade
  - High-level InstallPatches() and UninstallPatches() methods
  - Patch repository queries
  - Installation status checks

**PatchOrchestrator Public API:**
```csharp
var orchestrator = new PatchOrchestrator("/path/to/patches");

// Install patches
var result = orchestrator.InstallPatches(
    gamePath: "C:/Games/KOTOR",
    patchIds: new[] { "bugfix-pack", "widescreen-fix" }
);

// Remove patches
orchestrator.UninstallPatches("C:/Games/KOTOR");

// Query patches
var patches = orchestrator.GetAvailablePatches();
```

### Phase 7: Console Application (MVP)

Simple CLI tool for testing and basic usage.

**KPatchConsole/**
- `Program.cs` - Entry point with basic command parsing
- `Commands/InstallCommand.cs` - Install patches
- `Commands/ListCommand.cs` - List available patches
- `Commands/StatusCommand.cs` - Check installation status
- `Commands/UninstallCommand.cs` - Remove patches

**Smoke Tests in Program.cs:**
```csharp
--test-models       # Test model creation and validation
--test-parsers      # Test TOML parsing
--test-detection    # Test game detection
--test-backup       # Test backup creation/restoration
--test-config-gen   # Test config generation
--install           # Full installation
--list              # List patches
--uninstall         # Remove patches
```

## .kpatch Archive Format

ZIP file structure:
```
my_patch.kpatch (ZIP)
├── manifest.toml      # Patch metadata
├── hooks.toml         # Hook definitions with wrapper system fields
└── binaries/
    ├── windows_x86.dll    # Patch DLL for Win32
    ├── windows_x64.dll    # (Future) 64-bit
    └── [other platforms]
```

## Testing Strategy

### Progressive Testing
1. **Unit test each phase** - Test classes as we build them
2. **Test with dummy files** - Use test EXE before touching real KOTOR
3. **Smoke tests in console** - Simple CLI tests for each component
4. **Integration tests** - Full flow with test patches
5. **Real-world test** - Only test on KOTOR copy when confident

### Test Files Needed
- `test_manifest.toml` - Sample patch manifest
- `test_hooks.toml` - Sample hooks config
- `test_dummy.exe` - Minimal PE executable for testing
- `test_patch.dll` - Simple test patch DLL

## Error Handling Philosophy

### KPatchCore (Installation Time)
- **Fail fast**: Stop immediately on errors
- **Revert on failure**: Restore backup if installation fails partway
- **Detailed errors**: Tell user exactly what went wrong
- **Use PatchResult**: Return type pattern instead of exceptions
- **Validate everything**: Check all conditions before modifying files

### Example Error Flow
```csharp
var result = orchestrator.InstallPatches(gamePath, patchIds);
if (!result.Success)
{
    Console.WriteLine($"Installation failed: {result.Error}");
    foreach (var msg in result.Messages)
        Console.WriteLine($"  - {msg}");
    return 1;
}
```

## DLL-to-EXE Function Calling

Patch DLLs can call functions in the game executable using absolute addresses:

### How Patches Call Game Functions
```cpp
// In patch DLL - call game function at known address
typedef void (__stdcall *GameFunction)(int param);

void MyPatchFunction() {
    GameFunction originalFunc = (GameFunction)0x00501234;
    originalFunc(42);  // Direct call to swkotor.exe function
}
```

### Important Notes:
- **KOTOR exports only `entry`** - Use absolute addresses for all game functions
- **Calling conventions matter** - Match the original (`__stdcall`, `__cdecl`, `__fastcall`)
- **Stack management** - Ensure stack is balanced after calls
- **Wrapper system helps** - Automatically preserves registers for `HookType.Inline` and `HookType.Wrap`

### Test Executable
- Real KOTOR test copy: `C:\Users\laned\Documents\KotOR Installs\swkotor.exe`
- Safe to use for testing (backed up)

## Integration with C++ Runtime

### Installation Flow
1. **KPatchCore** (C#):
   - Validates patches
   - Creates backup
   - Modifies EXE import table → adds `kotor_patcher.dll`
   - Copies `kotor_patcher.dll` to game directory
   - Copies patch DLLs to `patches/` subfolder
   - Generates `patch_config.toml`

2. **kotor_patcher.dll** (C++):
   - Loaded by game at startup
   - Reads `patch_config.toml`
   - Loads patch DLLs
   - Applies hooks with wrapper system

### Generated patch_config.toml
```toml
[[patches]]
id = "widescreen-fix"
dll = "patches/widescreen-fix.dll"

  [[patches.hooks]]
  address = 0x401234
  function = "FixedResolutionHandler"
  original_bytes = [0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x20]
  type = "inline"
  exclude_from_restore = ["eax"]
```

## Dependencies (Already Configured)

- **PeNet** (v5.1.0) - PE file manipulation
- **Tomlyn** (v0.19.0) - TOML parsing/generation
- **System.IO.Compression** - ZIP handling for .kpatch files
- **.NET 8.0** - Target framework

## Next Steps

1. ✅ **Complete Phase 1** - Models & Common utilities
2. ✅ **Complete Phase 2** - Parsers (ManifestParser, HooksParser, ExecutableParser)
3. 🚧 **Implement Phase 3** - Simple Operations (BackupManager, ConfigGenerator, GameDetector)
4. ⏳ **Continue with Phases 4-7** - Following plan above

## File Organization

```
KPatchCore/
├── Models/
│   ├── GameVersion.cs       ✅
│   ├── Hook.cs              ✅
│   ├── PatchManifest.cs     ✅
│   ├── PatchConfig.cs       ✅
│   ├── BackupInfo.cs        ✅
│   └── PatchResult.cs       ✅
├── Common/
│   ├── FileHasher.cs        ✅
│   └── PathHelpers.cs       ✅
├── Parsers/
│   ├── ManifestParser.cs    ✅
│   ├── HooksParser.cs       ✅
│   └── ExecutableParser.cs  ✅
├── Detectors/
│   └── GameDetector.cs      ⏳
├── Validators/
│   ├── PatchValidator.cs    ⏳
│   ├── HookValidator.cs     ⏳
│   ├── DependencyValidator.cs ⏳
│   └── GameVersionValidator.cs ⏳
├── Applicators/
│   ├── BackupManager.cs     ⏳
│   ├── ConfigGenerator.cs   ⏳
│   ├── LoaderInjector.cs    ⏳
│   ├── PatchApplicator.cs   ⏳
│   └── PatchRemover.cs      ⏳
└── Managers/
    ├── PatchRepository.cs   ⏳
    └── PatchOrchestrator.cs ⏳
```

## Notes for Next Agent

- All Models are complete and properly typed with nullable reference types enabled
- Common utilities handle edge cases (file not found, path traversal, etc.)
- Using C# 12 features (required init properties, file-scoped namespaces)
- PatchResult pattern chosen over exceptions for expected failures
- Hook class includes all wrapper system fields (type, preserve_*, exclude_from_restore)
- Ready to start on Parsers next - will use Tomlyn for TOML, PeNet for PE files

## Questions/Decisions Made

- ✅ **Error handling**: PatchResult pattern (not exceptions)
- ✅ **TOML library**: Tomlyn (already in NuGet)
- ✅ **PE library**: PeNet (already in NuGet)
- ✅ **Logging**: Use PatchResult.Messages for now, proper logging later
- ⏳ **Patch signing**: Future feature
- ⏳ **Auto-update**: Future feature

Last Updated: 2025-10-16
Current Phase: Phase 3 - Simple Operations
Next: Phase 4 - Validators
