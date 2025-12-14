# KotorPatcher Runtime DLL

KotorPatcher is the runtime DLL that gets injected into the KotOR game process to apply patches dynamically. It reads patch configurations, loads patch DLLs, and modifies game code in-memory using various hooking techniques.

## Architecture Overview

The patcher operates in three phases:

1. **Initialization** (DLL_PROCESS_ATTACH): Parse `patch_config.toml`, load patch DLLs, apply hooks
2. **Runtime**: Patches execute as the game runs, intercepting and modifying game behavior
3. **Cleanup** (DLL_PROCESS_DETACH): Unload patch DLLs and free allocated memory

## Project Structure

### Core Components

**dllmain.cpp**: DLL entry point that handles initialization and cleanup lifecycle events.

**patcher.h / patcher.cpp**: Core patching engine containing hook application logic, patch DLL loading, and configuration management. Defines the `PatchInfo` structure that represents a single hook configuration.

**config_reader.h / config_reader.cpp**: TOML parser that reads `patch_config.toml` and converts it into `PatchInfo` structures. Uses the tomlplusplus library for parsing.

**trampoline.h / trampoline.cpp**: Low-level memory patching utilities for writing JMP/CALL instructions, verifying bytes, managing memory protection, and writing NOP instructions.

### Wrapper System

The wrapper system generates runtime x86 assembly code to intercept game functions, preserve CPU state, extract parameters, and call patch functions.

**wrapper_base.h**: Abstract base class defining the wrapper generator interface. Provides platform-agnostic API for creating hook wrappers.

**wrapper_x86_win32.h / wrapper_x86_win32.cpp**: x86 32-bit Windows implementation of the wrapper generator. Emits machine code at runtime to save/restore registers, extract parameters from CPU state, and manage the transition between game code and patch code.

**wrapper_context.h**: Defines `PatchContext` structure containing saved CPU state (registers, flags, stack pointer). Provides helper methods for accessing parameters and modifying return values.

## Hook Types

KotorPatcher supports four distinct hook types:

### DETOUR Hooks

Full-featured hooks with automatic state management. Requires a patch DLL with an exported function.

- Writes 5-byte JMP instruction at hook address
- Generates wrapper stub that saves all CPU state (PUSHAD/PUSHFD)
- Extracts parameters from registers or stack based on configuration
- Calls patch function with extracted parameters (cdecl convention)
- Restores CPU state (with optional register exclusions)
- Re-executes original bytes (stolen bytes) or skips them based on configuration
- Jumps back to continue game execution

**Use when**: Complex logic needed, calling game functions, accessing game state, parameter extraction required.

### SIMPLE Hooks

Direct byte replacement in memory. No DLL loading or wrapper generation.

- Verifies original bytes match expected values
- Overwrites with replacement bytes of equal length
- No state preservation or function calls

**Use when**: Changing constants, NOPing instructions, simple instruction replacements.

### REPLACE Hooks

Allocates executable memory for custom assembly code that executes in place of original instructions.

- Verifies original bytes (minimum 5 bytes)
- Allocates executable memory for replacement code
- Writes replacement bytes followed by JMP back to game
- Writes JMP at hook address to allocated memory
- NOPs remaining bytes

**Use when**: Need more complex logic than SIMPLE hooks but don't require DLL infrastructure.

### DLL_ONLY

Loads a patch DLL without applying any hooks. Used for patches that hook via their DllMain.

## Key Classes and Structures

### PatchInfo

Represents a single hook configuration. Contains:

- **Hook location**: `hookAddress` specifies where to patch
- **Hook type**: `type` determines patching strategy (DETOUR/SIMPLE/REPLACE/DLL_ONLY)
- **Patch DLL**: `dllPath` and `functionName` for DETOUR hooks
- **Byte arrays**: `originalBytes` (for verification and execution), `replacementBytes` (for SIMPLE/REPLACE)
- **State management**: `preserveRegisters`, `preserveFlags`, `excludeFromRestore`
- **Parameters**: `parameters` array defining how to extract values from CPU state
- **Behavior flags**: `skipOriginalBytes` determines whether to re-execute stolen bytes

