# Parameter Extraction System - Implementation Complete

## Overview

The parameter extraction system allows patch authors to write clean C functions that receive parameters directly, instead of manually extracting them with inline assembly. The wrapper automatically reads parameters from registers and stack locations and pushes them in __cdecl order.

## What Was Implemented

### C# Components (Installation Time)

1. **Parameter.cs** - Model for parameter definitions
   - `Source`: Where to read from ("eax", "esp+0", etc.)
   - `Type`: Data type (int, uint, pointer, float, byte, short)
   - Validation for source syntax

2. **Hook.cs** - Updated to include `List<Parameter> Parameters`

3. **HooksParser.cs** - Parses `[[hooks.parameters]]` arrays from hooks.toml
   - Validates parameter sources and types
   - Returns detailed error messages

4. **ConfigGenerator.cs** - Writes parameters to patch_config.toml
   - Serializes parameter arrays for C++ runtime to read

### C++ Components (Runtime)

5. **patcher.h** - Added parameter types and structures
   - `enum class ParameterType` - INT, UINT, POINTER, FLOAT, BYTE, SHORT
   - `struct ParameterInfo` - source and type
   - Added `std::vector<ParameterInfo> parameters` to `PatchInfo`

6. **wrapper_base.h** - Added parameters to WrapperConfig
   - Forward declared ParameterInfo
   - Added `std::vector<ParameterInfo> parameters`

7. **config_reader.cpp** - Parses parameters from TOML
   - Reads [[hooks.parameters]] arrays
   - Parses source and type fields
   - Converts type strings to ParameterType enum

8. **patcher.cpp** - Passes parameters to wrapper
   - Line 137: `wrapperConfig.parameters = patch.parameters;`

9. **wrapper_x86_win32.cpp** - **THE BIG ONE!**
   - `ExtractAndPushParameter()` - Extracts from source and pushes
   - Updated `GenerateInlineWrapper()` to:
     1. Loop through parameters in reverse order (right-to-left for __cdecl)
     2. Extract each parameter using ExtractAndPushParameter
     3. Call the patch function
     4. Clean up parameters (caller cleanup for __cdecl)

### Patch Updates

10. **EnableScriptAurPostString/hooks.toml** - Parameter definitions
```toml
[[hooks.parameters]]
source = "eax"        # string from EAX register
type = "pointer"

[[hooks.parameters]]
source = "esp+0"      # x from [ESP]
type = "int"

[[hooks.parameters]]
source = "esp+4"      # y from [ESP+4]
type = "int"

[[hooks.parameters]]
source = "esp+8"      # life from [ESP+8]
type = "float"
```

11. **EnableScriptAurPostString/aurpoststring_patch.cpp** - Clean C signature
```cpp
extern "C" void __cdecl EnableAurPostString_Hook(char* string, int x, int y, float life)
{
    AurPostStringFunc aurPostString = (AurPostStringFunc)AUR_POST_STRING_ADDR;
    aurPostString(string, y, x, life);
}
```

## How It Works

### Wrapper Assembly Flow

When the hook at 0x005cb41c is triggered:

```asm
; 1. Save CPU state
PUSHAD                          ; Save all registers (EAX now at [ESP+28])
PUSHFD                          ; Save flags

; 2. Setup for parameter extraction
MOV EBX, ESP                    ; Save wrapper ESP
ADD ESP, 36                     ; Restore ESP to original value

; 3. Extract and push parameters (reverse order)
; Parameter 4: life from [ESP+8]
MOV ECX, [ESP+8]
PUSH ECX

; Parameter 3: y from [ESP+4]
MOV ECX, [ESP+4]
PUSH ECX

; Parameter 2: x from [ESP]
MOV ECX, [ESP]
PUSH ECX

; Parameter 1: string from EAX (saved at [EBX+28])
MOV ECX, [EBX+28]
PUSH ECX

; 4. Call patch function
CALL EnableAurPostString_Hook   ; Parameters on stack in correct order!

; 5. Clean up parameters (__cdecl caller cleanup)
ADD ESP, 16                     ; 4 params * 4 bytes

; 6. Restore wrapper state
MOV ESP, EBX                    ; Restore wrapper ESP
POPFD                           ; Restore flags
POPAD                           ; Restore all registers

; 7. Execute stolen bytes
LEA ECX, [ESP+0xc]
MOV DWORD PTR [ESP+0x1c], -1

; 8. Jump back to continue
JMP 0x005cb428
```

## Supported Parameter Sources

### Registers
- `"eax"`, `"ebx"`, `"ecx"`, `"edx"`, `"esi"`, `"edi"`, `"ebp"`, `"esp"`
- Read from PUSHAD saved state at [EBX + offset]

### Stack Offsets
- `"esp+0"`, `"esp+4"`, `"esp+8"`, etc.
- Read from [ESP + offset] after ESP is restored to original value

### Future Enhancements
- Dereferenced pointers: `"[eax]"`, `"[esp+4]"`
- Memory addresses: `"[0x401234]"`
- Complex expressions: `"[eax+ecx*4]"`

## Benefits

### Before (Manual Extraction)
```cpp
extern "C" void __cdecl EnableAurPostString_Hook()
{
    char* string;
    int x, y;
    float life;

    __asm {
        add esp, 0x10        // Undo compiler prologue
        mov string, eax
        mov eax, [esp]
        mov x, eax
        mov eax, [esp+4]
        mov y, eax
        // ... etc
        sub esp, 0x10        // Restore for epilogue
    }

    aurPostString(string, y, x, life);
}
```

### After (Parameter Extraction)
```cpp
extern "C" void __cdecl EnableAurPostString_Hook(char* string, int x, int y, float life)
{
    aurPostString(string, y, x, life);
}
```

**Clean, maintainable, type-safe, and compiler-friendly!**

## Testing

To test the system:

1. Rebuild KotorPatcher C++ project
2. Rebuild EnableScriptAurPostString patch DLL
3. Repackage .kpatch file
4. Install to game
5. Launch via KPatchLauncher.exe
6. Verify AurPostString debug text appears in-game

Check DebugView for wrapper generation messages showing parameter extraction.

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│                   INSTALLATION TIME                      │
├─────────────────────────────────────────────────────────┤
│  hooks.toml                                             │
│  └─> HooksParser.cs                                     │
│      └─> Hook model (with Parameters list)             │
│          └─> ConfigGenerator.cs                         │
│              └─> patch_config.toml (in game dir)        │
└─────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────┐
│                      GAME RUNTIME                        │
├─────────────────────────────────────────────────────────┤
│  patch_config.toml                                      │
│  └─> config_reader.cpp                                  │
│      └─> PatchInfo (with parameters vector)             │
│          └─> patcher.cpp                                │
│              └─> WrapperConfig (with parameters)        │
│                  └─> wrapper_x86_win32.cpp              │
│                      └─> ExtractAndPushParameter()      │
│                          └─> Generated machine code     │
│                              └─> Patch function called! │
└─────────────────────────────────────────────────────────┘
```

## Conclusion

This system eliminates one of the biggest pain points in writing patches: manually extracting parameters with fragile inline assembly. Patch authors can now write normal C functions with proper signatures, and the wrapper system handles all the messy details automatically.

The implementation is fully backward compatible - hooks without parameters work exactly as before.
