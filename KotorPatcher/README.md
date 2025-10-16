# KotorPatcher Runtime DLL

This is the C++ runtime component that gets injected into the KOTOR game process and applies patches at runtime.

## Overview

`kotor_patcher.dll` is loaded by the game at startup (via import table modification) and:
1. Reads `patch_config.toml` from the game directory
2. Loads specified patch DLLs
3. Writes JMP trampolines at designated addresses to redirect execution to patch functions

## Architecture

### Components

- **dllmain.cpp**: DLL entry point, calls `InitializePatcher()` on `DLL_PROCESS_ATTACH`
- **patcher.h/.cpp**: Core patching logic - loads patch DLLs and applies hooks
- **trampoline.h/.cpp**: Low-level memory manipulation - writes JMP/CALL instructions
- **config_reader.h/.cpp**: Parses `patch_config.toml` using toml++

### Dependencies

- **Windows API**: `LoadLibrary`, `GetProcAddress`, `VirtualProtect`, `FlushInstructionCache`
- **toml++**: Header-only TOML parsing library (in `external/toml.hpp`)

## Configuration Format

The runtime reads `patch_config.toml` which has the following structure:

```toml
[[patches]]
id = "patch-name"
dll = "patches/patch.dll"

  [[patches.hooks]]
  address = 0x401234              # Hook address (hex or decimal)
  function = "MyPatchFunction"    # Exported function name in DLL
  original_bytes = [0x55, 0x8B, 0xEC]  # Expected bytes for verification
```

### Example Config

See `examples/patch_config.toml` for a complete example.

### Config Fields

- **patches** (array): List of patches to apply
  - **id** (string, optional): Patch identifier for debugging
  - **dll** (string, required): Path to patch DLL (relative to game directory)
  - **hooks** (array, required): List of hook points for this patch
    - **address** (hex string or integer, required): Memory address to hook
    - **function** (string, required): Name of exported function in patch DLL
    - **original_bytes** (array of integers, required): Expected bytes at hook address for verification

## How It Works

### Initialization Flow

1. Game launcher loads `kotor_patcher.dll` (added to import table by KPatchCore)
2. Windows calls `DllMain` with `DLL_PROCESS_ATTACH`
3. `InitializePatcher()` is invoked:
   - Locates DLL's own path using `GetModuleHandleA` + `GetModuleFileNameA`
   - Reads `patch_config.toml` from same directory
   - Calls `Config::ParseConfig()` to parse configuration
   - Calls `ApplyPatches()` to apply all hooks

### Patch Application

For each patch hook:
1. **Load DLL**: `LoadLibrary(patch.dllPath)`
2. **Get Function**: `GetProcAddress(handle, patch.functionName)`
3. **Verify Bytes**: Check that bytes at hook address match `original_bytes`
4. **Write Trampoline**: Write 5-byte JMP instruction: `E9 [4-byte offset]`

### Trampoline Mechanism

A **trampoline** is a 5-byte JMP instruction that redirects execution:

```
Original game code at 0x401234:
  55 8B EC 83 EC 20  (push ebp; mov ebp,esp; sub esp,0x20)

After patching:
  E9 C7 FD 0F 00     (jmp 0x501000 - patch function in DLL)

Offset calculation:
  offset = target_address - (hook_address + 5)
  offset = 0x501000 - (0x401234 + 5) = 0x0FFDC7
```

**Memory Safety**:
- `VirtualProtect` to make code writable (usually read-only)
- `memcpy` to write new instruction
- `VirtualProtect` to restore original protection
- `FlushInstructionCache` to ensure CPU sees changes

## Building

### Requirements

- Visual Studio 2022
- Windows SDK 10.0
- C++17 compiler

### Build Steps

```bash
msbuild KotorPatcher.vcxproj /p:Configuration=Release /p:Platform=Win32
```

Or open in Visual Studio and build with:
- **Configuration**: Release
- **Platform**: Win32 (x86) - KOTOR is a 32-bit game

### Output

- **Debug**: `x64/Debug/kotor_patcher.dll` or `Debug/kotor_patcher.dll`
- **Release**: `x64/Release/kotor_patcher.dll` or `Release/kotor_patcher.dll`

## Testing

### Unit Testing

To test the runtime in isolation:

1. Create a test `patch_config.toml`
2. Create a simple test patch DLL with exported functions
3. Use a test harness or inject into a dummy process
4. Check OutputDebugString output using DebugView

### Integration Testing

1. Install KPatchCore-generated patches into KOTOR
2. Launch game
3. Use DebugView to monitor debug output from patcher
4. Verify patches are applied correctly

### Debug Output

