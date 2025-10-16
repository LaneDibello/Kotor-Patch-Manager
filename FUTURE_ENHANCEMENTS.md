# Future Enhancements & Roadmap

This document tracks planned features and improvements for the KOTOR Patcher project.

---

## Phase 2: Enhanced Trampolines (HIGH PRIORITY)

### Stolen Bytes / Detour Trampolines

**Current Limitation:**
The runtime currently uses simple 5-byte JMP trampolines that overwrite original game code. This means:
- Original instructions are lost
- Patches must completely replace functionality
- Cannot "wrap" or "extend" existing game functions
- Limited to function entry point hooks

**Proposed Enhancement:**
Implement proper detour trampolines that preserve original instructions.

### Implementation Plan

#### 1. Add Length Disassembler

Choose one of:
- **hde32** (Minimal, ~500 lines, header-only)
  - Pros: Tiny, fast, no dependencies
  - Cons: x86 only, just length (no full disassembly)
  - Recommended for this project

- **Zydis** (Full-featured)
  - Pros: Complete disassembler, well-maintained, supports x86/x64
  - Cons: Larger library, more overhead

- **Capstone** (Industry standard)
  - Pros: Very mature, multi-architecture
  - Cons: Heaviest dependency

**Recommendation:** Start with hde32 for minimal footprint.

#### 2. Trampoline Allocator

```cpp
// New module: trampoline_allocator.h/cpp
namespace KotorPatcher {
    namespace TrampolineAllocator {
        // Allocate executable memory near target address
        void* AllocateTrampoline(DWORD nearAddress, size_t size);

        // Free all allocated trampolines on cleanup
        void FreeAllTrampolines();
    }
}
```

**Implementation:**
```cpp
void* AllocateTrampoline(DWORD nearAddress, size_t size) {
    // Try to allocate within ±2GB of target (for 5-byte relative JMP)
    DWORD minAddr = (nearAddress > 0x7FFFFFFF) ? 0 : nearAddress - 0x7FFFFFFF;
    DWORD maxAddr = nearAddress + 0x7FFFFFFF;

    SYSTEM_INFO si;
    GetSystemInfo(&si);

    // Search for available memory in chunks
    for (DWORD addr = minAddr; addr < maxAddr; addr += si.dwAllocationGranularity) {
        void* mem = VirtualAlloc(
            reinterpret_cast<LPVOID>(addr),
            size,
            MEM_COMMIT | MEM_RESERVE,
            PAGE_EXECUTE_READWRITE
        );
        if (mem) return mem;
    }

    return nullptr; // Fallback: allocate anywhere
}
```

#### 3. Enhanced PatchInfo Structure

```cpp
// Update patcher.h
struct PatchInfo {
    std::string dllPath;
    std::string functionName;
    DWORD hookAddress;
    std::vector<BYTE> originalBytes;

    // NEW: Detour support
    TrampolineType type;  // SIMPLE_JMP or DETOUR
    void* trampolineAddr; // Allocated trampoline (if detour)
    size_t stolenByteCount; // How many bytes we stole

    enum class TrampolineType {
        SIMPLE_JMP,  // Current implementation
        DETOUR       // New: stolen bytes trampoline
    };
};
```

#### 4. Enhanced Trampoline Functions

```cpp
// Add to trampoline.h
namespace Trampoline {
    // New: Create detour trampoline with stolen bytes
    bool WriteDetourJump(
        DWORD hookAddress,
        void* patchFunction,
        void** outOriginalFunction,  // Returns callable trampoline
        size_t* outStolenBytes
    );

    // Existing simple JMP (keep for compatibility)
    bool WriteJump(DWORD address, void* target);
}
```

