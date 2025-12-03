# Patch Options Design Document

**Status:** Design Phase
**Created:** 2025-12-03
**Purpose:** Enable user-configurable parameters for patches at installation time

---

## Overview

This document outlines the design for adding configurable options to patches. Currently, patches have fixed behavior (e.g., LevelUpLimit always sets max level to 50). This feature would allow users to customize patch behavior within reasonable bounds (e.g., set max level to any value between 21-100).

### Core Principle

**Options are resolved at installation time by KPatchCore, not at runtime by KotorPatcher.**

- For SIMPLE and REPLACE hooks: All byte values are resolved to literals before writing `patch_config.toml`
- For DETOUR hooks: Option values are passed to C++ DLLs via an `[options]` table in `patch_config.toml`

---

## Use Case: LevelUpLimit Patch

Currently, LevelUpLimit hardcodes the max level as 50 (0x32) throughout:

```toml
# Current hooks.toml
[[hooks]]
address = 0x004f1a3e
type = "simple"
original_bytes = [0xb8, 0x14, 0x00, 0x00, 0x00]
replacement_bytes = [0xb8, 0x32, 0x00, 0x00, 0x00]  # Hardcoded 50
```

```cpp
// Current Reallocations.cpp
int* requiredExpPerLevel = new int[0x33];  // Hardcoded 51
```

**With options**, users could choose max level 40, 60, 80, etc. within safe bounds.

---

## Architecture

### Installation Flow

```
1. User selects patch in launcher
2. Launcher reads manifest.toml (includes [[patch.options]] definitions)
3. UI displays option fields with defaults/min/max
4. User configures options (or accepts defaults)
5. User clicks "Install"
   ↓
6. KPatchCore reads hooks.toml
7. Detects template strings like "{{max_level}}"
8. Resolves templates using user-selected values
9. Generates patch_config.toml:
   - SIMPLE/REPLACE hooks: Literal byte arrays only
   - DETOUR hooks: Adds [options] table with values
10. Copies files to game directory
```

### Runtime Flow

```
1. Game launches (via KPatchLauncher)
2. KotorPatcher.dll injected
3. Loads patch_config.toml
4. For each DETOUR patch:
   - Loads patch DLL
   - Patch DLL reads options from [options] table (if needed)
   - DLL uses options to allocate memory, set constants, etc.
5. Applies all hooks with resolved values
6. Game continues with patches active
```

---

## Manifest Schema Extension

Add a new `[[patch.options]]` array to `manifest.toml`:

```toml
[patch]
id = "level-up-limit"
name = "Max Level Increase"
version = "2.0.0"
author = "Arniel_Stanislaus"
description = "Sets the maximum level in KotOR 1"

requires = []
conflicts = []

[patch.supported_versions]
kotor1_gog_103 = "9C10E0450A6EECA417E036E3CDE7474FED1F0A92AAB018446D156944DEA91435"
kotor1_steam_103 = "34E6D971C034222A417995D8E1E8FDD9F8781795C9C289BD86C499A439F34C88"

# NEW: Option definitions
[[patch.options]]
id = "max_level"
type = "integer"
default = 50
min = 21
max = 100
display_name = "Maximum Level"
description = "The maximum character level (must be at least 21)"

# Future: Computed options (derived from other options)
[[patch.options]]
id = "max_level_inclusive"
type = "computed"
expression = "max_level + 1"
description = "Internal value for inclusive comparisons"
```

### Option Types (Phase 1 - MVP)

**`integer`**
- Fields: `id`, `display_name`, `description`, `default`, `min`, `max`
- Used for: Levels, counts, array sizes, numeric constants
- Validation: Must be within [min, max] range

### Option Types (Phase 2 - Future)

**`float`**
- Fields: `id`, `display_name`, `description`, `default`, `min`, `max`, `step`
- Used for: Alpha values, multipliers, timers
- Stored as IEEE 754 little-endian bytes

**`boolean`**
- Fields: `id`, `display_name`, `description`, `default`
- Used for: Enable/disable sub-features
- Maps to 0x00 (false) or 0x01 (true)

**`enum`**
- Fields: `id`, `display_name`, `description`, `default`, `choices`
- Example: `choices = [{value = 0x01, label = "Easy"}, {value = 0x02, label = "Hard"}]`
- Used for: Difficulty modes, behavior variants

**`computed`**
- Fields: `id`, `expression`, `description`
- Derived from other options at install time
- Example: `expression = "max_level + 1"` for inclusive comparisons

---

## Hooks Template Syntax

### For SIMPLE and REPLACE Hooks

Use **string templates** in byte arrays:

