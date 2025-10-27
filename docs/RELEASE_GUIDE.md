# Release Guide

This guide explains how to create and publish releases of KotOR Patch Manager.

## Release Types

### 1. Full Release (with example patches)
**Script**: `publish-release.bat`

**Contains**:
- KPatchLauncher.exe (single self-contained executable)
- create-patch.bat (for users to create patches)
- Example patches (.kpatch files) - optional
- README.txt

**Size**: ~60-80 MB (with .NET runtime embedded)

**Use when**: Creating official releases for end users

### 2. Minimal Release (launcher + tools only)
**Script**: `publish-minimal.bat`

**Contains**:
- KPatchLauncher.exe
- create-patch.bat
- README.txt

**Size**: ~60-70 MB

**Use when**: Users will download patches separately

## Publishing Process

### Prerequisites

1. **Visual Studio 2022** (for building C++ patches)
2. **.NET 8.0 SDK** installed
3. All changes committed to git
4. Version number decided (follow SemVer)

### Step-by-Step: Full Release

1. **Update version numbers**:
   ```bash
   # Edit src/KPatchLauncher/KPatchLauncher.csproj
   <Version>0.2.0</Version>
   ```

2. **Run the publish script**:
   ```cmd
   publish-release.bat
   ```

3. **When prompted**:
   - Enter version (e.g., `0.1.0-alpha`)
   - Choose whether to include example patches (Y/N)

4. **Output**:
   - `releases/KotorPatchManager-v0.1.0-alpha/` (directory)
   - `releases/KotorPatchManager-v0.1.0-alpha.zip` (archive for distribution)

### Step-by-Step: Minimal Release

1. **Run the script**:
   ```cmd
   publish-minimal.bat
   ```

2. **Enter version** when prompted

3. **Output**:
   - `releases/KotorPatchManager-v0.1.0-alpha-minimal.zip`

## What Gets Published

### KPatchLauncher.exe

**Publishing settings** (from .csproj):
```xml
<PublishSingleFile>true</PublishSingleFile>
<SelfContained>true</SelfContained>
<RuntimeIdentifier>win-x86</RuntimeIdentifier>
<IncludeNativeLibrariesForSelfExtract>true</IncludeNativeLibrariesForSelfExtract>
<PublishReadyToRun>true</PublishReadyToRun>
```

This creates a **single .exe file** that:
- Contains the entire .NET 8 runtime (no installation required)
- Includes all Avalonia UI libraries
- Is optimized for startup performance
- Works on Windows 7+ (both x86 and x64)
- Is approximately 60-70 MB

**Why single-file?**
- Users only need to download one EXE
- No "missing DLL" errors
- Cleaner distribution
- Professional appearance

### create-patch.bat

The script for users to create their own patches. Includes:
- Auto-detection of SIMPLE vs DETOUR patches
- Visual Studio environment setup
- DLL compilation
- .kpatch packaging
- Export validation

### Example Patches (Optional)

Pre-built .kpatch files demonstrating the system:
- **EnableScriptAurPostString** - DETOUR hook with parameters
- **SemiTransparentLetterbox** - SIMPLE byte replacement
- **ScriptExtender** - Complex example with multiple functions

## Release Checklist

Before publishing:

- [ ] All tests passing
- [ ] Build succeeds in Release mode
- [ ] Version number updated in .csproj
- [ ] CHANGELOG.md updated with changes
- [ ] README.md accurate for current version
- [ ] No debug code or OutputDebugString in release
- [ ] Example patches compile and work
- [ ] Test on clean machine (no .NET installed)

## Publishing to GitHub

### Creating a GitHub Release

1. **Create git tag**:
   ```bash
   git tag -a v0.1.0-alpha -m "First alpha release"
   git push origin v0.1.0-alpha
   ```

2. **Go to GitHub**:
   - Navigate to Releases
   - Click "Draft a new release"
   - Choose the tag you just created

