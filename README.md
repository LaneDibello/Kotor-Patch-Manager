# KotOR Patch Manager

The KotOR Patch Manager (KPM) is a dynamic patching framework for Star Wars: Knights of the Old Republic 1 and 2.

It operates off of a runtime DLL/SO/DYLIB-injection scheme, which installs the applied patches live, meaning that the user's executable does not get modified and users can easily "cherry pick" certain patches they want to use. This also has added benefits for compatibility, version management, and distribution.

The repository contains:
- a selection of pre-made Patches,
- aa GameAPI and creation script to aid in creation of custom patches,
- C++ source for KotorPatcher.dll, the library that is injected at runtime,
- C# source for KPatchCore, the framework for parsing and applying these patches, and
- C# source for KPatchLauncher, a basic UI for using this patching framework (will likely be replaced in the future).

See road map [here](docs/Roadmap2026.md)

## Install
*NOTE: The patch manager is still in beta; there are likely going to be various issues within the current release. Please feel free to create GitHub Issues or contact Lane if anything strange comes up.*

### Using the Release
If you're just interested in experimenting with existing patches, you can simply download the [most recent release](https://github.com/LaneDibello/Kotor-Patch-Manager/releases). Windows users want `KotorPatchManager-v*.zip`; Linux users want the `KotorPatchManager-linux-v*.tar.gz`. Mac users with Apple Silicon chips will want `KotorPatchManager-macos-arm64-v*.tar.gz`; those with Intel chips will want `KotorPatchManager-macos-x64-v*.tar.gz`.

The release contains the following:
- bin/
	- AddressDatabases: SQLite DB files containing address information for various kotor versions.
	- `KotorPatcher.dll`, `KotorPatcher.so` (Linux only), and `KotorPatcher.dylib` (MacOS only): Dynamic libraries for PC (`.dll`), Linux (`.so`), and Mac (`.dylib`) that are compiled at runtime/injected.
	- **KPatchLauncher.exe** (Windows) or **KPatchLauncher** (Linux and MacOS): The main launcher program (must be in the same directory as the dynamic library and AddressDatabases/ to function correctly). For Linux and MacOS users, double click or run via the terminal: `./KPatchLauncher`.
- patches/
	- `.kpatch` files: The patch files that the framework can read in and apply to the game.
	- "additional files": Resources associated with certain patches that may be necessary to make use of them (i.e. modified `nwscript.nss`, for the script extender).
- tools/
	- `create-patch.bat` and `create-patch.py`: Alternative means for building `.kpatch` files (Linux and MacOS users must use the python script). The .bat file isn't too useful in its current iteration without the added GameAPI source and examples, but it's included nonetheless.
- README.txt: A brief contents and quick-start guide

To use KPM, just run the launcher. Set the "Game" path to target your game (i.e. `swkotor.exe`), and set the "Patches" path to target the directory with your `.kpatch` files (i.e. `<release>/patches`).

**Linux and Mac users**: There are two kinds of Linux and Mac installs, and the manager picks the right one from the executable you point it at:

- **The Windows game under Wine/Proton.** Because a native app can't inject into a Wine/Proton process, `bin` additionally contains `binkw32.dll` (a small proxy) and `sqlite3.dll`, which the manager stages into the game folder so the patches load when the game starts. Point the "Game" path at the game executable inside your Wine/Proton install (`swkotor.exe`).
- **KOTOR II's native Linux build (Aspyr).** To modify the Linux-native version of the game, the "Game" path should be pointed at the native Linux build, which has no extension (`KOTOR2`). The manager stages `KotorPatcher.so` and adds it to the game's library dependencies, so the game loads the patcher itself with no proxy and nothing to inject. See [docs/NATIVE_LINUX.md](docs/NATIVE_LINUX.md) for the details and current limitations. KOTOR I has no native Linux port, so this only applies to KOTOR II.
- **KOTOR II's native Mac build (Aspyr).** The Mac-native game is a `.app`, which will appear extension-less in the Finder, and the "Game" path should be pointed  the "Game" path at the folder containing it. 

Either way, use "Launch" to start the game through Steam or a custom command.

**Windows users** can also select the proxy mode by utilizing the "Options" menu.

### Building from Source
If you're interested in contributing or making your own patches, you're going to want to clone this repository and build from source.