```toml
[[hooks]]
address = 0x004f1a3e
type = "simple"
original_bytes = [0xb8, 0x14, 0x00, 0x00, 0x00]
replacement_bytes = [0xb8, "{{max_level}}", 0x00, 0x00, 0x00]

[[hooks]]
address = 0x005a54aa
type = "simple"
original_bytes = [0x83, 0xf8, 0x14]
replacement_bytes = [0x83, 0xf8, "{{max_level}}"]
```

**Parsing Rules:**
- Template format: `"{{option_id}}"` (double curly braces)
- Can appear anywhere in `replacement_bytes` array
- At install time, KPatchCore replaces with actual byte value
- Validates result is 0x00-0xFF (single byte)
- Multi-byte values require multiple templates or computed options

**Example Multi-byte (16-bit value):**
```toml
# For a 16-bit little-endian value
[[patch.options]]
id = "buffer_size"
type = "integer"
default = 512
min = 256
max = 4096

[[patch.options]]
id = "buffer_size_low"
type = "computed"
expression = "buffer_size & 0xFF"

[[patch.options]]
id = "buffer_size_high"
type = "computed"
expression = "(buffer_size >> 8) & 0xFF"

[[hooks]]
replacement_bytes = [0x68, "{{buffer_size_low}}", "{{buffer_size_high}}", 0x00, 0x00]
```

### For DETOUR Hooks

No templates in hooks - C++ code reads from `[options]` table:

```toml
# hooks.toml - no changes needed
[[hooks]]
address = 0x00552c9a
type = "detour"
original_bytes = [0x8d, 0x7e, 0x38, 0xb9, 0x15, 0x00, 0x00, 0x00, 0xf3, 0xab]
function = "InitRequiredExpPerLevel"
skip_original_bytes = true
```

```cpp
// Reallocations.cpp - reads options at runtime
static int g_maxLevel = 50;  // default
static int g_maxLevelInclusive = 51;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        // Read options from environment variables set by KotorPatcher
        const char* maxLevelStr = getenv("PATCH_LEVELUPLIMIT_MAX_LEVEL");
        if (maxLevelStr) {
            g_maxLevel = atoi(maxLevelStr);
            g_maxLevelInclusive = g_maxLevel + 1;
            debugLog("[LevelUpLimit] Using max level: %d", g_maxLevel);
        }
    }
    return TRUE;
}

extern "C" void __cdecl InitRequiredExpPerLevel(void* rules)
{
    debugLog("[LevelUpLimit] Running InitRequiredExpPerLevel");
    int* requiredExpPerLevel = new int[g_maxLevelInclusive];  // Uses option value
    setObjectProperty<int*>(rules, 0x38, requiredExpPerLevel);
}
```

---

## Generated patch_config.toml Format

### Example with Options

```toml
target_version_sha = "9C10E0450A6EECA417E036E3CDE7474FED1F0A92AAB018446D156944DEA91435"

# Options table - ONLY for DETOUR hooks
[options.level-up-limit]
max_level = 60
max_level_inclusive = 61

[[patches]]
id = "level-up-limit"
dll = "patches/LevelUpLimit/Reallocations.dll"

[[patches.hooks]]
address = 0x004f1a3e
type = "simple"
original_bytes = [0xb8, 0x14, 0x00, 0x00, 0x00]
replacement_bytes = [0xb8, 0x3c, 0x00, 0x00, 0x00]  # Resolved: 0x3c = 60

[[patches.hooks]]
address = 0x00552c9a
type = "detour"
original_bytes = [0x8d, 0x7e, 0x38, 0xb9, 0x15, 0x00, 0x00, 0x00, 0xf3, 0xab]
function = "InitRequiredExpPerLevel"
skip_original_bytes = true
# DLL reads options.level-up-limit.max_level at runtime
```

### Options Table Structure

```toml
[options.{patch-id}]
{option-id} = {resolved-value}
{option-id} = {resolved-value}

[options.{another-patch-id}]
{option-id} = {resolved-value}
```

**Key Points:**
- Only present if at least one patch has DETOUR hooks and options
- Nested by patch ID to avoid conflicts
- Contains ALL options (including computed ones)
- C++ patches read this via KotorPatcher or environment variables

---

## Implementation Phases

### Phase 1: MVP (Integer Options Only)

**C# Changes (KPatchCore):**
1. Add `PatchOption` model class:
   ```csharp
   public class PatchOption
   {
       public required string Id { get; init; }
       public required string Type { get; init; }  // "integer" for MVP
       public required string DisplayName { get; init; }
       public required string Description { get; init; }
       public int Default { get; init; }
       public int? Min { get; init; }
       public int? Max { get; init; }
   }
   ```

