# KPatchCore C# Library

KPatchCore is the installation-time framework for the KotOR Patch Manager. It handles patch discovery, validation, dependency resolution, and installation preparation. This library reads .kpatch files, validates game compatibility, resolves dependencies, and generates the patch_config.toml consumed by the runtime KotorPatcher.dll.

## Architecture Overview

KPatchCore operates in a pipeline:

1. **Discovery**: Scan .kpatch archives and load patch metadata
2. **Validation**: Verify game version compatibility, dependencies, and hook conflicts
3. **Resolution**: Calculate dependency-ordered installation sequence
4. **Installation**: Extract patch DLLs, generate patch_config.toml, deploy runtime files

The library uses a **PatchResult** pattern instead of exceptions for expected failures, providing clear success/failure information with detailed error messages.

## Project Structure

### Managers

**PatchOrchestrator**: Public API facade for the patch management system. Primary entry point for consumers. Provides high-level methods: InstallPatches, UninstallPatches, GetAvailablePatches, IsPatched.

**PatchRepository**: Manages collections of .kpatch files. Scans patches directory, loads patch archives, extracts metadata and hooks, provides patch lookup by ID. Handles version-specific hooks file selection based on target game SHA-256.

### Applicators

**PatchApplicator**: Orchestrates the full installation pipeline. Validates inputs, detects game version, loads and validates patches, checks dependencies/conflicts/version compatibility, creates backup, applies STATIC hooks to executable file, extracts patch DLLs, generates patch_config.toml, deploys KotorPatcher.dll.

**StaticHookApplicator**: Applies STATIC hooks directly to the game executable at install-time. Parses PE headers, converts virtual addresses to file offsets, verifies original bytes, writes replacement bytes. Used for patches that modify PE headers or other structures that must be patched before the executable loads.

**ConfigGenerator**: Generates patch_config.toml for the runtime patcher. Converts PatchConfig objects to TOML format with patches array, hooks definitions, and target version SHA. Filters out STATIC hooks as they are already applied to the file.

**BackupManager**: Creates and restores backups of game directories. Stores backup metadata with game version and installed patches list. Automatically restores on installation failure (including STATIC hook failures).

**PatchRemover**: Removes installed patches from game directory. Deletes patch_config.toml, removes patches directory, restores from backup if available (reverting STATIC hook changes).

### Parsers

**ManifestParser**: Parses manifest.toml files from .kpatch archives. Extracts patch metadata: id, name, version, author, description, dependencies, conflicts, supported versions. Returns PatchManifest objects.

**HooksParser**: Parses hooks.toml files from .kpatch archives. Supports metadata section with target_versions for game-specific hooks. Extracts hook definitions: address, type, original_bytes, function names, parameters. Validates hook configurations. Supports DETOUR, SIMPLE, REPLACE, and STATIC hook types.

**ExecutableParser**: Parses basic PE executable metadata (architecture, file size). Used for initial game detection.

**PeHeaderParser**: Parses PE (Portable Executable) headers including DOS header, PE signature, COFF header, optional header, and section headers. Converts virtual addresses to file offsets for STATIC hook application. Provides methods to read and write bytes at virtual addresses in the executable file.

### Validators

**DependencyValidator**: Validates patch dependencies and conflicts. Detects circular dependencies, calculates topological installation order, ensures all required patches are available.

**GameVersionValidator**: Verifies patches support detected game version. Checks that game SHA-256 matches patch's supported_versions dictionary.

**HookValidator**: Validates hook configurations. Detects address conflicts between multiple patches, ensures hooks meet minimum byte requirements, validates parameter configurations.

**PatchValidator**: Orchestrates validation across multiple validators. Performs comprehensive pre-installation checks.

### Detectors

**GameDetector**: Identifies game version by SHA-256 hash of executable. Maintains database of known KotOR versions (GOG, Steam, Physical distributions). Returns GameVersion with platform, distribution, version number, architecture.

### Common

**FileHasher**: Computes SHA-256 hashes of files. Used for game version detection and verification.

**PathHelpers**: Utility functions for path manipulation and normalization.

## Key Classes and Structures

### PatchManifest