### ParameterInfo

Defines how to extract a parameter for a DETOUR hook function:

- **source**: Register name ("eax", "ebx", etc.) or stack offset ("esp+0", "esp+4")
- **type**: Data type (INT, UINT, POINTER, FLOAT, BYTE, SHORT)

Parameters are extracted from saved CPU state and pushed onto stack in reverse order (cdecl convention) before calling the patch function.

### WrapperConfig

Configuration passed to wrapper generator:

- **patchFunction**: Address of patch function to call
- **hookAddress**: Game code address being hooked
- **originalBytes**: Stolen bytes to re-execute after patch
- **parameters**: Parameter extraction configuration
- **State preservation options**: Control register/flag saving
- **skipOriginalBytes**: Skip stolen byte execution

### HookType Enum

Defines the four hook types (DETOUR, SIMPLE, REPLACE, DLL_ONLY).

### ParameterType Enum

Defines supported parameter types (INT, UINT, POINTER, FLOAT, BYTE, SHORT).

## Core Functions

### Initialization

**InitializePatcher()**: Called on DLL_PROCESS_ATTACH. Initializes wrapper generator, loads patch_config.toml, sets KOTOR_VERSION_SHA environment variable, and applies all patches.

**CleanupPatcher()**: Called on DLL_PROCESS_DETACH. Frees wrapper stubs, deallocates REPLACE hook memory, and unloads patch DLLs.

### Patch Application

**ApplyPatches()**: Iterates through loaded patches and applies each one.

**ApplyPatch()**: Applies a single patch based on its type. Routes to type-specific handlers.

**ApplySimpleHook()**: Verifies bytes and performs direct memory replacement.

**ApplyReplaceHook()**: Allocates executable memory, writes replacement code, adds return JMP, and patches hook address.

### DETOUR Hook Application

For DETOUR hooks, `ApplyPatch()`:

1. Loads patch DLL via LoadLibraryA
2. Gets function address via GetProcAddress
3. Detects and skips hot-patch stub (0xCC byte) if present
4. Verifies original bytes
5. Generates wrapper via wrapper generator
6. Writes JMP to wrapper at hook address
7. NOPs remaining bytes

### Config Parsing

**Config::ParseConfig()**: Parses patch_config.toml and populates vector of PatchInfo structures. Extracts target_version_sha and validates hook configurations.

**ParseHexAddress()**: Converts hex strings ("0x401234") to DWORD addresses.

**ParseByteArray()**: Converts TOML arrays of integers or hex strings into byte vectors.

### Trampoline Utilities

**Trampoline::WriteJump()**: Writes 5-byte relative JMP (E9 xx xx xx xx) to specified address.

**Trampoline::WriteCall()**: Writes 5-byte relative CALL (E8 xx xx xx xx) to specified address.

**Trampoline::VerifyBytes()**: Compares bytes at address with expected values before patching.

**Trampoline::UnprotectMemory()**: Changes memory protection to PAGE_EXECUTE_READWRITE.

**Trampoline::ProtectMemory()**: Restores original memory protection.

**Trampoline::WriteNoOps()**: Writes NOP instructions (0x90) to fill unused bytes.

### Wrapper Generation

**WrapperGenerator_x86_Win32::GenerateWrapper()**: Routes to GenerateDetourWrapper().

**GenerateDetourWrapper()**: Generates x86 machine code for DETOUR wrapper:

1. Allocates executable memory
2. Emits PUSHAD (save registers) and PUSHFD (save flags)
3. Saves ESP to EBX for later restoration
4. Extracts and pushes parameters in reverse order
5. Calls patch function with relative CALL
6. Cleans up parameters (cdecl caller cleanup)
7. Restores ESP from EBX
8. Emits POPFD and POPAD (or selective register restoration)
9. Re-executes original bytes or skips them based on configuration
10. Jumps back to game code