2. Extend `PatchManifest.cs`:
   ```csharp
   public List<PatchOption> Options { get; init; } = new();
   ```

3. Update `ManifestParser.cs`:
   - Parse `[[patch.options]]` array from TOML
   - Validate option definitions (min <= default <= max, etc.)

4. Update `HooksParser.cs`:
   - Detect template strings in `replacement_bytes`
   - Track which bytes need resolution
   - Validate template references valid option IDs

5. Update `ConfigGenerator.cs`:
   - Accept `Dictionary<string, Dictionary<string, int>> resolvedOptions` parameter
   - Replace template strings with resolved byte values
   - Generate `[options.{patch-id}]` table for DETOUR patches
   - Validate resolved bytes fit in 0x00-0xFF

6. Add `OptionResolver.cs`:
   ```csharp
   public static class OptionResolver
   {
       public static PatchResult ResolveOptions(
           PatchManifest manifest,
           Dictionary<string, int> userValues,
           out Dictionary<string, int> resolved)
       {
           // Validate user values against constraints
           // Compute derived options
           // Return resolved option set
       }
   }
   ```

**C# Changes (KPatchLauncher UI):**
1. Add option input controls to patch details panel
2. Validate user input against min/max
3. Store selections per patch
4. Pass to PatchOrchestrator during installation

**C++ Changes (KotorPatcher):**
1. Update `config_reader.cpp`:
   - Parse `[options]` table if present
   - Store in global map: `std::map<std::string, std::map<std::string, std::string>>`

2. Update `patcher.cpp`:
   - Before loading each patch DLL, set environment variables:
     ```cpp
     for (auto& [optionId, value] : patchOptions) {
         std::string envVar = "PATCH_" + ToUpper(patchId) + "_" + ToUpper(optionId);
         SetEnvironmentVariableA(envVar.c_str(), value.c_str());
     }
     ```

**Documentation:**
1. Update patch creation guide with option examples
2. Document template syntax
3. Add LevelUpLimit as reference implementation

### Phase 2: Extended Types (Future)

1. Add `float`, `boolean`, `enum` option types
2. Implement `computed` options with expression evaluator
3. Add option validation warnings (e.g., "Values above 80 may cause instability")
4. Support option presets (e.g., "Recommended", "Extreme")
5. Add UI tooltips and help text for complex options

---

## Key Files to Modify

### C# (KPatchCore)

| File | Changes |
|------|---------|
| `Models/PatchManifest.cs` | Add `List<PatchOption> Options` property |
| `Models/PatchOption.cs` | **NEW** - Model for option definitions |
| `Parsers/ManifestParser.cs` | Parse `[[patch.options]]` from TOML |
| `Parsers/HooksParser.cs` | Detect template strings in byte arrays |
| `Applicators/ConfigGenerator.cs` | Resolve templates, generate `[options]` table |
| `Applicators/OptionResolver.cs` | **NEW** - Validate and resolve user option values |
| `Validators/OptionValidator.cs` | **NEW** - Validate option definitions and values |

### C# (KPatchLauncher)

| File | Changes |
|------|---------|
| `ViewModels/PatchDetailViewModel.cs` | Add options collection and user input properties |
| `Views/PatchDetailView.axaml` | Add option input controls (sliders, text boxes) |
| `ViewModels/MainViewModel.cs` | Pass resolved options to PatchOrchestrator |

### C++ (KotorPatcher)

| File | Changes |
|------|---------|
| `include/config_reader.h` | Add options map to API |
| `src/config_reader.cpp` | Parse `[options]` table |
| `include/patcher.h` | Add `std::map<std::string, std::map<string, string>> g_patchOptions` |
| `src/patcher.cpp` | Set environment variables before loading DLLs |

### Patch Updates

| Patch | Changes |
|-------|---------|
| `LevelUpLimit/manifest.toml` | Add `[[patch.options]]` for `max_level` |
| `LevelUpLimit/hooks.toml` | Replace hardcoded 0x32 with `"{{max_level}}"` |
| `LevelUpLimit/Reallocations.cpp` | Read `PATCH_LEVELUPLIMIT_MAX_LEVEL` env var |

---

## Alternative Approaches Considered

### ❌ Runtime Template Resolution

**Idea:** KotorPatcher resolves templates at runtime instead of install time.

**Rejected because:**
- Adds significant complexity to C++ code
- Harder to debug (no way to inspect final bytes)
- Violates principle that `patch_config.toml` should have literals
- Makes validation harder

### ❌ Multiple Hook File Variants

