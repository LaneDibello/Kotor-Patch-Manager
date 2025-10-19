# Complete Guide to Creating KotOR Patches

This guide covers everything you need to know to create patches for Knights of the Old Republic using the KotOR Patch Manager.

## Table of Contents

- [Understanding Patch Types](#understanding-patch-types)
- [The manifest.toml File](#the-manifesttoml-file)
- [The hooks.toml File](#the-hookstoml-file)
- [SIMPLE Patches](#simple-patches)
- [DETOUR Patches](#detour-patches)
- [Testing and Debugging](#testing-and-debugging)
- [Distribution](#distribution)
- [Best Practices](#best-practices)

---

## Understanding Patch Types

The KotOR Patch Manager supports two types of patches:

### SIMPLE Patches

- **What**: Direct byte-level modifications to the game executable
- **When**: Changing constants, NOPing instructions, modifying jump targets
- **Pros**: No C++ knowledge required, no compilation needed, very lightweight
- **Cons**: Can only replace bytes, cannot add new logic
- **Examples**:
  - Change max party size from 3 to 4
  - Modify movement speed multiplier
  - Change jump distance values
  - Flip conditional jumps
  - NOP out unwanted function calls

### DETOUR Patches

- **What**: Redirects game code to call your custom C++ function
- **When**: Need to add new features, complex logic, or call other game functions
- **Pros**: Full access to game state, can implement complex features
- **Cons**: Requires C++ knowledge, needs compilation, larger file size
- **Examples**:
  - Add debug logging
  - Implement new gameplay mechanics
  - Hook into game events
  - Modify AI behavior dynamically

---

## The manifest.toml File

The manifest describes your patch metadata.

### Minimum Required Fields

```toml
[patch]
id = "my-patch-id"              # Unique identifier (lowercase, hyphens only)
name = "My Patch Name"          # Display name
version = "1.0.0"               # Semantic versioning
author = "Your Name"            # Creator name
description = "What this patch does"

[game]
title = "Star Wars: Knights of the Old Republic"
executable = "swkotor.exe"

[[game.versions]]
name = "4CD"                    # Version identifier
sha256 = "dbd602d92bfbf2bb8467861b4c3f8d260ce0280e831c923e60c77ba1a5d02e2a"
```

### Optional Fields

```toml
[dependencies]
# Patches required before this one can be installed
requires = ["base-patch", "framework-patch"]

[conflicts]
# Patches that cannot be installed with this one
conflicts_with = ["incompatible-patch"]
```

### Getting the Game Hash

Run KPatchConsole to get your game's SHA256 hash:
```
kpatchconsole --status "C:\Path\To\swkotor.exe"
```

Look for the hash in the output and add it to your manifest.

### Patch ID Rules

- Lowercase letters and numbers only
- Use hyphens for spaces
- Must be unique across all patches
- Cannot contain underscores, spaces, or special characters
- Examples:
  - ✅ `my-awesome-patch`
  - ✅ `widescreen-fix-v2`
  - ❌ `My_Patch` (uppercase, underscore)
  - ❌ `patch #1` (space, special char)

---

## The hooks.toml File

The hooks file defines what memory addresses to modify and how.

### SIMPLE Hook Format

```toml
[[hooks]]
address = 0x00401234              # Memory address (hex)
type = "simple"                   # Hook type
original_bytes = [0x6A, 0x64]     # Current bytes (for verification)
replacement_bytes = [0x6A, 0xC8]  # New bytes (must be same length)
```

### DETOUR Hook Format

```toml
[[hooks]]
address = 0x005cb41c
function = "MyHookFunction"       # Function name exported from DLL
type = "detour"
original_bytes = [0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x20]  # Min 5 bytes

# Optional: Extract parameters from game state
[[hooks.parameters]]
source = "eax"                    # Register or stack location
type = "pointer"                  # Data type

[[hooks.parameters]]
source = "[esp+8]"
type = "int"

# Optional: State preservation (default: true)
preserve_registers = true
preserve_flags = true

# Optional: Don't restore specific registers
exclude_from_restore = ["eax", "ecx"]
```

### Finding Addresses and Bytes

Use a disassembler (IDA Pro, Ghidra, x64dbg) to:

1. **Find the address**: Locate the instruction you want to modify
2. **Get original bytes**: Copy the bytes at that address
3. **Verify instruction boundaries**: Never split an instruction

Example using x64dbg:
```
Address: 005CB41C
Bytes: 55 8B EC 83 EC 20
Disassembly:
  55          PUSH EBP
  8B EC       MOV EBP, ESP
  83 EC 20    SUB ESP, 0x20
```

---

## SIMPLE Patches

### Use Cases

#### 1. Change Constant Values

Change a hardcoded number:
```toml
[[hooks]]
# Change max party size from 3 to 4
address = 0x00405000
type = "simple"
original_bytes = [0x6A, 0x03]     # PUSH 3
replacement_bytes = [0x6A, 0x04]  # PUSH 4
```

#### 2. NOP Out Instructions

Disable a function call:
```toml
[[hooks]]
# NOP out a CALL instruction
address = 0x00401000
type = "simple"
original_bytes = [0xE8, 0x10, 0x20, 0x00, 0x00]  # CALL 0x00403015
replacement_bytes = [0x90, 0x90, 0x90, 0x90, 0x90]  # NOP NOP NOP NOP NOP
```

#### 3. Modify Jump Conditions

Change when a jump is taken:
```toml
[[hooks]]
# Change JE to JNE (flip condition)
address = 0x00403000
type = "simple"
original_bytes = [0x74, 0x10]  # JE +16
replacement_bytes = [0x75, 0x10]  # JNE +16
```

#### 4. Change Float Constants

Modify IEEE 754 float values:
```toml
[[hooks]]
# Change alpha from 1.0 to 0.5
address = 0x006a89b2
type = "simple"
original_bytes = [0x68, 0x00, 0x00, 0x80, 0x3F]  # PUSH 1.0f
replacement_bytes = [0x68, 0x00, 0x00, 0x00, 0x3F]  # PUSH 0.5f
```

### SIMPLE Patch Restrictions

- ❌ Cannot change number of bytes (length must match)
- ❌ Cannot call functions
- ❌ Cannot access game state
- ✅ Can replace any complete instruction(s)
- ✅ Any length allowed (as long as original and replacement match)
- ✅ No DLL compilation required

---

## DETOUR Patches

### Writing the C++ Code

#### Basic Function Structure

```cpp
#include <windows.h>

// Function signature must be extern "C" __cdecl
// Name must match hooks.toml
extern "C" void __cdecl MyHookFunction()
{
    OutputDebugStringA("[MyPatch] Hook called!\n");

    // Your patch logic here
}

// Required DLL entry point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            OutputDebugStringA("[MyPatch] DLL loaded\n");
            break;
        case DLL_PROCESS_DETACH:
            OutputDebugStringA("[MyPatch] DLL unloaded\n");
            break;
    }
    return TRUE;
}
```

#### With Parameter Extraction

If your hooks.toml has parameters, they'll be passed to your function:

```cpp
// Parameters are passed in the order defined in hooks.toml
extern "C" void __cdecl MyHookFunction(char* stringParam, int x, int y, float life)
{
    char debugMsg[512];
    sprintf_s(debugMsg, "[MyPatch] string=%s, x=%d, y=%d, life=%.2f\n",
        stringParam, x, y, life);
    OutputDebugStringA(debugMsg);

    // Call original game function
    typedef void (__cdecl *GameFunc)(char*, int, int, float);
    GameFunc originalFunc = (GameFunc)0x0044d490;
    originalFunc(stringParam, x, y, life * 2.0f);  // Double the lifetime
}
```

#### Calling Game Functions

```cpp
// Define function pointer type
typedef int (__cdecl *GetPlayerLevel)(void* playerObj);
const DWORD FUNC_ADDR = 0x00456789;

extern "C" void __cdecl MyHook(void* player)
{
    // Cast address to function pointer
    GetPlayerLevel getLevel = (GetPlayerLevel)FUNC_ADDR;

    // Call the game function
    int level = getLevel(player);

    // Use the result
    char msg[128];
    sprintf_s(msg, "[MyPatch] Player level: %d\n", level);
    OutputDebugStringA(msg);
}
```

### Parameter Types

Available parameter types in hooks.toml:

```toml
[[hooks.parameters]]
source = "eax"        # Or: ebx, ecx, edx, esi, edi, ebp
type = "int"          # Signed 32-bit integer

type = "uint"         # Unsigned 32-bit integer
type = "pointer"      # Memory address (void*)
type = "float"        # 32-bit floating point
type = "byte"         # 8-bit value
type = "short"        # 16-bit value
```

### Stack Parameters

Extract values from the stack:

```toml
[[hooks.parameters]]
source = "[esp+4]"    # First argument on stack
type = "pointer"

[[hooks.parameters]]
source = "[esp+8]"    # Second argument
type = "int"
```

### The exports.def File

If you don't create an exports.def, create-patch.bat will auto-generate one.

Manual exports.def:
```
LIBRARY MyPatchDLL
EXPORTS
    MyHookFunction
    AnotherHookFunction
```

The LIBRARY name is not important - use your patch name.

---

## Testing and Debugging

### 1. Test on a Backup Copy

**ALWAYS** test on a separate copy of the game:

```
C:\Games\KOTOR\          <- Your real game (don't touch!)
C:\Games\KOTOR-Test\     <- Test copy
```

### 2. Use DebugView

Download [DebugView](https://learn.microsoft.com/en-us/sysinternals/downloads/debugview) to see OutputDebugString messages:

1. Run DebugView as Administrator
2. Enable "Capture Global Win32"
3. Launch game with KPatchLauncher.exe
4. Watch for your debug messages

### 3. Verify Original Bytes

Before testing, verify your original_bytes match:

```cpp
// Add this to your DLL for DETOUR patches
void VerifyBytes(DWORD address, const BYTE* expected, size_t length)
{
    BYTE* actual = (BYTE*)address;
    for (size_t i = 0; i < length; i++) {
        if (actual[i] != expected[i]) {
            char msg[256];
            sprintf_s(msg, "[ERROR] Byte mismatch at offset %zu: expected 0x%02X, got 0x%02X\n",
                i, expected[i], actual[i]);
            OutputDebugStringA(msg);
        }
    }
}
```

### 4. Common Issues

#### Game Crashes on Launch

- **Wrong original_bytes**: Verify with disassembler
- **Wrong address**: Double-check hex address format
- **Instruction split**: Make sure original_bytes covers complete instructions

#### Hook Never Fires

- **Address not reached**: Use debugger to verify code path
- **Wrong game version**: Check SHA256 hash matches
- **Missing launcher**: Must use KPatchLauncher.exe, not swkotor.exe

#### DLL Won't Load

- **Missing exports**: Check exports.def matches function names
- **32/64-bit mismatch**: Must compile as 32-bit (x86)
- **Missing dependencies**: Use Dependency Walker to check

---

## Distribution

### Package Structure

Your `.kpatch` file should contain:

```
my-patch.kpatch (ZIP file):
├── manifest.toml
├── hooks.toml
└── binaries/                    # Only for DETOUR patches
    └── windows_x86.dll
```

### Installation

Users install with:
```
kpatchconsole --install "C:\Games\KOTOR\swkotor.exe" "C:\Patches" my-patch-id
```

### Uninstallation

Users uninstall with:
```
kpatchconsole --uninstall "C:\Games\KOTOR\swkotor.exe"
```

This removes ALL patches and restores the backup.

---

## Best Practices

### 1. Document Everything

Add comments to your hooks.toml:
```toml
[[hooks]]
# This hook changes the max party size from 3 to 4
# Original instruction at 0x405000: MOV [EAX+0x10], 3
address = 0x00405000
type = "simple"
# ...
```

### 2. Use Descriptive Names

```cpp
// ❌ Bad
extern "C" void __cdecl Hook1(int a, int b) { }

// ✅ Good
extern "C" void __cdecl IncreaseMaxPartySize(int currentSize, int maxAllowed) { }
```

### 3. Validate Inputs

```cpp
extern "C" void __cdecl MyHook(void* obj)
{
    if (!obj) {
        OutputDebugStringA("[MyPatch] ERROR: null object\n");
        return;
    }

    // Use obj safely
}
```

### 4. Version Your Patches

Use semantic versioning in manifest.toml:
- `1.0.0` - Initial release
- `1.0.1` - Bug fix
- `1.1.0` - New feature (compatible)
- `2.0.0` - Breaking change

### 5. Test Compatibility

Test your patch with:
- Fresh game install
- Existing save games
- Other popular patches
- Different game versions (if targeting multiple)

### 6. Handle Errors Gracefully

```cpp
extern "C" void __cdecl MyHook(void* param)
{
    __try {
        // Your code
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        OutputDebugStringA("[MyPatch] Exception caught\n");
    }
}
```

### 7. Use Meaningful Patch IDs

- ✅ `widescreen-fix`
- ✅ `improved-ai-v2`
- ✅ `debug-console`
- ❌ `patch1`
- ❌ `mymod`
- ❌ `test`

---

## Advanced Topics

### Multiple Hooks in One Patch

You can have multiple hooks in one hooks.toml:

```toml
[[hooks]]
address = 0x00401000
type = "simple"
# ...

[[hooks]]
address = 0x00402000
type = "detour"
function = "Hook1"
# ...

[[hooks]]
address = 0x00403000
type = "detour"
function = "Hook2"
# ...
```

### Mixing SIMPLE and DETOUR

One .kpatch can have both types:

```toml
[[hooks]]
# SIMPLE: Change a constant
address = 0x00401000
type = "simple"
original_bytes = [0x6A, 0x64]
replacement_bytes = [0x6A, 0xC8]

[[hooks]]
# DETOUR: Complex logic
address = 0x00402000
type = "detour"
function = "MyComplexHook"
original_bytes = [0x55, 0x8B, 0xEC, 0x83, 0xEC]
```

### Multiple .cpp Files

All .cpp files in your patch folder will be compiled into one DLL:

```
MyPatch/
├── main.cpp        # Main hooks
├── helpers.cpp     # Helper functions
├── utils.cpp       # Utility functions
└── hooks.toml      # References functions from any .cpp
```

---

## Getting Help

- Check the examples in `examples/`
- Review existing patches in `Patches/`
- Read the technical docs in `KotorPatcher/docs/`
- Test with DebugView to see what's happening

**Happy patching!**