This project has been built and configured with Visual Studio 2022. While there are likely alternatives that would function here, I have not tested nor validated any of them.

- Open the solution file (`KotorPatchManager.sln`) in Visual Studio.
- Ensure that the Startup project is `KPatchLauncher`.
- The Build Configuration should have `KotorPatcher` set to `Win32` (KotOR is a 32-bit game). The other projects can be `Any CPU`. This should be the default setting.
- Run `Build > Rebuild Solution` to validate that the build is configured correctly and functioning. If it fails, it's possible that some dependency resolution may have failed or you may not have the necessary `.Net` version. 

With those configurations, you should be able to just run the launcher. As stated above, set the "Game" path to target your game executable (`swkotor.exe`) and the "Patches" path to target the directory where you plan to store your `.kpatch` files.

To build the `.kpatch` file for any patch, simply open the directory (i.e. `cd Patches\AdditionalConsoleCommands`) in a batch-capable terminal (i.e. powershell or command-prompt) and run:
```
..\create-patch.bat
```

This will build and package the patch into a `.kpatch` file.

**On Linux:** there's no Visual Studio; instead you'll need the .NET 8 SDK, MinGW-w64 (`i686-w64-mingw32-g++`, for the 32-bit DLLs the game loads under Wine), and `python3`. The manager builds natively with `dotnet`, and the `build-mingw.sh` scripts cross-compile `KotorPatcher.dll` and the `binkw32.dll` proxy. Build a patch with `python3 ../create-patch.py` (the equivalent of `..\create-patch.bat`) from within the patch's directory.

To also target KOTOR II's native Linux build, run `./build-linux.sh`, which compiles `KotorPatcher.so` and stages it beside the launcher. That one needs 32-bit development libraries (`glibc-devel.i686` and `libstdc++-devel.i686` on Fedora, or your distro's equivalents).

**On MacOS**: Mac users will need to run:
```
CXX_MAC=clang++ make dylib
dotnet run --project src/KPatchLauncher/KPatchLauncher.csproj
```
Build a patch with `python3 ../create-patch.py` from within the patch's directory.

## Usage
Available patches will appear on the left-hand side, with descriptions on the right-hand side. Select the patches you want and select "Apply", to prepare the game for use with those patches. Select "Launch" to run the game with these patches applied.

Patches can be uninstalled by unchecking them and clicking "Apply" or using the "Uninstall All" button.

## Patches
This repository's "Patches" directory contains several example patches, such as the ScriptExtender, AdditionalConsoleCommands, Level-Cap extension, and more. In addition to this, it also contains a directory titled "Common", which has a variety of utilities and classes to aid in creation of patches.

A patch typically contains 2 to 3 parts:
- a `manifest.toml` file that specifies various patch and compatibility info;
- a `hooks.toml` file (or files) that contain the meat and potatoes of the actual patches; and, optionally,
- additional C++ code that gets compiled into the patch and injected, as specified by the hooks.

### Manifest
- `id`: A program-friendly identifier with expressed with `[a-zA-Z0-9_-]`.
- `name`: A human-friendly identifier that could appear in UIs and docs.
- `version`: The patch's version following `major.minor.patch` format.
- `author`: The patch's creator.
- `description`: A brief description of the patch and what it does.
- `requires`: List of required patches (by `id`) for the patch to work.
- `conflicts`: List of patches (by `id`) that conflict with the patch's functionality.
- `supported_versions`: key/value pair of game versions and their SHA-256s.

### Hooks
There are 4 different types of hooks currently, `simple`, `replace`, `detour`, and `static`. They all share certain fields.

#### Shared Fields
- `address`: The hexadecimal (`0x########`) address where the hook will be applied.
- `type`: The type of hook, either `"simple"`, `"replace"`, `"detour"`, or `"static"`.

#### Simple Hooks
Simple hooks replace a finite set of bytes with a new set of bytes of the same length. They must specify:

- `original_bytes`: Any number of bytes starting from `address` that will be overwritten.
- `replacement_bytes`: Bytes equal in length to `original_bytes` that will overwrite them.

#### Replace Hooks
Replace hooks allow for more advanced instruction replacement. They will allocate executable memory, write out the specified instructions, and then have the code at `address` jump to those instructions. Use these if you want to replace instructions with more complex logic that wouldn't fit into the existing logic. They require the specification of:

