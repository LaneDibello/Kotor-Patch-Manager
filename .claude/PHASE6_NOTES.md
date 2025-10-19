# Phase 6 Implementation Notes

## Overview
Phase 6 (Orchestration) is now complete! This phase ties together all previous phases into a cohesive patch management system.

## Components Implemented

### 1. PatchRepository.cs
**Purpose**: Manages collections of .kpatch files

**Key Features**:
- Scans directories for .kpatch files (ZIP archives)
- Loads and parses manifest.toml and hooks.toml from archives
- Validates .kpatch structure (manifest, hooks, binaries)
- Extracts patch DLLs to target directories
- Provides query interface for finding patches

**Public API**:
- `ScanPatches()` - Loads all .kpatch files from directory
- `GetPatch(patchId)` - Get specific patch
- `GetAllPatches()` - Get all available patches
- `ExtractPatchDll(patchId, targetDir)` - Extract DLL from .kpatch
- `FindPatches(predicate)` - Query patches with filter

### 2. PatchApplicator.cs
**Purpose**: Orchestrates the full patch installation process

**Installation Steps** (7 steps):
1. Validate inputs (game exe exists, etc.)
2. Detect game version
3. Load and validate patches (dependencies, conflicts, version compatibility, hook conflicts)
4. Create backup
5. Extract patch DLLs to `patches/` subdirectory
6. Generate `patch_config.toml`
7. Deploy KPatchLauncher.exe (launcher-based injection - RECOMMENDED)

**Error Handling**:
- Fails fast on errors
- Automatically restores backup on failure
- Provides detailed error messages with step-by-step progress

**Public API**:
- `InstallPatches(InstallOptions)` - Main installation method
- Returns `InstallResult` with detailed status

### 3. PatchRemover.cs
**Purpose**: Handles patch removal and restoration

**Key Features**:
- Restores from backup
- Cleans up backup files after successful restore
- Removes all patch files using safe delete helper
- Verifies clean state after removal
- Query methods for installation status

**Files Removed**:
- patches/ directory and all DLLs
- patch_config.toml
- KotorPatcher.dll
- KPatchLauncher.exe and related files (.dll, .runtimeconfig.json, .deps.json)
- KPatchCore.dll
- Tomlyn.dll

**Public API**:
- `RemoveAllPatches(gameExePath)` - Remove all patches
- `RemovePatches(gameExePath, patchIds)` - Remove specific (MVP: removes all)
- `HasPatchesInstalled(gameExePath)` - Check if game is patched
- `GetInstallationInfo(gameExePath)` - Get detailed installation info

**Note**: Selective patch removal (removing only specific patches) is not supported in MVP. All removal operations restore the backup and remove all patch files.

### 4. PatchOrchestrator.cs
**Purpose**: Main public API facade for the system

**Key Features**:
- Simple, high-level API for consumers
- Automatically scans for patches on construction
- Delegates to PatchApplicator and PatchRemover
- Provides convenient query methods

**Public API Example**:
```csharp
var orchestrator = new PatchOrchestrator("/path/to/patches");

// Install patches
var result = orchestrator.InstallPatches(
    gamePath: "C:/Games/KOTOR/swkotor.exe",
    patchIds: new[] { "bugfix-pack", "widescreen-fix" }
);

// Remove patches
var removeResult = orchestrator.UninstallPatches("C:/Games/KOTOR/swkotor.exe");

// Query patches
var patches = orchestrator.GetAvailablePatches();
var isPatch = orchestrator.IsPatched("C:/Games/KOTOR/swkotor.exe");
```

## Architecture Flow

### Installation Flow
```
User
  → PatchOrchestrator.InstallPatches()
    → PatchRepository.GetPatch() [for each patch]
    → PatchApplicator.InstallPatches()
      → GameDetector.DetectVersion()
      → Validators (Dependency, Hook, GameVersion)
      → BackupManager.CreateBackup()
      → PatchRepository.ExtractPatchDll() [for each patch]
      → ConfigGenerator.GenerateConfigFile()
      → LoaderInjector.InjectLoader() [optional]
    → Returns InstallResult
```