**Implementation:**
```cpp
bool WriteDetourJump(
    DWORD hookAddress,
    void* patchFunction,
    void** outOriginalFunction,
    size_t* outStolenBytes
) {
    // 1. Disassemble instructions at hookAddress until we have >= 5 bytes
    size_t totalLen = 0;
    size_t instructionCount = 0;
    BYTE* current = reinterpret_cast<BYTE*>(hookAddress);

    while (totalLen < 5) {
        size_t len = hde32_disasm(current, &hde);
        if (len == 0) return false; // Invalid instruction
        totalLen += len;
        current += len;
        instructionCount++;
    }

    *outStolenBytes = totalLen;

    // 2. Allocate trampoline memory (stolen bytes + JMP back)
    size_t trampolineSize = totalLen + 5; // stolen bytes + 5-byte JMP
    void* trampoline = TrampolineAllocator::AllocateTrampoline(hookAddress, trampolineSize);
    if (!trampoline) return false;

    // 3. Copy stolen bytes to trampoline
    memcpy(trampoline, reinterpret_cast<void*>(hookAddress), totalLen);

    // 4. Write JMP back to original code after stolen bytes
    DWORD returnAddr = hookAddress + totalLen;
    BYTE* jmpBack = reinterpret_cast<BYTE*>(trampoline) + totalLen;
    DWORD jmpOffset = returnAddr - (reinterpret_cast<DWORD>(jmpBack) + 5);
    jmpBack[0] = 0xE9; // JMP opcode
    memcpy(jmpBack + 1, &jmpOffset, 4);

    // 5. Write JMP to patch function at hook address
    if (!WriteJump(hookAddress, patchFunction)) {
        return false;
    }

    // 6. Return trampoline address (callable "original function")
    *outOriginalFunction = trampoline;
    return true;
}
```

#### 5. Update Config Format

Add optional `type` field to hooks in `patch_config.toml`:

```toml
[[patches.hooks]]
address = 0x401234
function = "MyPatchFunction"
original_bytes = [0x55, 0x8B, 0xEC]
type = "detour"  # Optional: "simple_jmp" (default) or "detour"
```

#### 6. Patch Function Signature (for Detours)

When using detours, patch functions receive a pointer to the original function:

```cpp
// Patch DLL exports this function
extern "C" __declspec(dllexport) void MyPatchFunction(void* originalFunction) {
    // Cast originalFunction to appropriate type
    typedef int (__stdcall *OriginalFunc)(int, int);
    OriginalFunc original = reinterpret_cast<OriginalFunc>(originalFunction);

    // Execute custom logic BEFORE original
    OutputDebugStringA("Before original function\n");

    // Call original function if needed
    int result = original(10, 20);

    // Execute custom logic AFTER original
    OutputDebugStringA("After original function\n");

    return; // Or return modified result
}
```

### Benefits of Detour Implementation

1. **Flexibility**: Patches can wrap existing functionality instead of replacing
2. **Safety**: Original code preserved and executable
3. **Mid-function hooks**: Can hook anywhere, not just function entry
4. **Compatibility**: Easier to combine multiple patches
5. **Debugging**: Can call original for comparison

### Testing Strategy

1. Create test patch that wraps a simple function
2. Verify stolen bytes are correct length
3. Test with instructions that span the 5-byte boundary
4. Ensure trampoline memory is executable
5. Validate JMP back to original code works

### Compatibility Notes

- Keep simple JMP as default for backward compatibility
- Make detours opt-in via config
- Document both approaches for patch authors
- Validate that patch DLL exports match trampoline type

---

## Phase 3: Other Enhancements

### Hot-Reload Patches
- Watch `patch_config.toml` for changes
- Reload and apply new patches without restarting game
- Remove/restore patches dynamically

### Signature Scanning
- Find hook addresses by byte patterns instead of fixed addresses
- Makes patches more version-portable
- Useful when exact addresses differ between game versions

### Advanced Memory Protection
- Restore original code on DLL unload
- Implement patch enable/disable at runtime
- Create snapshots for rollback

### Logging System
- Optional file logging (in addition to OutputDebugString)
- Configurable log levels (ERROR, WARN, INFO, DEBUG)
- Performance metrics (patch load time, etc.)

### Multiple Hook Types
- CALL trampolines (already implemented but unused)
- Inline hooks (mid-function)
- Import table hooks
- Virtual function table hooks

---

## Phase 4: Cross-Platform Support

### macOS Support
- Adapt trampolines for x86_64/ARM64 macOS
- Use mach-o DLL injection
- Support KOTOR Mac versions

### Linux Support (via Wine/Proton)
- Test compatibility with Wine
- Potential native Linux KOTOR support

---

## Implementation Priority

1. **Phase 2 - Detour Trampolines**: HIGH PRIORITY (requested by user)
2. **Signature Scanning**: Medium priority (helps with version compatibility)
3. **Hot-Reload**: Medium priority (quality of life for developers)
4. **Cross-Platform**: Low priority (depends on demand)

---

## Questions for Future Implementation

- Which disassembler library to use? (Recommend hde32 for minimal footprint)
- Should detours be default or opt-in? (Recommend opt-in for compatibility)
- How to pass original function pointer to patch? (Via parameter or global?)
- Support for multiple hook types per patch? (Future consideration)

---

Last Updated: 2024-10-16
Status: Planning Phase
