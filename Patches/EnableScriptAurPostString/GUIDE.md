# Complete Guide: Building and Testing EnableScriptAurPostString Patch

## Prerequisites

1. Visual Studio 2022 with C++ build tools installed
2. KPatchConsole compiled and ready
3. KOTOR 1 GOG version installed at `C:\Users\laned\Documents\KotOR Installs\`

## Step 1: Build the Patch DLL

1. Open File Explorer and navigate to:
   ```
   C:\Users\laned\source\Repos\KotOR Patch Manager\Patches\EnableScriptAurPostString
   ```

2. Double-click `build.bat`

3. You should see:
   ```
   Building EnableScriptAurPostString Patch DLL...

   [Visual Studio environment setup...]

   Build successful!
   Output: windows_x86.dll

   Verifying exports...
   EnableAurPostString_Hook
   ```

4. Verify that `windows_x86.dll` now exists in the directory

**Troubleshooting:**
- If Visual Studio path is wrong, edit `build.bat` line 6 to match your installation
- Make sure you have the "Desktop development with C++" workload installed in VS

## Step 2: Package the .kpatch File

1. In the same directory, double-click `package.bat`

2. You should see:
   ```
   Packaging EnableScriptAurPostString.kpatch...

   Copying files...
   Creating .kpatch archive...

   SUCCESS! Created EnableScriptAurPostString.kpatch

   File contents:
     manifest.toml
     hooks.toml
     binaries/windows_x86.dll
   ```

3. Verify that `EnableScriptAurPostString.kpatch` now exists

## Step 3: Create a Patches Directory

1. Create a directory to hold your patches:
   ```
   C:\Users\laned\Documents\KotOR Patches
   ```

2. Copy `EnableScriptAurPostString.kpatch` to this directory

## Step 4: Test Listing the Patch

1. Open Command Prompt or PowerShell

2. Navigate to KPatchConsole:
   ```cmd
   cd "C:\Users\laned\source\Repos\KotOR Patch Manager\KPatchConsole\KPatchConsole"
   ```

3. List available patches:
   ```cmd
   dotnet run -- --list "C:\Users\laned\Documents\KotOR Patches"
   ```

4. You should see:
   ```
   === Available Patches in C:\Users\laned\Documents\KotOR Patches ===

   Found 1 patch(es):

   [enable-script-aurpoststring]
     Name:        Enable Script AurPostString
     Version:     1.0.0
     Author:      KotOR Patch Manager
     Description: Re-enables the debug AurPostString script function...
     Hooks:       1
     Versions:    1 supported
   ```

## Step 5: Check Current Game Status

Before installing, check the game's patch status:

```cmd
dotnet run -- --status "C:\Users\laned\Documents\KotOR Installs\swkotor.exe"
```

Should show:
```
=== Patch Status for swkotor.exe ===

Patched:        NO
Backup:         NO
Loader Injected: NO
Config File:     NO
```

## Step 6: Install the Patch

**IMPORTANT: This will modify your game executable. Make sure you have a backup!**

```cmd
dotnet run -- --install "C:\Users\laned\Documents\KotOR Installs\swkotor.exe" "C:\Users\laned\Documents\KotOR Patches" enable-script-aurpoststring
```

You should see detailed progress:
```
=== Installing Patches to swkotor.exe ===

Game:    C:\Users\laned\Documents\KotOR Installs\swkotor.exe
Patches: C:\Users\laned\Documents\KotOR Patches
IDs:     enable-script-aurpoststring

Step 1/7: Validating inputs...
Step 2/7: Detecting game version...
  Detected: KOTOR Unknown (Other, Windows, x86)
Step 3/7: Loading and validating patches...
  Validating patch: enable-script-aurpoststring
  ✓ All validations passed
Step 4/7: Creating backup...
  Backup created: swkotor.exe.backup.20241017_...
Step 5/7: Extracting patch DLLs...
  Extracted: enable-script-aurpoststring -> patches/enable-script-aurpoststring.dll
Step 6/7: Generating patch_config.toml...
  Generated config with 1 patches, 1 hooks
Step 7/7: Injecting loader DLL (experimental)...
  ⚠️ Loader injection is experimental...

=== Installation Complete ===
Game Version: KOTOR Unknown (Other, Windows, x86)
Installed Patches (1):
  - enable-script-aurpoststring

Backup: swkotor.exe.backup.20241017_...
Config: patch_config.toml
```

## Step 7: Verify Installation

Check status again:

```cmd
dotnet run -- --status "C:\Users\laned\Documents\KotOR Installs\swkotor.exe"
```

Should now show:
```
=== Patch Status for swkotor.exe ===

Patched:        YES
Backup:         YES
Backup Path:    swkotor.exe.backup.20241017_...
Backup Date:    2024-10-17 ...
Loader Injected: YES/NO (depending on if experimental injection worked)
Config File:     YES

Installed Patches (1):
  - enable-script-aurpoststring

Patch DLLs (1):
  - enable-script-aurpoststring.dll
```

## Step 8: Test In-Game

**CRITICAL: The kotor_patcher.dll runtime loader needs to be present!**

Before launching the game, you need:
1. `kotor_patcher.dll` in the game directory (the C++ runtime from the KotorPatcher project)
2. This DLL loads at game startup and applies the hooks

**To test:**
1. Launch KOTOR 1
2. Load a save game or start a new game
3. Look for debug text appearing on screen - many scripts call `AurPostString`:
   - Character dialogue
   - Quest updates
   - Area transitions
   - Combat events

## Step 9: Uninstalling (if needed)

To remove the patch:

```cmd
dotnet run -- --uninstall "C:\Users\laned\Documents\KotOR Installs\swkotor.exe"
```

This will:
1. Restore the backup
2. Remove all patch files
3. Clean up the installation

## Troubleshooting

### Patch Won't List
- Check that the .kpatch file is in the correct directory
- Verify the ZIP file structure is correct (use 7-Zip to inspect)

### Installation Fails
- Check that you have write permissions to the game directory
- Verify the game executable hash matches (run --status first)

### No Debug Text Appears In-Game
- Make sure `kotor_patcher.dll` exists in the game directory
- Check that the DLL is actually loading (loader injection may have failed)
- Check Windows Event Viewer for DLL load errors

### Game Crashes
- Uninstall the patch immediately
- Check that original_bytes in hooks.toml match the actual game executable
- Verify the hook address is correct for your game version

## Expected Behavior

When working correctly, you should see debug text appearing on screen like:
- "Area: tar_m02aa loaded"
- "Character: n_twilek spawned"
- "Quest state: tat_swoop = 10"

The text will appear at coordinates (x, y) specified by the script and stay on screen for the specified lifetime.

## Next Steps

Once this patch works, you can:
1. Create more patches
2. Test multiple patches together
3. Verify dependency resolution
4. Test conflict detection
