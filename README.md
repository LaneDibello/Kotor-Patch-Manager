# KotOR Patch Manager

The KotOR Patch Manager (KPM) is a dynamic patching framework for Star Wars: Knights of the Old Republic 1 and 2.

It operates via a runtime DLL/SO/DYLIB-injection scheme, which installs the applied patches live, meaning that the user's executable does not get modified and users can easily "cherry-pick" certain patches they want to use. This also has added benefits for compatibility, version management, and distribution.

See roadmap [here](docs/Roadmap2026.md).

## Install
*NOTE: The patch manager is still in beta; there are likely going to be various issues within the current release. Please feel free to create GitHub Issues or contact Lane if anything strange comes up.*

If you're just interested in using the current offering of patches, you can simply download the [most recent release](https://github.com/LaneDibello/Kotor-Patch-Manager/releases). Windows users want `KotorPatchManager-v*.zip`; Linux users want `KotorPatchManager-linux-v*.tar.gz`. Mac users with Apple Silicon chips will want `KotorPatchManager-macos-arm64-v*.tar.gz`; those with Intel chips will want `KotorPatchManager-macos-x64-v*.tar.gz`.

If you're interested in contributing or making your own patches, you're going to want to clone this repository (`git clone https://github.com/FTD516/Kotor-Patch-Manager`) and build from source. Note that you will have to create `.kpatch` files for every patch you wish to use (see ["Building from Source"](#building-from-source)).

## Quick Start / Basic Usage
**The launcher is in the bin/ folder**—for Windows users, **KPatchLauncher.exe**, for Linux and macOS users, **KPatchLauncher**. Do not move the main launcher out of the bin/ folder: it must be in the same directory as the dynamic library and AddressDatabases folder to function correctly. 

To use KPM, run the launcher. For Linux and macOS users, double-click or run via the terminal: `./KPatchLauncher`. 

Once KPM is open, set the "Game" path to target your game (i.e., `swkotor.exe` or, if you cannot select your game directly, the folder containing your game), and set the "Patches" path to target the directory with your `.kpatch` files (e.g., `<release>/patches`). 

Available patches will appear on the left-hand side, with descriptions on the right-hand side. Select the patches you want and click "Apply" to prepare the game for use with those patches. Patches can be uninstalled by unchecking them and clicking "Apply" or using the "Uninstall All" button.

Select "Launch" to run the game with these patches applied.

***Important:*** Please read each patch description carefully—some patches require additional files to be installed. You can find these additional files in the patches folder, under \[Name of Patch\] additional files/. You can also install these additional files by pointing HoloPatcher (presently not included) at The KPM Mods/ directory.

### Proxy Mode
Several Linux and Mac users run a version of KotOR 1 or 2 that is not native to their OS—choosing, instead, to run a Windows version via Wine or Proton. KPM allows Linux and Mac users to patch these non-native versions; the manager picks the right install method based on the version of the game it is pointed at.

The details, for those who want them:
- Patching **KOTOR II's native Linux build (Aspyr):** To modify the Linux-native version of the game, the "Game" path should be pointed at the native Linux build, which has no extension (`KOTOR2`). The manager stages `KotorPatcher.so` and adds it to the game's library dependencies, so the game loads the patcher itself with no proxy and nothing to inject. See [docs/NATIVE_LINUX.md](docs/NATIVE_LINUX.md) for the details and current limitations. KOTOR I has no native Linux port, so this only applies to KOTOR II.
- Patching **KOTOR's native Mac build (Aspyr):** The Mac-native game is a `.app`, which will appear extension-less in the Finder, and the "Game" path should be pointed at the folder containing it. Again, no proxy is needed for macOS users to patch their native versions of the games.

Conversely, for patching **the Windows game under Wine/Proton:** Because a native app can't inject into a Wine/Proton process, bin/ additionally contains `binkw32.dll` (a small proxy) and `sqlite3.dll`, which the manager stages into the game folder, so the patches load when the game starts. Simply point the "Game" path at the game executable inside your Wine/Proton install (`swkotor.exe`).

**Windows users** can also select the proxy mode by utilizing the "Options" menu.

## Release Contents / Advanced Usage
The release contains:
- bin/
 	- The OS-specific launcher: `KPatchLauncher.exe` (Windows) or `KPatchLauncher` (Linux and macOS).
	- AddressDatabases/: SQLite DB files containing address information for various KotOR versions.
	- `KotorPatcher.dll`, `KotorPatcher.so` (Linux only), and `KotorPatcher.dylib` (macOS only): Dynamic libraries for PC (`.dll`), Linux (`.so`), and Mac (`.dylib`) that are injected at runtime.
- patches/
	- `.kpatch` files: The patch files that the framework can read in and apply to the game.
	- Additional files associated with certain patches that may be necessary to make use of them (e.g., a modified `nwscript.nss` for the script extender).
- tools/
	- `create-patch.bat` and `create-patch.py`: Alternative means for building `.kpatch` files (Linux and macOS users must use the Python script). The .bat file isn't too useful in its current iteration without the added GameAPI source and examples, but it's included nonetheless.
- `README.txt`: A brief contents and quick-start guide.
- The KPM Mods/
  	- A traditional mod allowing for the install (via HoloPatcher, presently not included) of various additional files associated with certain patches.

### Building from Source
This project has been built and configured with Visual Studio 2022. While there are likely alternatives that would function here, I have not tested or validated any of them.

#### On Windows
- Open the solution file (`KotorPatchManager.sln`) in Visual Studio.
- Ensure that the Startup project is `KPatchLauncher`.
- The Build Configuration should have `KotorPatcher` set to `Win32` (KotOR is a 32-bit game). The other projects can be `Any CPU`. This should be the default setting.
- Run `Build > Rebuild Solution` to validate that the build is configured correctly and functioning. If it fails, it's possible that some dependency resolution may have failed or you may not have the necessary `.NET` version. 

With those configurations, you should be able to just run the launcher. As stated above, set the "Game" path to target your game executable (`swkotor.exe`) and the "Patches" path to target the directory where you plan to store your `.kpatch` files.

To build the `.kpatch` file for any patch, simply open the directory (e.g., `cd Patches\AdditionalConsoleCommands`) in a batch-capable terminal (e.g., PowerShell or Command Prompt) and run:
```
..\create-patch.bat
```

This will build and package the patch into a `.kpatch` file.

#### On Linux
There's no Visual Studio; instead you'll need the .NET 8 SDK, MinGW-w64 (`i686-w64-mingw32-g++`, for the 32-bit DLLs the game loads under Wine), and `python3`. The manager builds natively with `dotnet`, and the `build-mingw.sh` script cross-compiles `KotorPatcher.dll` and the `binkw32.dll` proxy. Build a patch with `python3 ../create-patch.py` (the equivalent of `..\create-patch.bat`) from within the patch's directory.

To also target KOTOR II's native Linux build, run `./build-linux.sh`, which compiles `KotorPatcher.so` and stages it beside the launcher. That one needs 32-bit development libraries (`glibc-devel.i686` and `libstdc++-devel.i686` on Fedora, or your distro's equivalents).

#### On macOS
Mac users will need to run:
```
CXX_MAC=clang++ make dylib
dotnet run --project src/KPatchLauncher/KPatchLauncher.csproj
```
Build a patch with `python3 ../create-patch.py` from within the patch's directory.

### Patch Creation
This repository's "Patches" directory contains several example patches, such as the ScriptExtender, AdditionalConsoleCommands, Level-Cap extension, and more. It also contains a directory titled "Common", which has a variety of utilities and classes to aid in the creation of patches.

A patch typically contains 2 to 3 parts:
- a `manifest.toml` file that specifies various patch and compatibility info;
- version-specific `hooks.toml` files that contain the meat and potatoes of the actual patch; and, optionally,
- additional C++ code that gets compiled into the patch and injected, as specified by the hooks.

#### Manifest
- `id`: A program-friendly identifier, expressed with `[a-zA-Z0-9_-]`.
- `name`: A human-friendly identifier that could appear in UIs and docs.
- `version`: The patch's version following `major.minor.patch` format.
- `author`: The patch's creator.
- `description`: A brief description of the patch and what it does.
- `requires`: List of required patches (by `id`) for the patch to work.
- `conflicts`: List of patches (by `id`) that conflict with the patch's functionality.
- `supported_versions`: key/value pair of game versions and their SHA-256s.

#### Hooks
There are currently 4 different types of hooks, `simple`, `replace`, `detour`, and `static`. They all share certain fields.

##### Shared Fields
- `address`: The hexadecimal (`0x########`) address where the hook will be applied.
- `type`: The type of hook, either `"simple"`, `"replace"`, `"detour"`, or `"static"`.

##### Simple Hooks
Simple hooks replace a finite set of bytes with a new set of bytes of the same length. They must specify:

- `original_bytes`: Any number of bytes starting from `address` that will be overwritten.
- `replacement_bytes`: Bytes equal in length to `original_bytes` that will overwrite them.

##### Replace Hooks
Replace hooks allow for more advanced instruction replacement. They will allocate executable memory, write out the specified instructions, and then have the code at `address` jump to those instructions. Use these if you want to replace instructions with more complex logic that wouldn't fit into the existing logic. They require the specification of:

- `original_bytes`: 5 or more bytes, starting from `address`, that will be replaced with a JUMP instruction and NOPs.
- `replacement_bytes`: Any number of bytes that will be jumped to by the above instruction and executed as x86, after which logic will jump back to `address+original_bytes.length`.

##### Detour Hooks
Detour hooks are our most advanced option. They replace the code at `address` with a JUMP to a wrapper that (1) stores register values, (2) prepares parameters, and (3) calls an external function defined and compiled within your patch. Use these for very complex patches, especially those that need to call an existing in-game function, output debug strings, or reference specific addresses. Specify:

- `original_bytes`: 5 or more bytes starting from `address` that will be replaced with a JUMP instruction and NOPs.
- `function`: The name of an exported `extern "C"`/`__cdecl` function that will be compiled and run from your additional C++ code.
- `skip_original_bytes`: If `false`, the `original_bytes` will be re-run after the function finishes execution, making it a true detour. If `true`, the `original_bytes` will never be run.
- `exclude_from_restore`: List of registers to keep modified after hook execution completes.
- `parameters`: Parameters to be passed in (cdecl/stack style) to your `function`:
	- `source`: The register from which the parameter will be sourced.
	- `type`: The type of the parameter. Currently we support: `Int`, `Uint`, `Pointer`, `Float`, `Byte`, and `Short`.

##### Static Hooks
Static hooks are applied directly to the executable file at install-time, before the game runs. This is necessary for patches that modify the PE header or other structures that must be patched before the executable loads into memory. Unlike other hook types, static hooks do not require runtime injection. Specify:

- `original_bytes`: Any number of bytes starting from `address` that will be verified before patching.
- `replacement_bytes`: Bytes equal in length to `original_bytes` that will overwrite the original bytes in the file.
 
#### Building Patches
To build a patch, use the `create-patch.bat` batch file or `create-patch.py` Python script in the Patches/ directory. Usually, from within a patch's specific folder, this will look like:
```
..\create-patch.bat
```
or
```
../create-patch.py
```
#### Additional Patch Files
If your patch is meant to be delivered with additional files (for example, files to be placed in the override folder), these should be put in a directory called `additional` in the patch directory. You can also edit The KPM Mods directory to include your patch in that mod.

### KotorPatcher (C++ DLL)
This C++ project builds the actual DLLs that get injected into the game.

The main entry point is in `dllmain.cpp`, which handles initialization and teardown of the patcher.

The bulk of the business logic lives within `patcher.cpp`, which initializes the version-specific wrapper and parses the `patch_config.toml` that is generated when patches are applied.

For more details about how this system works, see the [KotorPatcher README](src/KotorPatcher/README.md).

### KPatchCore (C# Class Library)
This C# Class Library includes all the necessary functionality to apply patches to the game.
This basically boils down to parsing the patch hooks and manifests, checking compatibility, and building a `patch_config.toml` file.

For more details on this system, see the [KPatchCore README](src/KPatchCore/README.md).

### KPatchLauncher (C# Launcher UI)
The C# Avalonia project is the basic UI that leverages the patching framework, allowing users to apply patches and launch patched games.

For more details on this app, see the [KPatchLauncher README](src/KPatchLauncher/README.md).

## See Also
KOTOR Patch Manager development takes place mainly in the OpenKotOR Discord. If you have any questions or want to work on patching, join:

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

Thanks to Mark Gillard for the `tomlplusplus` project.
