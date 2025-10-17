# EnableScriptAurPostString Patch

## Overview

This patch re-enables the debug `AurPostString` script function in KOTOR 1 that was disabled for release. This function is called throughout the game scripts to display on-screen debug text.

## Technical Details

### Hook Point
- **Address**: `0x005cb41c` in `ExecuteCommandAurPostString`
- **Location**: Right after the C-string conversion, before cleanup
- **Type**: Inline hook with wrapper system

### What It Does

The game's NWScript VM has a function `AurPostString(string, int, int, float)` that is meant to display debug messages on screen. The function is fully implemented in the game (`0x0044d490`) but the script command handler was neutered - it extracts all the parameters from the VM stack but never actually calls the display function.

This patch:
1. Intercepts at the point where all parameters have been extracted
2. Calls the actual `AurPostString` display function at `0x0044d490`
3. Returns control to continue with normal cleanup

### Parameters

At the hook point (`0x005cb41c`):
- `EAX` = `char*` string (the text to display)
- `[ESP]` = `int` x (screen X coordinate)
- `[ESP+4]` = `int` y (screen Y coordinate)
- `[ESP+8]` = `float` life (how long to display in seconds)

These are reorganized and passed to:
```cpp
void __cdecl AurPostString(char *string, int y, int x, float life)
```

## Files

- `aurpoststring_patch.cpp` - The patch DLL source code
- `manifest.toml` - Patch metadata and supported versions
- `hooks.toml` - Hook configuration
- `build.bat` - Build script for compiling the DLL
- `BUILD.md` - Detailed build instructions

## Building

### Quick Build
1. Double-click `build.bat`
2. Wait for compilation
3. `windows_x86.dll` will be created

### Manual Build
See `BUILD.md` for detailed instructions.

## Packaging into .kpatch

After building the DLL, create the .kpatch file:

```
EnableScriptAurPostString.kpatch (ZIP file)
├── manifest.toml
├── hooks.toml
└── binaries/
    └── windows_x86.dll
```

## Installation

Use the KPatch Console application:
```cmd
kpatch --install "C:\path\to\swkotor.exe" "C:\path\to\patches" enable-script-aurpoststring
```

## Testing

After installation, launch KOTOR 1. Many game scripts call `AurPostString` for debugging:
- Character spawn notifications
- Quest state changes
- Area transitions
- Combat events

You should see debug text appearing on screen during gameplay.

## Supported Versions

- KOTOR 1 GOG v1.03 (Windows x86)
  - Hash: `9C10E0450A6EECA417E036E3CDE7474FED1F0A92AAB018446D156944DEA91435`

## Notes

- This patch has no dependencies
- No known conflicts with other patches
- Safe to install/uninstall at any time
- The original game files are backed up automatically

## Assembly Analysis

Original code at `0x005cb41c`:
```asm
005cb41c  8d 4c 24 0c    LEA ECX, [ESP+0xc]      ; Prepare for destructor
005cb420  c7 44 24 1c    MOV [ESP+0x1c], -1      ; Exception handler cleanup
          ff ff ff ff
005cb428  e8 f3 a7 01 00 CALL CExoString::~CExoString
```

Our hook replaces the first 5 bytes with a JMP to our wrapper, which:
1. Saves all registers
2. Calls `EnableAurPostString_Hook()`
3. Restores all registers
4. Executes the original `LEA ECX, [ESP+0xc]` instruction
5. Jumps back to `0x005cb421` to continue
