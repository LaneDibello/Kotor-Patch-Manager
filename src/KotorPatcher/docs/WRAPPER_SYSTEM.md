# Wrapper System Implementation Guide

## Overview

The KOTOR Patcher now includes a comprehensive wrapper system that automatically manages CPU state for patches. This makes writing patches significantly easier and safer.

## What Changed

### Before (Simple Trampolines)
- Direct JMP to patch function
- Patch must manually save/restore registers
- Error-prone assembly required
- No inspection of original state

### After (Wrapper System)
- Automatic state management
- Full CPU context passed to patch
- Write simple C code
- Selective register modification
- Platform-extensible design

## Architecture

### Core Components

1. **wrapper_context.h**
   - Defines `PatchContext` structure
   - Contains helper methods for state access
   - Platform-independent interface

2. **wrapper_base.h**
   - Abstract `WrapperGeneratorBase` interface
   - `WrapperConfig` for hook configuration
   - Factory function for platform selection

3. **wrapper_x86_win32.cpp**
   - x86 32-bit Windows implementation
   - Runtime code generation
   - Selective register restoration

4. **Updated patcher.cpp**
   - Wrapper generator initialization
   - Hook type dispatch (INLINE/REPLACE/WRAP)
   - Wrapper-aware trampoline writing

5. **Updated config_reader.cpp**
   - Parses `type` field
   - Parses `preserve_registers` / `preserve_flags`
   - Parses `exclude_from_restore` arrays

## Usage Guide

### For Patch Authors

#### 1. Simple Inspection Hook

**Goal**: Just log information, don't modify anything

**Patch Code**:
```cpp
extern "C" __declspec(dllexport)
void LogFunction(PatchContext* ctx) {
    char msg[128];
    sprintf_s(msg, "Called with EAX=0x%08X\n", ctx->eax);
    OutputDebugStringA(msg);
}
```

**Config**:
```toml
[[patches.hooks]]
address = 0x401234
function = "LogFunction"
original_bytes = [0x55, 0x8B, 0xEC]
type = "inline"  # Default, can omit
```

#### 2. Modify Return Value

**Goal**: Change what a function returns

**Patch Code**:
```cpp
extern "C" __declspec(dllexport)
void ForceReturn42(PatchContext* ctx) {
    ctx->SetReturnValue(42);
}
```

**Config**:
```toml
[[patches.hooks]]
address = 0x401234
function = "ForceReturn42"
original_bytes = [0x55, 0x8B, 0xEC]
type = "inline"
exclude_from_restore = ["eax"]  # Allow EAX modification
```

#### 3. Modify Multiple Registers

**Goal**: Set output parameters via registers

**Patch Code**:
```cpp
extern "C" __declspec(dllexport)
void SetOutputs(PatchContext* ctx) {
    ctx->eax = 100;  // Primary return
    ctx->edx = 200;  // Secondary return
}
```

**Config**:
```toml
[[patches.hooks]]
address = 0x401234
function = "SetOutputs"
original_bytes = [0x55, 0x8B, 0xEC]
type = "inline"
exclude_from_restore = ["eax", "edx"]
```

#### 4. Conditional Logic

**Goal**: Modify behavior based on inputs

**Patch Code**:
```cpp
extern "C" __declspec(dllexport)
void ClampValue(PatchContext* ctx) {
    DWORD param = ctx->GetParameter(0);

    if (param > 1000) {
        ctx->SetReturnValue(1000);
    }
}
```

**Config**:
```toml
[[patches.hooks]]
address = 0x401234
function = "ClampValue"
original_bytes = [0x55, 0x8B, 0xEC]
type = "inline"
exclude_from_restore = ["eax"]
```

#### 5. Legacy Assembly (REPLACE type)

**Goal**: Full control for experts

**Patch Code**:
```cpp
extern "C" __declspec(dllexport) __declspec(naked)
void AssemblyPatch() {
    __asm {
        push ebp
        mov ebp, esp
        // ... your code ...
        pop ebp
        ret
    }
}
```

**Config**:
```toml
[[patches.hooks]]
address = 0x401234
function = "AssemblyPatch"
original_bytes = [0x55, 0x8B, 0xEC]
type = "replace"  # No wrapper, direct JMP
```

### For Patcher Developers

#### Adding a New Platform

To support a new platform (e.g., x64 Windows):

1. **Create header**: `include/wrappers/wrapper_x86_64_win64.h`
2. **Implement class**: Inherit from `WrapperGeneratorBase`
3. **Implement codegen**: Override `GenerateWrapper()`
4. **Update factory**: Modify `GetWrapperGenerator()` for conditional compilation

```cpp
// wrapper_x86_64_win64.h
class WrapperGenerator_x86_64_Win64 : public WrapperGeneratorBase {
public:
    void* GenerateWrapper(const WrapperConfig& config) override {
        // Generate x64 assembly
        // Use RCX, RDX, R8, R9 for parameters (MS x64 calling convention)
        // Save/restore XMM registers if needed
        // ...
    }

    const char* GetPlatformName() const override {
        return "x86_64_Win64";
    }
};
```

```cpp
// wrapper_base.cpp
WrapperGeneratorBase* GetWrapperGenerator() {
#if defined(_M_IX86)
    static WrapperGenerator_x86_Win32 generator;
    return &generator;
#elif defined(_M_X64)
    static WrapperGenerator_x86_64_Win64 generator;
    return &generator;
#else
    #error "Unsupported platform"
#endif
}
```

## Technical Details

### Wrapper Generation (INLINE type)

