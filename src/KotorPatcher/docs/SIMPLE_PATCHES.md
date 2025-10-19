# SIMPLE Patches

## Overview

SIMPLE patches allow direct byte-level modifications to the game executable without requiring DLL compilation. This is perfect for simple changes like:
- Changing constant values
- NOPing out instructions
- Modifying jump targets
- Replacing single instructions

## Key Features

- ✅ **No DLL Required**: No C++ compilation needed
- ✅ **Runtime Application**: Applied in memory, no permanent exe modification
- ✅ **Verification**: Original bytes checked before replacement
- ✅ **Any Length**: Can replace any number of bytes (as long as lengths match)
- ✅ **Mixed Patches**: Can combine SIMPLE and DETOUR hooks in same .kpatch

## TOML Format

```toml
[[hooks]]
address = 0x00401234
type = "simple"
original_bytes = [0x75, 0x10]        # JNZ short +16
replacement_bytes = [0x90, 0x90]     # NOP NOP
```

### Required Fields

- `address`: Memory address to patch (hexadecimal)
- `type`: Must be `"simple"`
- `original_bytes`: Bytes currently at the address (for verification)
- `replacement_bytes`: New bytes to write

### Restrictions

- ❌ No `function` field (SIMPLE hooks don't call functions)
- ❌ No `parameters` array (no function to pass parameters to)
- ❌ No `preserve_registers` / `preserve_flags` (direct replacement only)
- ✅ `original_bytes.length` MUST equal `replacement_bytes.length`

## Example: Semi-Transparent Letterbox

```toml
[[hooks]]
address = 0x006a89b2
type = "simple"

# Original: PUSH 1.0f
original_bytes = [0x68, 0x00, 0x00, 0x80, 0x3f]

# Modified: PUSH 0.5f
replacement_bytes = [0x68, 0x00, 0x00, 0x00, 0x3f]
```

This changes the dialogue letterbox alpha from 1.0 (opaque) to 0.5 (semi-transparent).

## Common Use Cases

### 1. NOP Out Instructions

Replace unwanted instructions with NOPs:

```toml
[[hooks]]
address = 0x00401000
type = "simple"
original_bytes = [0xe8, 0x10, 0x20, 0x00, 0x00]  # CALL 0x00403015
replacement_bytes = [0x90, 0x90, 0x90, 0x90, 0x90]  # NOP NOP NOP NOP NOP
```

### 2. Change Constant Values

Modify hardcoded constants:

```toml
[[hooks]]
address = 0x00402000
type = "simple"
original_bytes = [0x6a, 0x64]        # PUSH 100
replacement_bytes = [0x6a, 0xc8]     # PUSH 200
```

### 3. Flip Jump Conditions

Change conditional jumps:

```toml
[[hooks]]
address = 0x00403000
type = "simple"
original_bytes = [0x74, 0x10]  # JE (jump if equal)
replacement_bytes = [0x75, 0x10]  # JNE (jump if not equal)
```

### 4. Change Jump Targets

Modify where a jump goes:

```toml
[[hooks]]
address = 0x00404000
type = "simple"
original_bytes = [0xeb, 0x10]  # JMP short +16 bytes
replacement_bytes = [0xeb, 0x20]  # JMP short +32 bytes
```

## How It Works (Runtime)

1. **Verification**: Check that `original_bytes` match what's at `address`
2. **Make Writable**: Change memory protection to `PAGE_EXECUTE_READWRITE`
3. **Replace**: `memcpy` replacement bytes to address
4. **Restore Protection**: Restore original memory protection
5. **Flush Cache**: Invalidate instruction cache for the modified region

## Comparison: SIMPLE vs DETOUR

| Feature | SIMPLE | DETOUR |
|---------|--------|--------|
| DLL Required | ❌ No | ✅ Yes |
| Can call C functions | ❌ No | ✅ Yes |
| Parameter extraction | ❌ No | ✅ Yes |
| State preservation | ❌ No | ✅ Yes |
| Minimum size | Any | 5 bytes (JMP) |
| Use case | Byte changes | Function hooks |

## Best Practices

### 1. Document What You're Changing

```toml
[[hooks]]
# Changes max party size from 3 to 4
# Original instruction: MOV [EAX+offset], 3
address = 0x00405000
type = "simple"
original_bytes = [0xc7, 0x40, 0x10, 0x03, 0x00, 0x00, 0x00]
replacement_bytes = [0xc7, 0x40, 0x10, 0x04, 0x00, 0x00, 0x00]
```

### 2. Use Disassembly Comments

Show the assembly instruction being modified:

```toml
# 68 00 00 80 3f    PUSH 0x3F800000  ; 1.0f
# 68 00 00 00 3f    PUSH 0x3F000000  ; 0.5f
```

### 3. Verify Instruction Boundaries

Always replace complete instructions, never partial:
- ✅ Replace 5-byte `CALL` with 5 NOPs
- ❌ Replace first 3 bytes of 5-byte `CALL` (will crash!)

### 4. Test Before Distributing

Use a debugger to verify:
- Bytes at address match `original_bytes`
- Replacement creates valid instruction(s)
- Game behavior changes as expected

## Limitations

### Cannot Do:
- Call C/C++ functions (use DETOUR for this)
- Access saved register state (use DETOUR with parameters)
- Execute original instructions after modification (use DETOUR)
- Add more bytes than originally present (instruction length changes)

### Can Do:
- Change any constant value
- Modify jump targets (within range)
- NOP out unwanted code
- Change conditional jump types
- Replace instruction with different one of same length

## Validation

The C# parser validates SIMPLE hooks:
- `replacement_bytes` length MUST match `original_bytes` length
- No `function` field allowed
- No `parameters` array allowed
- At runtime, `original_bytes` verified before replacement

## Example Patches

See the `Patches/SemiTransparentLetterbox` directory for a complete working example of a SIMPLE patch.

## When to Use SIMPLE vs DETOUR

**Use SIMPLE when:**
- Changing a constant value
- NOPing out a few instructions
- Simple byte-level modifications
- No need to call custom code

**Use DETOUR when:**
- Need to call a C function
- Complex logic required
- Need access to game state (registers/stack)
- Parameter extraction needed
- Want to execute original code after your function

## Combining SIMPLE and DETOUR

You can mix both types in one `.kpatch`:

```toml
[[hooks]]
# SIMPLE: Change constant
address = 0x00401000
type = "simple"
original_bytes = [0x6a, 0x64]
replacement_bytes = [0x6a, 0xc8]

[[hooks]]
# DETOUR: Complex hook with DLL
address = 0x00402000
type = "detour"
function = "MyHookFunction"
original_bytes = [0x8b, 0x45, 0x08, 0x50, 0xe8]
# ... parameters, etc.
```

The patcher will:
1. Apply all SIMPLE hooks (no DLL needed)
2. Load DLL and apply DETOUR hooks

This is useful for patches that need both simple tweaks and complex functionality!
