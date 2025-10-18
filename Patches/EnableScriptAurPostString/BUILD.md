# Building the EnableScriptAurPostString Patch DLL

## Requirements
- Visual Studio 2022 (or compatible C++ compiler)
- Windows SDK
- 32-bit (x86) compilation target

## Build Steps

### Option 1: Using Visual Studio Developer Command Prompt

1. Open "x86 Native Tools Command Prompt for VS 2022"

2. Navigate to the patch directory:
   ```cmd
   cd "C:\Users\laned\source\Repos\KotOR Patch Manager\Patches\EnableScriptAurPostString"
   ```

3. Compile the DLL:
   ```cmd
   cl /LD /O2 /MD aurpoststring_patch.cpp /link /DEF:exports.def /OUT:windows_x86.dll
   ```

   **IMPORTANT**: The `/DEF:exports.def` flag is crucial - it prevents the hot-patch stub (0xCC) from being added to exported functions.

### Option 2: Using cl.exe directly

```cmd
"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\<version>\bin\Hostx64\x86\cl.exe" /LD /O2 /MD aurpoststring_patch.cpp /link /DEF:exports.def /OUT:windows_x86.dll
```

## Compiler Flags Explained

- `/LD` - Create a DLL
- `/O2` - Optimize for speed
- `/MD` - Link with the multithreaded DLL runtime
- `/link` - Pass options to the linker
- `/OUT:windows_x86.dll` - Output filename

## Output

The build should produce:
- `windows_x86.dll` - The patch DLL (this is what goes in the .kpatch file)
- `windows_x86.lib` - Import library (not needed)
- `windows_x86.exp` - Export file (not needed)
- `aurpoststring_patch.obj` - Object file (not needed)

## Verification

After building, verify the DLL exports the correct function:
```cmd
dumpbin /EXPORTS windows_x86.dll
```

You should see `EnableAurPostString_Hook` in the exports list.