The patcher uses `OutputDebugStringA` for logging. View with:
- [DebugView](https://learn.microsoft.com/en-us/sysinternals/downloads/debugview) (SysInternals)
- Visual Studio debugger (Debug Output window)

Example output:
```
[Config] Successfully loaded 4 patch hook(s)
[Config] Loaded hook: widescreen-fix -> FixedResolutionHandler @ 0x00401234 (6 bytes)
[Config] Loaded hook: widescreen-fix -> CustomAspectRatio @ 0x00405678 (8 bytes)
[KotorPatcher] Failed to initialize
```

## Error Handling

### Philosophy

**Runtime errors should not crash the game**. The patcher follows these principles:

1. **Graceful degradation**: If patches fail to load, game runs without them
2. **Detailed logging**: All errors logged via `OutputDebugString`
3. **Fail-fast on verification**: If original bytes don't match, abort patching
4. **No exceptions**: All errors handled with return values

### Common Errors

| Error | Cause | Solution |
|-------|-------|----------|
| "Failed to open config file" | `patch_config.toml` not found | Ensure config exists in game directory |
| "TOML parse error" | Malformed TOML syntax | Validate config with TOML parser |
| "Failed to load: [dll]" | Patch DLL not found or wrong architecture | Check DLL path and ensure it's 32-bit |
| "Function not found" | Exported function doesn't exist | Verify DLL exports with `dumpbin /exports` |
| "Original bytes mismatch" | Wrong game version or already patched | Check game version, verify not double-patched |
| "Failed to write trampoline" | Memory protection error | Run as administrator, check antivirus |

## Security Considerations

1. **Byte verification**: Always verify original bytes before patching to prevent corruption
2. **Memory protection**: Restore original protection after patching
3. **No arbitrary code**: Only load DLLs specified in config (generated by trusted KPatchCore)
4. **Path validation**: DLL paths are relative to game directory

## Performance

The runtime is designed to be **lightweight**:

- **Initialization**: <10ms typical (depends on number of patches)
- **Memory overhead**: ~50KB for DLL + patch DLLs
- **Runtime overhead**: None after initialization (hooks are direct JMPs)

## Troubleshooting

### Patcher Not Loading

1. Check EXE import table with `dumpbin /imports swkotor.exe`
2. Ensure `kotor_patcher.dll` is in game directory
3. Check DLL is correct architecture (x86 for 32-bit game)

### Patches Not Applied

1. Check `patch_config.toml` exists and is valid TOML
2. Use DebugView to see error messages
3. Verify patch DLLs exist at specified paths
4. Ensure game version matches (original_bytes check)

### Game Crashes

1. Check original_bytes in config match actual game bytes
2. Verify patch DLLs are not corrupted
3. Ensure patch functions have correct calling convention
4. Check for memory corruption in patch code

## Current Limitations

### Simple JMP Trampolines (No Stolen Bytes)

The current implementation uses **simple 5-byte JMP trampolines** that completely replace the original instructions at the hook point. This has important implications:

**What This Means:**
- Original instructions at the hook address are **overwritten** and lost
- Patch functions cannot execute the original code they replaced
- Patches must **completely replace** the hooked function's behavior
- Works best for hooking at function entry points

**Compatibility Constraints:**
- Patches targeting different addresses: ✅ **Compatible**
- Patches targeting overlapping addresses: ❌ **Incompatible** (prevented by byte verification)
- Mid-function hooks: ⚠️ **Limited** (original instructions lost)

**Example:**
```
Original game code at 0x401234:
  55 8B EC 83 EC 20  (push ebp; mov ebp,esp; sub esp,0x20)

After patching:
  E9 C7 FD 0F 00     (jmp to patch DLL)

The patch function must implement the ENTIRE behavior -
it cannot call the original code.
```

**Validation:**
- KPatchCore validates no overlapping hook addresses
- Runtime verifies original bytes before patching
- Conflicts are detected and prevented

**See Future Enhancements below for the planned stolen bytes implementation.**

## Future Enhancements

### High Priority

- [ ] **Detour Trampolines with Stolen Bytes** (Phase 2)
  - Disassemble instructions at hook point to preserve them
  - Allocate executable trampoline memory
  - Copy stolen bytes + JMP back to original code
  - Allow patches to call original function before/after their logic
  - Benefits:
    - Patches can wrap/extend functionality (not just replace)
    - Mid-function hooks work safely
    - Original code preserved and callable
  - Implementation:
    - Add length disassembler (Zydis, Capstone, or minimal hde32)
    - Allocate trampolines with `VirtualAlloc(PAGE_EXECUTE_READWRITE)`
    - Provide "original function" pointer to patch functions
    - Add hook type field to config: `type = "detour"` vs `type = "simple_jmp"`

### Medium Priority

- [ ] Hot-reload patches without restarting game
- [ ] Config reload on file change
- [ ] More sophisticated error handling
- [ ] Logging to file (optional)
- [ ] Support for CALL trampolines (currently only JMP)
- [ ] Support for longer trampolines (>5 bytes)
- [ ] Signature scanning (find addresses by byte patterns)

## License

Part of the KOTOR Patcher project. See main repository for license information.