**Idea:** Generate separate hooks files for each option combination.

```
hooks_level50.toml
hooks_level60.toml
hooks_level80.toml
```

**Rejected because:**
- Exponential file explosion with multiple options
- Maintenance nightmare (change applies to all variants)
- Doesn't scale

### ❌ Full Expression Language

**Idea:** Add scripting language for complex byte calculations.

```toml
replacement_bytes = ["0xb8", "max_level & 0xFF", "(max_level >> 8) & 0xFF"]
```

**Rejected because:**
- Over-engineered for current needs
- Security concerns (arbitrary code in patches)
- Could add in Phase 2 as "computed" options if needed

---

## Validation & Safety

### Option Value Validation

1. **At Definition (manifest parsing):**
   - `min <= default <= max`
   - `type` is valid
   - `id` is unique within patch

2. **At User Input (launcher UI):**
   - Value is within [min, max]
   - Value matches type constraints
   - Real-time feedback

3. **At Installation (KPatchCore):**
   - All required options have values
   - Resolved bytes fit in target size (0x00-0xFF for single byte)
   - Templates reference valid option IDs
   - Final byte arrays match expected lengths

### Runtime Safety

1. **Memory Allocation:**
   - C++ patches must validate option values before allocating arrays
   - Add bounds checking in critical sections
   - Log actual values used for debugging

2. **Game Compatibility:**
   - Document safe ranges in option descriptions
   - Warn users about experimental values
   - Consider adding "untested" flag for extreme values

---

## Testing Strategy

### Unit Tests (C#)

1. `OptionResolverTests`:
   - Valid integer options resolve correctly
   - Out-of-range values rejected
   - Computed options calculate properly
   - Template string detection and replacement

2. `ConfigGeneratorTests`:
   - Generated TOML has literal bytes
   - Options table present for DETOUR patches
   - Options table absent for SIMPLE-only patches

3. `ManifestParserTests`:
   - Valid option definitions parse correctly
   - Invalid definitions rejected with clear errors

### Integration Tests

1. **LevelUpLimit with options:**
   - Install with default (50)
   - Install with min (21)
   - Install with max (100)
   - Launch game and verify level cap works

2. **Multiple patches with options:**
   - Install patches with overlapping option names
   - Verify isolation (level-up-limit.max_level vs other-patch.max_level)

3. **Edge cases:**
   - Patch with no options (ensure backwards compatible)
   - Patch with only computed options
   - Invalid template references

### Manual Testing

1. UI displays options correctly with defaults
2. Sliders respect min/max bounds
3. Values persist across launcher restarts
4. Reinstalling patch with different values works
5. Game behavior matches selected options

---

## Backwards Compatibility

### For Existing Patches

Patches without `[[patch.options]]` continue to work unchanged:
- No templates in hooks = no resolution needed
- No `[options]` table in generated config
- Zero code changes required

### For Existing Installations

When implementing this feature:
1. Old `patch_config.toml` files (without options) still load correctly
2. Patches installed before options support continue working
3. User must reinstall patches to configure options (expected behavior)

---

## Future Enhancements

### Phase 3+

1. **Option Presets:**
   ```toml
   [[patch.presets]]
   id = "vanilla-plus"
   name = "Vanilla Plus"
   max_level = 30

   [[patch.presets]]
   id = "extreme"
   name = "Extreme"
   max_level = 100
   ```

2. **Option Dependencies:**
   ```toml
   [[patch.options]]
   id = "enable_feature_x"
   type = "boolean"

   [[patch.options]]
   id = "feature_x_strength"
   type = "integer"
   depends_on = "enable_feature_x == true"  # Only shown if enabled
   ```

3. **Validation Rules:**
   ```toml
   [[patch.options]]
   id = "max_level"
   warning_if = "max_level > 80"
   warning_message = "Values above 80 may cause UI issues"
   ```

4. **Import/Export Configurations:**
   - Save option configurations as profiles
   - Share configurations between users
   - Include in saved games metadata

---

## Questions for Implementation

When implementing this feature, consider:

1. **UI Design:** Should options use sliders, text boxes, or both?
2. **Storage:** Store user option values in launcher config or per-installation?
3. **Defaults:** Should defaults come from manifest or be overridable globally?
4. **Validation:** Should launcher validate game compatibility before allowing extreme values?
5. **Environment Variables:** Is this the best way to pass options to C++ DLLs, or should we parse the TOML directly in each DLL?

---

## Example: Complete LevelUpLimit with Options

