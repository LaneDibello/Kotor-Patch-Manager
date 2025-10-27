# Launch Process Analysis & Recommendations

## Current Process Flow

### How Patches Are Applied (PatchApplicator.cs)

1. **Extract patch DLLs** → `GameDir/patches/*.dll`
2. **Generate patch_config.toml** → `GameDir/patch_config.toml`
3. **Copy KotorPatcher.dll** → `GameDir/KotorPatcher.dll`
4. **Copy launcher files** → `GameDir/*` (6 files)

### How Game Is Launched (ProcessInjector.cs + MainViewModel.cs)

1. **Start game suspended** using CreateProcess
2. **Inject KotorPatcher.dll** using LoadLibraryA via remote thread
3. **Resume game** - KotorPatcher reads patch_config.toml and applies patches
4. **Game runs** with patches active

## The Problems

### Problem 1: Unnecessary File Copies

**Current behavior**: Copies 6 launcher files to game directory:
```
- KPatchLauncher.exe
- KPatchLauncher.dll
- KPatchLauncher.runtimeconfig.json
- KPatchLauncher.deps.json
- KPatchCore.dll
- Tomlyn.dll
```

**Why this is wrong**:
- **KPatchLauncher.exe is already running** - we don't need another copy
- **Launcher files are only needed at install time**, not runtime
- **ProcessInjector only needs KotorPatcher.dll** - it injects that directly
- **Game never loads the launcher files** - only KotorPatcher.dll

**Root cause**: Lines 367-413 in PatchApplicator.cs copy launcher unnecessarily

### Problem 2: Self-Contained EXE Doesn't Have Separate DLLs

When published as self-contained single-file:
- All DLLs are embedded in KPatchLauncher.exe
- No separate KPatchLauncher.dll, KPatchCore.dll, etc.
- Can't copy files that don't exist!

### Problem 3: No Result Validation

**Current behavior**: PatchApplicator returns success based only on:
- Files copied
- Config generated
- No exceptions thrown

**Missing validation**:
- Does patch_config.toml actually work?
- Can KotorPatcher.dll actually load?
- Are patch DLLs valid?
- Will injection succeed?

### Problem 4: Debug Symbols in Release

Release builds include .pdb files which should not be distributed.

## Recommended Solutions

### Solution 1: Eliminate Unnecessary Copies

**What really needs to be in game directory**:
```
GameDir/
├── swkotor.exe                  (original game)
├── KotorPatcher.dll             (✓ REQUIRED - injected at launch)
├── patch_config.toml            (✓ REQUIRED - read by KotorPatcher)
└── patches/
    ├── patch1.dll               (✓ REQUIRED - loaded by KotorPatcher)
    └── patch2.dll
```

**What does NOT need to be there**:
```
❌ KPatchLauncher.exe            (already running from its own location)
❌ KPatchLauncher.dll            (embedded in single-file exe)
❌ KPatchLauncher.runtimeconfig.json
❌ KPatchLauncher.deps.json
❌ KPatchCore.dll                (embedded in single-file exe)
❌ Tomlyn.dll                    (embedded in single-file exe)
```

**Changes needed**:
1. Remove launcher copy logic from PatchApplicator.cs (lines 367-413)
2. Keep only KotorPatcher.dll copy (lines 354-365)
3. Update UI to not reference launcher files in game directory

### Solution 2: Fix Self-Contained Build

**For development builds** (multi-file, Debug mode):
- Keep current behavior for testing
- Allows debugging with symbols