Represents metadata from a patch's manifest.toml:

- **Id**: Unique patch identifier (e.g., "script-extender")
- **Name**: Human-readable name
- **Version**: Semantic version string
- **Author**: Patch creator
- **Description**: Patch functionality description
- **Requires**: List of dependency patch IDs
- **Conflicts**: List of conflicting patch IDs
- **SupportedVersions**: Dictionary mapping game version ID to expected SHA-256 hash

### Hook

Represents a single hook point in game code:

- **Address**: Virtual Memory address to patch
- **Type**: DETOUR, SIMPLE, REPLACE, or STATIC
- **Function**: Exported function name (DETOUR only)
- **OriginalBytes**: Bytes at hook address for verification
- **ReplacementBytes**: New bytes to write (SIMPLE/REPLACE/STATIC only)
- **Parameters**: Parameter extraction configuration (DETOUR only)
- **PreserveRegisters/PreserveFlags**: State preservation options (DETOUR only)
- **ExcludeFromRestore**: Registers to keep modified after hook (DETOUR only)
- **SkipOriginalBytes**: Whether to skip re-executing stolen bytes (DETOUR only)

### PatchResult / PatchResult&lt;T&gt;

Result pattern for operations that may fail:

- **Success**: Boolean indicating operation outcome
- **Error**: Error message if failed
- **Messages**: Additional informational messages
- **Data**: Returned data (generic type for PatchResult&lt;T&gt;)

Factory methods: PatchResult.Ok(), PatchResult.Fail(). Chainable with WithMessage().

### GameVersion

Represents a detected game version:

- **Title**: KOTOR1, KOTOR2, or Unknown
- **Platform**: Windows, Linux, macOS
- **Distribution**: GOG, Steam, Physical, Other
- **Version**: Version number string
- **Architecture**: x86 or x64
- **Hash**: SHA-256 hash of executable
- **FileSize**: Executable file size in bytes

### PatchConfig

Configuration for patch_config.toml generation:

- **TargetVersionSha**: Game version SHA-256 hash
- **Patches**: List of patch entries with ID, DLL path, and hooks

### ParameterInfo / Parameter

Defines parameter extraction for DETOUR hooks:

- **Source**: Register name ("eax", "ebx") or stack offset ("esp+0", "esp+4")
- **Type**: INT, UINT, POINTER, FLOAT, BYTE, SHORT

## Core Functions

### Installation Pipeline

**PatchOrchestrator.InstallPatches()**: Main installation entry point. Takes InstallOptions with game path, patch IDs, backup preference. Returns InstallResult with success status, installed patches, backup info, game version, config path.

**PatchApplicator.InstallPatches()**: Executes 8-step installation process:
1. Validate inputs (game exe exists)
2. Detect game version (SHA-256 hash)
3. Load and validate patches (dependencies, conflicts, version compatibility, hook conflicts)
4. Create backup (optional but recommended)
5. Apply STATIC hooks directly to executable file
6. Extract patch DLLs to patches/ directory
7. Generate patch_config.toml with version-specific runtime hooks (DETOUR/SIMPLE/REPLACE)
8. Deploy KotorPatcher.dll to game directory

### Patch Discovery

**PatchRepository.ScanPatches()**: Scans patches directory for .kpatch files. Opens each archive, loads manifest.toml and hooks files, validates DLL existence for DETOUR hooks, populates internal patch dictionary.

**PatchRepository.LoadPatch()**: Loads single .kpatch archive. Parses manifest, detects hooks files (pattern: *hooks.toml), verifies binary exists if DETOUR hooks present. Returns PatchEntry with manifest, hooks, and archive path.

**PatchRepository.LoadHooksForVersion()**: Loads version-specific hooks by filtering hooks files. Checks [metadata] section's target_versions array, includes file if SHA matches or no target specified. Merges hooks from all matching files.

### Validation

**DependencyValidator.ValidateDependencies()**: Ensures all required patches are in install set. Detects missing dependencies.

**DependencyValidator.ValidateNoConflicts()**: Checks for conflicting patches in install set. Reports conflicts if found.

**DependencyValidator.CalculateInstallOrder()**: Topological sort of patches based on dependencies. Ensures dependencies installed before dependents.

