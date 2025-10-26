# Parameter Extraction System for INLINE Hooks

## Problem Statement

When using INLINE hooks, the compiler-generated function prologue (`SUB ESP, n`) allocates local variables before the patch code executes. This clobbers any parameters that were on the stack at the hook point, making them inaccessible.

### Example Issue

If we hook at a point where:
- `EAX` = parameter 1
- `[ESP]` = parameter 2
- `[ESP+4]` = parameter 3

And the patch function does:
```cpp
void MyPatch() {
    int param2;
    __asm {
        mov eax, [esp]  // Try to read param2
        mov param2, eax
    }
}
```

The compiled code looks like:
```asm
SUB ESP, 0x10        ; Compiler allocates locals - ESP moved!
MOV EAX, [ESP]       ; Now reading wrong memory!
```

## Solution: Parameter Extraction System

Allow patch authors to declare parameters in `hooks.toml`, and have the wrapper extract them and pass them as normal function parameters.

### Design

#### 1. Parameter Definition (hooks.toml)

```toml
[[hooks]]
address = 0x005cb41c
function = "EnableAurPostString_Hook"
original_bytes = [0x8d, 0x4c, 0x24, 0x0c, 0xc7]
stolen_bytes = [0x8d, 0x4c, 0x24, 0x0c, 0xc7, 0x44, 0x24, 0x1c, 0xff, 0xff, 0xff, 0xff]
type = "inline"

# Parameter extraction configuration
[[hooks.parameters]]
source = "eax"        # Read from EAX register
type = "pointer"      # char*

[[hooks.parameters]]
source = "esp+0"      # Read from [ESP]
type = "int"          # int

[[hooks.parameters]]
source = "esp+4"      # Read from [ESP+4]
type = "int"          # int

[[hooks.parameters]]
source = "esp+8"      # Read from [ESP+8]
type = "float"        # float
```

#### 2. Patch Function Signature

Patch author writes a clean function:

```cpp
extern "C" void __cdecl EnableAurPostString_Hook(char* string, int x, int y, float life)
{
    // Parameters are passed normally - no inline assembly needed!
    AurPostStringFunc aurPostString = (AurPostStringFunc)AUR_POST_STRING_ADDR;
    aurPostString(string, y, x, life);
}
```

#### 3. Wrapper Behavior

The INLINE wrapper generator:

1. Saves all registers (PUSHAD/PUSHFD)
2. **Saves ESP to EBX** (points to saved state - NOT modified!)
3. **Reads each parameter from specified source** (register or stack offset, using adjusted offsets)
4. **Pushes parameters in __cdecl order** (right-to-left)
5. Calls the patch function with proper parameters
6. Cleans up parameters (caller cleanup for __cdecl)
7. Restores ESP from EBX
8. Restores registers (POPFD/POPAD)
9. Executes stolen bytes
10. Jumps back to original code

**Critical Implementation Detail**: ESP is NOT restored before extracting parameters. Instead, ESP remains pointing at the saved state, and parameters are read using adjusted offsets (`savedStateSize + 4 + userOffset`). This prevents PUSH instructions from clobbering saved registers.

### Parameter Source Syntax

- **Registers**: `"eax"`, `"ebx"`, `"ecx"`, `"edx"`, `"esi"`, `"edi"`, `"ebp"`, `"esp"`
- **Stack offsets**: `"esp+0"`, `"esp+4"`, `"esp+8"`, etc.
- **Dereferenced pointers**: `"[eax]"`, `"[esp+12]"`, etc.

### Parameter Types

- `"int"` - 32-bit integer (DWORD)
- `"uint"` - Unsigned 32-bit integer (DWORD)
- `"pointer"` - 32-bit pointer (void*)
- `"float"` - 32-bit floating point
- `"byte"` - 8-bit value
- `"short"` - 16-bit value

All parameters are pushed as 4 bytes on the stack (x86 __cdecl convention).

## Benefits

1. **Clean patch code**: No inline assembly to read parameters
2. **Type safety**: Compiler checks parameter types
3. **Maintainable**: Easy to understand and modify
4. **Flexible**: Works for any calling convention at hook point
5. **Debuggable**: Standard C functions are easier to debug

## Implementation Phases

### Phase 1: C# Models and Parsing
- Add `Parameter` model class
- Update `Hook` class to include `List<Parameter>`
- Update `HooksParser` to parse parameter arrays
- Update `ConfigGenerator` to serialize parameters

### Phase 2: C++ Runtime Parsing
- Add `ParameterInfo` struct to patcher.h
- Update `config_reader.cpp` to parse parameters
- Add parameters to `PatchInfo` and `WrapperConfig`

### Phase 3: C++ Wrapper Generation
- Update `GenerateInlineWrapper` to extract parameters
- Generate code to read from registers/stack
- Generate code to push parameters in __cdecl order
- Adjust stack cleanup based on parameter count

### Phase 4: Testing
- Update EnableAurPostString patch to use parameters
- Test in-game functionality
- Verify stack alignment and parameter values

## Example Generated Assembly

For the AurPostString hook with 4 parameters, the wrapper generates:

```asm
; Save state
PUSHAD                    ; Save all registers (32 bytes)
PUSHFD                    ; Save flags (4 bytes)

; Setup for parameter extraction
MOV EBX, ESP              ; Save ESP (points to saved state)
                          ; ESP is NOT modified - stays at saved state!

; Extract parameters in reverse order (for __cdecl push)
; Stack parameters use adjusted offsets: savedStateSize + 4 + userOffset

; Parameter 4: life from esp+8
; Actual offset: 36 (savedStateSize) + 4 (return addr) + 8 = 48
MOV ECX, [ESP+48]
PUSH ECX

; Parameter 3: y from esp+4
; Actual offset: 36 + 4 + 4 = 44
MOV ECX, [ESP+44]
PUSH ECX

; Parameter 2: x from esp+0
; Actual offset: 36 + 4 + 0 = 40
MOV ECX, [ESP+40]
PUSH ECX

; Parameter 1: string from EAX
; Read from PUSHAD saved state at [EBX+32]
MOV ECX, [EBX+32]         ; OFFSET_EAX = 32
PUSH ECX

; Call patch function
CALL patch_function       ; __cdecl: caller cleans stack

; Clean up parameters (caller cleanup for __cdecl)
ADD ESP, 16               ; 4 params * 4 bytes

; Restore wrapper state
MOV ESP, EBX              ; Restore ESP to saved state
POPFD                     ; Restore flags
POPAD                     ; Restore all registers

; Execute stolen bytes and return
LEA ECX, [ESP+0xc]
MOV DWORD PTR [ESP+0x1c], -1
JMP 0x005cb428
```

## Backward Compatibility

If no `[[hooks.parameters]]` are specified, the wrapper works as before - patch function takes no parameters. This maintains compatibility with existing patches that don't use parameter extraction.