- `original_bytes`: 5 or more bytes, starting from `address`, that will be replaced with a JUMP instruction and NOPs.
- `replacement_bytes`: Any number of bytes that will be jumped to by the above instruction and executed as x86, after which logic will jump back to `address+original_bytes.length`.

#### Detour Hooks
Detour hooks are our most advanced option. They replace the code at address with a JUMP to a wrapper that (1) stores register values, (2) prepares parameters, and (3) calls an external function defined and compiled within your patch. Use these for very complex patches, especially those that need to call an existing in-game function, output debug strings, or reference specific addresses. Specify:

- `original_bytes`: 5 or more bytes starting from `address` that will be replaced with a JUMP instruction and NOPs.
- `function`: The name of an exported `extern "C"`/`__cdecl` function that will be compiled and run from your additional C++ code.
- `skip_original_bytes`: If `false`, the `original_bytes` will be re-run after the function finishes execution, making it a true detour. If `true`, the `original_bytes` will never be run.
- `exclude_from_restore`: List of registers to keep modified after hook execution completes.
- `parameters`: Parameters to be passed in (cdecl/stack style) to your `function`:
	- `source`: the register from which the parameter will be sourced.
	- `type`: The type of the parameter. Currently we support: `Int`, `Uint`, `Pointer`, `Float`, `Byte`, and `Short`.

#### Static Hooks
Static hooks are applied directly to the executable file at install-time, before the game runs. This is necessary for patches that modify the PE header or other structures that must be patched before the executable loads into memory. Unlike other hook types, static hooks do not require runtime injection. Specify:

- `original_bytes`: Any number of bytes starting from `address` that will be verified before patching.
- `replacement_bytes`: Bytes equal in length to `original_bytes` that will overwrite the original bytes in the file.
 
### Building Patches
To build a patch, use the `create-patch.bat` batch file or `create-patch.py` Python script from within the Patches/ directory. Usually this will look like:
```
..\create-patch.bat
```
or
```
../create-patch.py
```
### Additional Patch Files
If your patch is meant to be delivered with additional files (for example, files to be placed in the override folder), these should be put in a directory called `additional` in the patch directory. You can also edit The KPM Mods directory to include your patch in that mod.

## KotorPatcher (C++ DLL)
This C++ project builds the actual DLL that get's injected into the game.

The main entry point is in `dllmain.cpp`, which handles initialization and tear-down of the patcher.

The bulk of the business logic lives within `patcher.cpp`, which initializes the version-specific wrapper and parses the patch_config.toml that is generated when patches are applied.

For more details about how this system works see the [KotorPatcher README](src/KotorPatcher/README.md).


## KPatchCore (C# Class Library)
This C# Class Library includes all the necessary functionality to apply patches to the game.
This basically boils down to parsing the patch hooks and manifests, checking compatibility, and building a patch_config.toml.

For more details on this system see the [KPatchCore README](src/KPatchCore/README.md).

## KPatchLauncher (C# Launcher UI)
The C# Avalonia project is the basic UI that leverages the patching framework, aallowing for applying patches and launching patched games.

For more details on this app see the [KPatchLauncher README](src/KPatchLauncher/README.md).

## See Also
KOTOR Patch Manager development takes place mainly in the OpenKotOR discord, if you have any questions or want to work on patching join:

[![OpenKotOR Discord](https://discordapp.com/api/guilds/739590575359262792/widget.png?style=banner2)](https://discord.gg/openkotor)

You can contact Lane on Discord @lane_d

Related [DeadlyStream thread](https://deadlystream.com/topic/11948-kotor-1-gog-reverse-engineering/)

Lane's [YouTube Channel](https://www.youtube.com/@lane_m)

## License
This project is licensed under the MIT License. See [LICENSE](LICENSE) for the full text.

Third-party components keep their own licenses: `tomlplusplus` is MIT, and the bundled SQLite is public domain.

This project is an unofficial fan work. It is not affiliated with, authorized by, or endorsed by BioWare, Obsidian Entertainment, LucasArts, Aspyr, or Disney. Star Wars: Knights of the Old Republic and all related trademarks are the property of their respective owners. No original game assets or executables are distributed here; you must own a copy of the game.

## Acknowledgements
Special thanks to the KotOR modding community for providing feedback and ideation for various features here.

Thanks to Mark Gillard for the `tomlplusplus` project