The `GenerateInlineWrapper()` function creates x86 machine code at runtime:

**Generated Assembly**:
```asm
wrapper_stub:
    pushad              ; 0x60 - Save 8 registers
    pushfd              ; 0x9C - Save EFLAGS

    ; Build context
    push dword [original_function]  ; 0x68 + addr
    push dword [return_address]     ; Calculate from stack
    push dword [original_esp]       ; Calculate

    ; Call patch
    lea eax, [esp]      ; 0x8D 0x84 0x24 + offset
    push eax            ; 0x50
    call patch_function ; 0xE8 + rel32
    add esp, 4          ; 0x83 0xC4 0x04

    ; Cleanup context
    add esp, 12         ; 0x83 0xC4 0x0C

    ; Restore state
    popfd               ; 0x9D
    popad (or selective pops)
    ret                 ; 0xC3
```

**Selective Register Restoration**:

If `exclude_from_restore = ["eax", "edx"]`:
```asm
; Instead of POPAD (0x61):
pop edi     ; 0x5F - Restore
pop esi     ; 0x5E - Restore
pop ebp     ; 0x5D - Restore
add esp, 4  ; 0x83 0xC4 0x04 - Skip ESP
pop ebx     ; 0x5B - Restore
add esp, 4  ; Skip EDX (excluded)
pop ecx     ; 0x59 - Restore
add esp, 4  ; Skip EAX (excluded)
```

### Memory Layout

**Stack during patch execution**:
```
[ESP+0]  -> PatchContext.eax
[ESP+4]  -> PatchContext.ecx
[ESP+8]  -> PatchContext.edx
[ESP+12] -> PatchContext.ebx
[ESP+16] -> PatchContext.esp_at_pushad
[ESP+20] -> PatchContext.ebp
[ESP+24] -> PatchContext.esi
[ESP+28] -> PatchContext.edi
[ESP+32] -> PatchContext.eflags
[ESP+36] -> PatchContext.original_esp
[ESP+40] -> PatchContext.return_address
[ESP+44] -> PatchContext.original_function
```

### Performance Considerations

**Wrapper Overhead** (INLINE type):
- **Setup**: ~15 instructions (PUSHAD, PUSHFD, context build)
- **Teardown**: ~15 instructions (context cleanup, POPFD, POPAD)
- **Estimated**: ~50-100 CPU cycles
- **Negligible** for most game functions

**REPLACE type**:
- Zero overhead from wrapper
- Patch function overhead depends on implementation

## Testing

### Build the Project

Build in Visual Studio (Win32 platform):
```
Right-click KotorPatcher project -> Build
```

### Create Test Patch DLL

See `examples/example_patch.cpp` for complete examples.

### Test Configuration

See `examples/patch_config.toml` for test configs.

### Monitor Output

Use DebugView to see debug messages:
```
[Wrapper] Using wrapper generator: x86_Win32
[Config] Loaded hook: test-patch -> MyPatch @ 0x00401234 (6 bytes)
[Wrapper] Generated INLINE wrapper at 0x12340000 (87 bytes)
[KotorPatcher] Applied INLINE hook at 0x00401234 -> MyPatch
```

## Future Enhancements

### Phase 2: Detour Trampolines with Stolen Bytes

The wrapper system is designed to integrate with detour trampolines:

```cpp
// Future: WRAP type with detours
extern "C" __declspec(dllexport)
void WrapOriginal(PatchContext* ctx) {
    // Before original
    OutputDebugStringA("Before\n");

    // Call original function
    typedef int (*OriginalFunc)(int, int);
    OriginalFunc original = (OriginalFunc)ctx->original_function;
    int result = original(ctx->GetParameter(0), ctx->GetParameter(1));

    // After original
    ctx->SetReturnValue(result * 2);  // Modify result
}
```

This requires:
- Length disassembler (hde32 recommended)
- Trampoline allocator
- Stolen bytes preservation
- See `FUTURE_ENHANCEMENTS.md` for details

### Platform Expansion

Easy to add:
- Windows x64
- macOS (x86_64 and ARM64)
- Linux (via Wine or native)

Each platform just needs a new `WrapperGenerator` implementation.

## Comparison: Old vs New

| Feature | Old (Simple JMP) | New (Wrapper System) |
|---------|------------------|----------------------|
| **Ease of use** | Hard (assembly) | Easy (C code) |
| **State access** | Manual | Automatic |
| **Register save/restore** | Manual | Automatic |
| **Modify specific registers** | Manual tracking | `exclude_from_restore` |
| **Inspect original state** | Not possible | Full `PatchContext` |
| **Error-prone** | Very | Minimal |
| **Performance** | Fastest | Minimal overhead |
| **Debugging** | Nightmare | Standard C debugging |
| **Platform portability** | Rewrite for each | Change one class |

## Recommendations

### For Most Patches
Use **INLINE** type:
- Easiest to write and maintain
- Automatic safety
- Negligible performance impact
- Full debugging support

### For Performance-Critical Hooks
Use **REPLACE** type only if:
- You've profiled and wrapper overhead matters
- You're comfortable with x86 assembly
- You understand calling conventions perfectly

### For Before/After Logic
Wait for **WRAP** type (Phase 2):
- Requires detour trampolines
- Coming soon with stolen bytes implementation

## Questions?

See:
- `README.md` - General patcher documentation
- `examples/example_patch.cpp` - Complete patch examples
- `examples/patch_config.toml` - Configuration examples
- `FUTURE_ENHANCEMENTS.md` - Planned features

## License

Part of the KOTOR Patcher project.
