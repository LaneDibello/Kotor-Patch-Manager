# Semi-Transparent Letterbox Patch

## Overview

This patch makes the dialogue letterbox semi-transparent instead of fully opaque, allowing you to see more of the game world during conversations.

**Patch Type**: SIMPLE (no DLL required)

## What It Does

Changes a single instruction in the game executable:
- **Address**: `0x006a89b2`
- **Before**: `PUSH 1.0f` (fully opaque letterbox)
- **After**: `PUSH 0.5f` (50% transparent letterbox)

## Technical Details

### Original Instruction
```
68 00 00 80 3f    PUSH 0x3F800000  ; Push 1.0f onto stack
```

### Modified Instruction
```
68 00 00 00 3f    PUSH 0x3F000000  ; Push 0.5f onto stack
```

### IEEE 754 Float Representation
- `1.0f` = `0x3F800000` (little-endian: `00 00 80 3f`)
- `0.5f` = `0x3F000000` (little-endian: `00 00 00 3f`)

## SIMPLE Hook Type

This patch demonstrates the **SIMPLE** hook type introduced in the KotOR Patch Manager:

- ✅ No DLL compilation required
- ✅ Direct byte replacement in memory
- ✅ Applied at runtime (no permanent exe modification)
- ✅ Verified before application (ensures correct game version)
- ✅ Clean and simple - just specify bytes to replace

## Installation

Package this patch into a `.kpatch` file using KPatchConsole:

```bash
# Create the .kpatch package
# (packaging command to be implemented)

# Install the patch
kpatchconsole install --game "C:\Path\to\swkotor.exe" --patch SemiTransparentLetterbox.kpatch
```

## Uninstallation

```bash
kpatchconsole uninstall --game "C:\Path\to\swkotor.exe"
```

This will restore the original game executable from backup.

## Compatibility

- **Game Version**: 4CD retail version of KotOR 1
- **Conflicts**: None known
- **Dependencies**: None

## Visual Effect

The letterbox will be 50% transparent, allowing you to see the game environment behind the dialogue interface while maintaining readability.

## Notes

This is an example of the simplest possible patch - changing a single constant value. More complex SIMPLE patches can replace multiple bytes, change jump targets, NOP out instructions, etc.