### manifest.toml
```toml
[patch]
id = "level-up-limit"
name = "Max Level Increase"
version = "3.0.0"
author = "Arniel_Stanislaus"
description = "Configurable maximum level for KotOR 1"

[[patch.options]]
id = "max_level"
type = "integer"
default = 50
min = 21
max = 100
display_name = "Maximum Level"
description = "The maximum character level. Values above 80 may have UI issues."

[[patch.options]]
id = "max_level_inclusive"
type = "computed"
expression = "max_level + 1"

[patch.supported_versions]
kotor1_gog_103 = "9C10E0450A6EECA417E036E3CDE7474FED1F0A92AAB018446D156944DEA91435"
```

### hooks.toml (excerpt)
```toml
# SIMPLE hooks use templates
[[hooks]]
address = 0x004f1a3e
type = "simple"
original_bytes = [0xb8, 0x14, 0x00, 0x00, 0x00]
replacement_bytes = [0xb8, "{{max_level}}", 0x00, 0x00, 0x00]

[[hooks]]
address = 0x0054fe50
type = "simple"
original_bytes = [0x83, 0xf8, 0x14]
replacement_bytes = [0x83, 0xf8, "{{max_level}}"]

# DETOUR hooks stay the same
[[hooks]]
address = 0x00552c9a
type = "detour"
original_bytes = [0x8d, 0x7e, 0x38, 0xb9, 0x15, 0x00, 0x00, 0x00, 0xf3, 0xab]
function = "InitRequiredExpPerLevel"
skip_original_bytes = true
```

### Reallocations.cpp (excerpt)
```cpp
#include "Common.h"
#include <cstdlib>

// Global configuration (loaded at DLL_PROCESS_ATTACH)
static int g_maxLevel = 50;
static int g_maxLevelInclusive = 51;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        // Read configuration from environment
        const char* maxLevelStr = getenv("PATCH_LEVELUPLIMIT_MAX_LEVEL");
        if (maxLevelStr) {
            g_maxLevel = atoi(maxLevelStr);
            g_maxLevelInclusive = g_maxLevel + 1;
            debugLog("[LevelUpLimit] Using max_level=%d from options", g_maxLevel);
        } else {
            debugLog("[LevelUpLimit] Using default max_level=%d", g_maxLevel);
        }
    }
    return TRUE;
}

extern "C" void __cdecl InitRequiredExpPerLevel(void* rules)
{
    debugLog("[LevelUpLimit] Allocating array for %d levels", g_maxLevelInclusive);
    int* requiredExpPerLevel = new int[g_maxLevelInclusive];
    setObjectProperty<int*>(rules, 0x38, requiredExpPerLevel);
}

extern "C" void __cdecl InitNumSpellLevels(void* thisClass)
{
    debugLog("[LevelUpLimit] Allocating spell levels for %d", g_maxLevel);
    setObjectProperty<BYTE*>(thisClass, 0x114, new BYTE[g_maxLevel]);
}

// ... similar changes for other init functions
```

### Generated patch_config.toml (user selected max_level=60)
```toml
target_version_sha = "9C10E0450A6EECA417E036E3CDE7474FED1F0A92AAB018446D156944DEA91435"

[options.level-up-limit]
max_level = 60
max_level_inclusive = 61

[[patches]]
id = "level-up-limit"
dll = "patches/LevelUpLimit/Reallocations.dll"

[[patches.hooks]]
address = 0x004f1a3e
type = "simple"
original_bytes = [0xb8, 0x14, 0x00, 0x00, 0x00]
replacement_bytes = [0xb8, 0x3c, 0x00, 0x00, 0x00]  # 0x3c = 60

[[patches.hooks]]
address = 0x0054fe50
type = "simple"
original_bytes = [0x83, 0xf8, 0x14]
replacement_bytes = [0x83, 0xf8, 0x3c]  # 0x3c = 60

[[patches.hooks]]
address = 0x00552c9a
type = "detour"
original_bytes = [0x8d, 0x7e, 0x38, 0xb9, 0x15, 0x00, 0x00, 0x00, 0xf3, 0xab]
function = "InitRequiredExpPerLevel"
skip_original_bytes = true
```

---

## Conclusion

This design provides a clean, maintainable approach to patch options that:

1. Resolves values at install time (simpler, more debuggable)
2. Maintains backwards compatibility
3. Works with all hook types (SIMPLE, REPLACE, DETOUR)
4. Separates concerns appropriately (C# for resolution, C++ for execution)
5. Scales to future enhancements

The template string approach for byte arrays works around TOML's type system while keeping the implementation straightforward. The environment variable approach for C++ DLLs is simple and robust.

When ready to implement, start with Phase 1 (integer options only) and use LevelUpLimit as the reference implementation.