**For release builds** (single-file, Release mode):
- Configure to exclude .pdb files
- All .NET code embedded in one exe
- Only KotorPatcher.dll remains separate (it's C++, not .NET)

**Project settings to add**:
```xml
<PropertyGroup Condition="'$(Configuration)' == 'Release'">
    <DebugType>none</DebugType>
    <DebugSymbols>false</DebugSymbols>
</PropertyGroup>
```

### Solution 3: Add Result Validation

**Validate after applying patches**:

```csharp
// After copying files, validate installation
var validationResult = ValidatePatchInstallation(gameDir);
if (!validationResult.Success)
{
    // Restore backup
    // Return failure
}
```

**Validation checks**:
1. ✅ patch_config.toml exists and is valid TOML
2. ✅ KotorPatcher.dll exists and is valid PE file
3. ✅ All referenced patch DLLs exist
4. ✅ All patch DLLs have required exports
5. ✅ Config addresses don't overlap

### Solution 4: Launcher Location Strategy

**Current problem**: Launcher copies itself to game dir, launches from there

**Better approach**: Launch from anywhere

```csharp
// MainViewModel.cs - LaunchGame()
var gameDir = Path.GetDirectoryName(GameExePath);
var patcherDllPath = Path.Combine(gameDir, "KotorPatcher.dll");

// Launch game with injection FROM ANYWHERE
var result = ProcessInjector.LaunchWithInjection(
    GameExePath,
    patcherDllPath  // ← Uses DLL in game dir, not launcher dir
);
```

**No need to**:
- Copy launcher to game directory
- Run from game directory
- Pollute game installation

## Implementation Plan

### Phase 1: Remove Unnecessary Copies (High Priority)

1. **Update PatchApplicator.cs**:
   - Remove `LauncherExePath` parameter
   - Remove `CopyLauncher` parameter
   - Remove lines 367-413 (launcher copy logic)
   - Keep only KotorPatcher.dll copy

2. **Update MainViewModel.cs**:
   - Remove LauncherExePath from InstallOptions
   - Remove CopyLauncher setting
   - Ensure LaunchGame uses game dir for KotorPatcher.dll

3. **Benefits**:
   - Clean game directory
   - No duplicate launcher files
   - Works with self-contained builds
   - Simpler codebase

### Phase 2: Fix Release Build (High Priority)

1. **Update KPatchLauncher.csproj**:
   ```xml
   <PropertyGroup Condition="'$(Configuration)' == 'Release'">
       <DebugType>none</DebugType>
       <DebugSymbols>false</DebugSymbols>
   </PropertyGroup>
   ```

2. **Update publish scripts**:
   - Add cleanup step to remove .pdb files
   - Verify only .exe is output

### Phase 3: Add Validation (Medium Priority)

1. **Create InstallationValidator class**:
   - ValidatePatchConfig(configPath)
   - ValidatePatcherDll(dllPath)
   - ValidatePatchDlls(patchesDir, config)
   - ValidateExports(dllPath, expectedExports)

2. **Update PatchApplicator**:
   - Call validation before returning success
   - Provide detailed error messages
   - Restore backup on validation failure

3. **Update UI**:
   - Show validation progress
   - Display specific validation failures

### Phase 4: Improve Error Reporting (Low Priority)

1. **Structured results**:
   ```csharp
   public class ValidationError
   {
       public string Component { get; set; }  // "patch_config", "dll", etc.
       public string Issue { get; set; }
       public string Suggestion { get; set; }
   }
   ```

2. **Better user feedback**:
   - "KotorPatcher.dll is missing from game directory"
   - "patch_config.toml has invalid syntax at line 5"
   - "Patch DLL 'foo.dll' is missing export 'MyFunction'"

## File Size Comparison

**Current approach** (copying everything):
```
Game Directory:
├── swkotor.exe (original)
├── KotorPatcher.dll          ~100 KB
├── patch_config.toml         ~1 KB
├── patches/*.dll             ~50 KB each
├── KPatchLauncher.exe        ~60 MB  ❌ NOT NEEDED
├── KPatchLauncher.dll        (embedded)
├── KPatchCore.dll            (embedded)
└── ... (3 more JSON files)
                              ──────────
Total added:                  ~60-70 MB
```

**Recommended approach** (minimal):
```
Game Directory:
├── swkotor.exe (original)
├── KotorPatcher.dll          ~100 KB
├── patch_config.toml         ~1 KB
└── patches/*.dll             ~50 KB each
                              ──────────
Total added:                  ~150 KB + patches
```

**Savings**: 99.75% reduction in files added to game directory!

## Testing Checklist

After implementing these changes:

- [ ] Apply patches from launcher
- [ ] Verify only KotorPatcher.dll copied to game dir
- [ ] Verify patch_config.toml generated correctly
- [ ] Verify launcher NOT copied to game dir
- [ ] Launch game from launcher
- [ ] Verify patches apply successfully
- [ ] Verify validation catches missing DLLs
- [ ] Verify validation catches corrupt config
- [ ] Test release build has no .pdb files
- [ ] Test self-contained exe works standalone

## Migration Notes

**For users with existing installations**:
- Old launcher files in game directory will remain
- Can be manually deleted (not harmful, just unused)
- Consider adding cleanup step on next patch apply

**Backward compatibility**:
- Old patch installations still work
- Just have extra files that aren't used