3. **Fill in release info**:
   - **Title**: `v0.1.0-alpha - First Alpha Release`
   - **Description**: Copy from CHANGELOG.md, include:
     - What's new
     - Known issues
     - Installation instructions
     - System requirements

4. **Upload assets**:
   - `KotorPatchManager-v0.1.0-alpha.zip` (main release)
   - `KotorPatchManager-v0.1.0-alpha-minimal.zip` (optional)
   - Individual .kpatch files (optional)

5. **Mark as pre-release** if alpha/beta

6. **Publish release**

### Release Notes Template

```markdown
## KotOR Patch Manager v0.1.0-alpha

First alpha release of the KotOR Patch Manager!

### Features
- Patch management UI with live preview
- Support for DETOUR and SIMPLE patch types
- DLL injection-based launcher
- Example patches included

### What's Included
- `KPatchLauncher.exe` - Main application (60 MB, self-contained)
- `create-patch.bat` - Patch creation tool
- 3 example patches (.kpatch files)

### System Requirements
- Windows 7 or later (x86 or x64)
- KOTOR 1 (GOG or Steam version)
- No .NET installation required!

### Known Issues
- [List any known bugs or limitations]

### Installation
1. Download `KotorPatchManager-v0.1.0-alpha.zip`
2. Extract anywhere
3. Run `KPatchLauncher.exe`
4. Point to your KOTOR installation
5. Apply patches and launch!

### Creating Patches
Use `create-patch.bat` to build your own patches. See documentation for details.
```

## Version Numbering

Follow [Semantic Versioning](https://semver.org/):

- **0.1.0-alpha**: First alpha release
- **0.2.0-alpha**: Second alpha with new features
- **0.9.0-beta**: Beta release (feature-complete)
- **1.0.0**: First stable release
- **1.1.0**: Minor update with new features
- **1.0.1**: Patch/bugfix release

## File Size Optimization

Current setup produces:
- **KPatchLauncher.exe**: ~60-70 MB (self-contained)
- **.kpatch files**: 10-100 KB each
- **Total release**: ~60-80 MB

### Why so large?

The .exe includes:
- .NET 8 runtime (~40 MB)
- Avalonia UI framework (~15 MB)
- Application code (~5 MB)

### Can it be smaller?

**Options**:
1. **Framework-dependent** (requires .NET 8 installed):
   - Size: ~5 MB
   - Tradeoff: Users must install .NET 8
   - Not recommended for ease of use

2. **Enable trimming** (not recommended for Avalonia):
   - Can reduce by 20-30%
   - May break at runtime
   - Not worth the risk

3. **Separate .NET runtime** download:
   - Launcher: ~5 MB
   - Runtime: ~55 MB (one-time download)
   - Added complexity

**Recommendation**: Stick with self-contained single-file. 60 MB is acceptable in 2024, and it provides the best user experience.

## Testing the Release

Before publishing:

1. **Extract to clean directory**
2. **Run KPatchLauncher.exe** (no VS or .NET SDK installed)
3. **Test all features**:
   - Browse for game
   - Browse for patches
   - Apply patches
   - Uninstall patches
   - Launch game
4. **Test create-patch.bat**:
   - Create a SIMPLE patch
   - Create a DETOUR patch
   - Verify .kpatch files work

## Troubleshooting

### "The publish failed" error
- Check .NET 8 SDK is installed
- Ensure no files are locked
- Try cleaning: `dotnet clean`

### Executable too large
- This is expected for self-contained
- See "File Size Optimization" above

### Missing dependencies in published exe
- Check `<IncludeNativeLibrariesForSelfExtract>true</IncludeNativeLibrariesForSelfExtract>`
- Verify Avalonia packages are referenced

### create-patch.bat doesn't work
- Ensure Visual Studio 2022 installed
- Check path to vcvars32.bat
- Verify C++ build tools installed

## Future Improvements

- [ ] Auto-update system
- [ ] Crash reporting / telemetry
- [ ] Digitally sign the executable
- [ ] MSI installer option
- [ ] Portable vs installed modes
- [ ] Delta updates (only download changes)
