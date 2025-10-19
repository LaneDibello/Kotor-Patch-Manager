# KotOR Patch Manager - Patch Creation Kit

Quick start guide for creating patches for Knights of the Old Republic using the KotOR Patch Manager.

## 📋 Prerequisites

- **Visual Studio 2022, 2019, or 2017** (only needed for DETOUR patches with C++ code)
  - Install "Desktop development with C++" workload
- **PowerShell** (included with Windows)
- **A disassembler/debugger** (IDA Pro, Ghidra, x64dbg, etc.) to find addresses and bytes

## 🚀 Quick Start (5 Minutes)

### Option 1: SIMPLE Patch (No C++ Required)

Create a patch that modifies bytes directly without needing a DLL.

1. **Create a new folder** for your patch:
   ```
   MyAwesomePatch/
   ```

2. **Copy the template files** from `templates/`:
   ```
   copy templates\manifest.template.toml MyAwesomePatch\manifest.toml
   copy templates\hooks.template.toml MyAwesomePatch\hooks.toml
   ```

3. **Edit manifest.toml** with your patch info

4. **Edit hooks.toml** with your byte changes:
   ```toml
   [[hooks]]
   address = 0x006a89b2
   type = "simple"
   original_bytes = [0x68, 0x00, 0x00, 0x80, 0x3f]  # What's there now
   replacement_bytes = [0x68, 0x00, 0x00, 0x00, 0x3f]  # What to change it to
   ```

5. **Copy create-patch.bat** into your patch folder

6. **Run create-patch.bat**:
   ```
   cd MyAwesomePatch
   create-patch.bat
   ```

7. **Done!** Your `.kpatch` file is ready to install.

### Option 2: DETOUR Patch (C++ Required)

Create a patch that calls your custom C++ code.

1. **Create a new folder** for your patch

2. **Copy the template files** from `templates/`

3. **Create your C++ file** (e.g., `my_patch.cpp`):
   ```cpp
   #include <windows.h>

   extern "C" void __cdecl MyHook(int param1, float param2) {
       // Your code here
   }

   BOOL WINAPI DllMain(HINSTANCE h, DWORD r, LPVOID l) {
       return TRUE;
   }
   ```

4. **Edit hooks.toml** to reference your function:
   ```toml
   [[hooks]]
   address = 0x005cb41c
   function = "MyHook"
   type = "detour"
   original_bytes = [0x55, 0x8B, 0xEC, ...]
   ```

5. **Run create-patch.bat** - it will auto-compile and package everything

## 📁 Folder Structure

Your patch folder should look like this:

### SIMPLE Patch:
```
MySimplePatch/
├── manifest.toml
├── hooks.toml
└── create-patch.bat
```

### DETOUR Patch:
```
MyDetourPatch/
├── manifest.toml
├── hooks.toml
├── my_code.cpp
├── exports.def (optional - auto-generated if missing)
└── create-patch.bat
```

## 🎯 What create-patch.bat Does

1. **Validates** your manifest.toml and hooks.toml exist
2. **Detects patch type**:
   - No .cpp files = SIMPLE patch
   - One or more .cpp files = DETOUR patch
3. **Builds DLL** (if DETOUR):
   - Auto-finds Visual Studio
   - Compiles all .cpp files together
   - Generates exports.def if missing
4. **Packages** everything into a `.kpatch` file:
   - Adds manifest.toml
   - Adds hooks.toml
   - Adds binaries/ folder (if DLL exists)
5. **Verifies** the package contents

## 📚 Examples

Check the `examples/` folder for complete working examples:

- **DETOUR_EXAMPLE/**: Shows how to create a DLL-based patch with parameter extraction
- **SIMPLE_EXAMPLE/**: Shows how to create byte-replacement patches

## 🐛 Troubleshooting

### "ERROR: Visual Studio not found!"
- Install Visual Studio with C++ build tools
- Or set `VCVARSALL` environment variable to point to `vcvars32.bat`

### "ERROR: manifest.toml not found!"
- Make sure you're running create-patch.bat from your patch folder
- Copy the template from `templates/manifest.template.toml`

### DLL compilation fails
- Check `build.log` for error details
- Make sure your function signatures match the hooks.toml
- Verify all .cpp files have valid C++ syntax

### Package created but won't install
- Verify your `original_bytes` match the actual game executable
- Check that the game version hash in manifest.toml is correct
- Run `kpatchconsole --status <game_exe>` to get the correct hash

## 📖 Next Steps

- **Read CREATING_PATCHES.md** for complete documentation
- **Study the examples** in `examples/` folder
- **Test your patch** with a backup copy of the game
- **Use DebugView** to see your patch running in real-time

## 🛠️ Advanced Usage

### Custom DLL names
The script always creates `windows_x86.dll` - this is required for the .kpatch format.

### Multiple .cpp files
All .cpp files in the folder will be compiled into one DLL automatically.

### Override Visual Studio path
Set environment variable before running:
```
set VCVARSALL=C:\Path\To\vcvars32.bat
create-patch.bat
```

## ⚠️ Important Notes

- **Always backup your game** before testing patches
- **Test on a separate game copy** first
- **Verify original_bytes** with a disassembler - wrong bytes = crashes
- **Use DebugView** to see OutputDebugString messages from your DLL
- **Launch with KPatchLauncher.exe**, not swkotor.exe directly

## 📞 Getting Help

See CREATING_PATCHES.md for:
- Complete guide to manifest.toml format
- Complete guide to hooks.toml format
- Parameter extraction details
- Hook type comparison (SIMPLE vs DETOUR)
- Testing and debugging tips
- Common issues and solutions

---

**Ready to create your first patch? Start with a SIMPLE one!**