**ExtractAndPushParameter()**: Generates x86 code to extract a parameter from saved CPU state or stack and push it for the patch function. Handles register sources (eax, ebx, etc.) and stack offsets (esp+0, esp+4, etc.).

**EmitBytes/EmitByte/EmitDword**: Helper functions to write raw bytes into code buffer.

**CalculateRelativeOffset()**: Calculates 32-bit relative offset for JMP/CALL instructions.

## Wrapper Code Generation Details

The generated DETOUR wrapper follows this structure:

```
1. PUSHAD              ; Save all registers (32 bytes)
2. PUSHFD              ; Save flags (4 bytes)
3. MOV EBX, ESP        ; Save stack pointer for restoration
4. [Extract params]    ; Read from saved state, push in reverse order
5. CALL patch_func     ; Call the patch function
6. ADD ESP, N          ; Clean up parameters (cdecl)
7. MOV ESP, EBX        ; Restore stack pointer
8. POPFD               ; Restore flags
9. POPAD or selective  ; Restore registers (respect excludeFromRestore)
10. [Original bytes]   ; Re-execute stolen instructions (unless skipped)
11. JMP return_addr    ; Jump back to game code
```

Register exclusion allows patches to modify specific registers (e.g., changing EAX to modify return value) by selectively skipping restoration for excluded registers.

## Debug Logging

KotorPatcher uses OutputDebugStringA() throughout to log initialization, patch application, and errors. All log messages are prefixed with component names:

- `[KotorPatcher]`: Main patcher operations
- `[Config]`: Configuration parsing
- `[Trampoline]`: Memory patching operations
- `[Wrapper]`: Wrapper generation

## Viewing Debug Logs

Use Sysinternals DebugView to capture debug output:

1. Download DebugView from Microsoft Sysinternals
2. Run as Administrator
3. Enable "Capture Global Win32" in the Capture menu
4. Filter for "KotorPatcher" to see only patcher messages
5. Use Ctrl+X to clear the log buffer

Debug logs show patch loading, hook application, wrapper generation, and any errors encountered during runtime.

## Memory Management

**Wrapper stubs**: Allocated via VirtualAlloc with PAGE_EXECUTE_READWRITE, tracked in m_allocatedWrappers, freed on cleanup.

**REPLACE code buffers**: Allocated via VirtualAlloc, tracked in g_allocatedCodeBuffers, freed on cleanup.

**Patch DLLs**: Loaded via LoadLibraryA, tracked in g_loadedPatches, unloaded via FreeLibrary on cleanup.

All allocations are cleaned up in CleanupPatcher() to prevent memory leaks.

## Environment Variables

KotorPatcher sets the `KOTOR_VERSION_SHA` environment variable based on target_version_sha from patch_config.toml. This allows patch DLLs to query the game version and adjust behavior accordingly.

## Technical Constraints

**Platform**: x86 32-bit Windows only (KotOR is a 32-bit game).

**Calling convention**: Patch functions must use __cdecl convention (caller cleans stack).

**Function exports**: DETOUR patch functions must be exported as `extern "C"` to prevent name mangling.

**Instruction boundaries**: Original bytes must align with x86 instruction boundaries (minimum 5 bytes for DETOUR/REPLACE to fit JMP instruction).

**Memory protection**: All memory patching temporarily changes protection to PAGE_EXECUTE_READWRITE, then restores original protection.

**Hot-patch stubs**: Automatically detects and skips Visual Studio hot-patch stubs (0xCC byte before function entry).

## Error Handling

All patch operations verify original bytes before modification to detect version mismatches. Failures log detailed error messages via OutputDebugStringA and return false to abort initialization. If any patch fails to apply, the entire initialization fails to prevent partial/corrupted game state.