### Removal Flow
```
User
  → PatchOrchestrator.UninstallPatches()
    → PatchRemover.RemoveAllPatches()
      → BackupManager.FindLatestBackup()
      → BackupManager.RestoreBackup()
      → BackupManager.DeleteBackup() [cleanup after restore]
      → SafeDeleteFile() for all patch-related files
    → Returns RemovalResult
```

## Key Design Decisions

1. **Fail-Fast with Automatic Rollback**: If any step fails during installation, the system automatically restores the backup and cleans up.

2. **Result Pattern Throughout**: All operations return result objects with detailed status, not exceptions. This makes error handling explicit and predictable.

3. **MVP Simplifications**:
   - No selective patch removal (always removes all and restores backup)
   - Launcher-based injection (PE modification experimental, not recommended)
   - No runtime patch management (install/uninstall only)

4. **Separation of Concerns**:
   - Repository: .kpatch file management
   - Applicator: Installation orchestration
   - Remover: Removal and cleanup
   - Orchestrator: Public API facade

## Testing Requirements

Phase 6 tests should verify:

1. **PatchRepository**:
   - Loading valid .kpatch files
   - Rejecting invalid .kpatch files (missing manifest, hooks, or binary)
   - Extracting DLLs correctly
   - Querying patches

2. **PatchApplicator** (Integration Tests):
   - Full installation flow with valid patches
   - Validation failures (missing dependencies, conflicts, version mismatch)
   - Hook conflict detection
   - Backup creation and restoration on failure
   - Directory structure creation (patches/ subdirectory)
   - Config generation

3. **PatchRemover**:
   - Removal with backup restoration
   - Removal without backup (cleanup only)
   - Installation status queries
   - Clean state verification

4. **PatchOrchestrator**:
   - End-to-end workflow
   - API convenience methods

## .kpatch File Format (Reminder)

```
my_patch.kpatch (ZIP file)
├── manifest.toml      # Patch metadata
├── hooks.toml         # Hook definitions
└── binaries/
    └── windows_x86.dll    # Patch DLL
```

## Dependencies on Previous Phases

Phase 6 successfully integrates:
- ✅ Phase 1: All models (GameVersion, Hook, PatchManifest, PatchConfig, BackupInfo, PatchResult)
- ✅ Phase 2: All parsers (ManifestParser, HooksParser for .kpatch loading)
- ✅ Phase 3: BackupManager, ConfigGenerator, GameDetector
- ✅ Phase 4: All validators (PatchValidator, HookValidator, DependencyValidator, GameVersionValidator)
- ✅ Phase 5: LoaderInjector (experimental, optional)

## Known Limitations

1. **No Selective Removal**: MVP only supports "remove all patches" operation
2. **PE Injection Experimental**: PE modification not recommended; use launcher-based injection
3. **No Patch Signing**: Trust is implicit (no signature verification)
4. **Single Architecture**: Only windows_x86 binaries supported
5. **No Version Migration**: Can't upgrade patches in-place (must remove and reinstall)

## Completed Enhancements

- ✅ Phase 7: Console Application (KPatchConsole) with install/uninstall/list/status commands
- ✅ Launcher-based DLL injection (KPatchLauncher.exe) - RECOMMENDED approach
- ✅ Parameter extraction system for INLINE hooks
- ✅ Working example patch (EnableScriptAurPostString)

## File Locations

```
KPatchCore/
├── Managers/
│   ├── PatchRepository.cs        ✅ NEW
│   └── PatchOrchestrator.cs      ✅ NEW
└── Applicators/
    ├── PatchApplicator.cs        ✅ NEW
    └── PatchRemover.cs           ✅ NEW
```

## Build Status
✅ Builds successfully with 0 errors, 2 warnings (nullability in GameVersionValidator - pre-existing)
