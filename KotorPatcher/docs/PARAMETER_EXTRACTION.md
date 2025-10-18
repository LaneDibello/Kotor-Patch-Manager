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
2. **Restores ESP to original value** (so we can read from original stack)
3. **Reads each parameter from specified source** (register or stack offset)
4. **Pushes parameters in __cdecl order** (right-to-left)
5. Calls the patch function with proper parameters
6. Cleans up parameters
7. Restores wrapper ESP
8. Restores registers (POPFD/POPAD)
9. Executes stolen bytes
10. Jumps back to original code

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
PUSHAD                    ; Save all registers
PUSHFD                    ; Save flags

; Restore ESP to read parameters
MOV EBX, ESP              ; Save wrapper ESP
ADD ESP, 36               ; Restore original ESP (32 + 4)

; Extract parameters in reverse order (for __cdecl push)
MOV ECX, [ESP+8]          ; Read param4 (life) from [ESP+8]
PUSH ECX                  ; Push param4

MOV ECX, [ESP+4]          ; Read param3 (y) from [ESP+4]
PUSH ECX                  ; Push param3

MOV ECX, [ESP]            ; Read param2 (x) from [ESP]
PUSH ECX                  ; Push param2

; EAX was saved by PUSHAD, retrieve it
MOV ECX, [EBX+28]         ; Read saved EAX from PUSHAD state
PUSH ECX                  ; Push param1 (string)

; Call patch function
CALL patch_function       ; __cdecl: callee cleans stack... wait, no!

; Clean up parameters (caller cleans for __cdecl)
ADD ESP, 16               ; 4 params * 4 bytes

; Restore wrapper state
MOV ESP, EBX              ; Restore wrapper ESP
POPFD                     ; Restore flags
POPAD                     ; Restore registers

; Execute stolen bytes and return
LEA ECX, [ESP+0xc]
MOV DWORD PTR [ESP+0x1c], -1
JMP 0x005cb428
```

## Backward Compatibility

If no `[[hooks.parameters]]` are specified, the wrapper works as before - patch function takes no parameters. This maintains compatibility with existing patches that don't use parameter extraction.

## Future Enhancements

- Support for return value modification
- Support for struct/array parameters
- Support for 64-bit parameters (long long, double)
- Automatic parameter detection based on calling convention analysis