**GameVersionValidator.ValidateAllPatchesSupported()**: Verifies game version SHA exists in each patch's supported_versions. Fails if any patch doesn't support detected version.

**HookValidator.ValidateMultiPatchHooks()**: Detects address conflicts across multiple patches. Ensures no two patches hook same address.

### Configuration Generation

**ConfigGenerator.GenerateConfigFile()**: Converts PatchConfig to TOML format and writes to file. Includes target_version_sha, patches array with IDs, DLL paths, and hooks.

**ConfigGenerator.GenerateConfigString()**: Produces TOML string from PatchConfig. Serializes addresses, byte arrays, hook types, parameters, and state preservation options.

### Game Detection

**GameDetector.DetectVersion()**: Computes SHA-256 of game executable, looks up in known versions database. Returns GameVersion with full metadata or unknown version with hash.

**GameDetector.GetKnownVersions()**: Returns dictionary of all recognized game versions. Currently supports KotOR 1 (GOG, Steam, Cracked) and KotOR 2 (GOG, Steam, Legacy) distributions.

### Backup Management

**BackupManager.CreateBackup()**: Creates timestamped backup of game directory. Stores game version hash, installed patches list. Returns BackupInfo with backup path and metadata.

**BackupManager.RestoreBackup()**: Restores game directory from backup. Removes patches directory, deletes patch_config.toml, restores original state. Automatically called on installation failure.

### Removal

**PatchRemover.RemoveAllPatches()**: Uninstalls all patches from game. Deletes patch_config.toml, removes patches directory. Optionally restores from most recent backup.

**PatchRemover.HasPatchesInstalled()**: Checks if game has patches installed by testing for patch_config.toml existence.

**PatchRemover.GetInstallationInfo()**: Reads patch_config.toml and returns information about installed patches.

## .kpatch Archive Format

.kpatch files are ZIP archives containing:

- **manifest.toml**: Patch metadata and dependencies
- **hooks.toml** or **{version}.hooks.toml**: Hook definitions (version-specific naming supported)
- **binaries/windows_x86.dll**: Compiled patch DLL (required for DETOUR hooks)
- **additional/**: Optional files (documentation, configuration)

Multiple hooks files supported for version-specific patches. Files with target_versions metadata array loaded only for matching game SHAs.

## Hook Types

**DETOUR**: Full-featured hooks with DLL and parameter extraction. Requires function name, minimum 5 original bytes, optional parameters array. Runtime generates wrapper with state preservation. Applied at runtime by KotorPatcher.dll.

**SIMPLE**: Direct byte replacement in memory. Requires replacement_bytes of same length as original_bytes. No DLL needed. Perfect for changing constants or NOPing instructions. Applied at runtime by KotorPatcher.dll.

**REPLACE**: Allocated code execution in memory. Requires replacement_bytes (any length), minimum 5 original bytes. Writes JMP to allocated memory, executes replacement bytes, JMPs back. No DLL or wrapper needed. Applied at runtime by KotorPatcher.dll.

**STATIC**: Direct byte replacement in executable file. Requires replacement_bytes of same length as original_bytes. No DLL needed. Applied at install-time by StaticHookApplicator before the game runs. Perfect for PE header modifications (4GB patch), import table changes, or any modification that must occur before the executable loads. Uses virtual addresses with automatic conversion to file offsets via PE header parsing.

## Error Handling

All operations return PatchResult or PatchResult&lt;T&gt; instead of throwing exceptions. Expected failures (missing files, version mismatches, validation errors) handled gracefully with descriptive error messages. Installation pipeline automatically restores backup on any failure. Validation errors aggregated and reported with context (patch ID, hook index, parameter number).

## Dependencies

Uses **Tomlyn** library for TOML parsing and serialization. Requires .NET 8.0.

## Technical Constraints

**Platform**: Cross-platform C# but targets Windows games (x86 32-bit).

**Archive Format**: ZIP compression for .kpatch files.

**Hash Algorithm**: SHA-256 for game version detection and verification.

**TOML Format**: Both input (manifest.toml, hooks.toml) and output (patch_config.toml) use TOML.
