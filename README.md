# KotOR Patch Manager

The KotOR Patch Manager is a dynamic patching framework for Star Wars: Knights of the Old Republic 1 and 2.

This operates off of a runtime DLL-injection scheme which installs the applied patches live. Meaning that the user's EXEs do not get modified, and they can easily "cherry-pick" certain patches they want to use. This also has added benefits for compatibility, version management, and distribution.

The repository contains:
- A selection of pre-made Patches
- A GameAPI and creation script to aid in creation of custom patches
- C++ source for KotorPatcher.dll, the library that is injected at runtime
- C# source for KPatchCore, the framework for parsing and applying these patches
- C# source for KPatchLauncher, a basic UI for using this patching framework (will likely be replaced in the future)

See road map [here](docs/Roadmap2026.md)

## Install
*NOTE: The patch manager is still in Beta, there are likely going to be various issues within the current release. Please feel free to create GitHub Issues or contact Lane if anything strange comes up*

### Using the Release
If you're just interested in experimenting with the existing patches, then you can just download the [most recent release](https://github.com/LaneDibello/Kotor-Patch-Manager/releases). Windows users want the `.zip` (`KotorPatchManager-v*.zip`); Linux users want the `.tar.gz` (`KotorPatchManager-linux-v*.tar.gz`).

This Zip file will contain the following:
- bin
	- AddressDatabases: SQLite DB files containing address information for various kotor versions
	- KotorPatcher.dll: The DLL that we will be injecting
	- KPatchLauncher.exe: Main Launcher Program (must be in the same directory as KotorPatcher.dll and AddressDatabases to function correctly)
- patches
	- `.kpatch` files: These are the patch files that the framework can read in and apply to the game
	- "additional files": These are resources associated with certain patches that may be necessary to make use of them (i.e. modified nwscript.nss for the script extender)
- tools
	- create-patch.bat: Batch file for building `.kpatch` files. This isn't too useful in its current iteration without the added GameAPI source and examples, but it's included nonetheless.
- README.txt: A brief contents and quick-start guide

To use, just run the launcher. Set the "Game" path to target your game executable (i.e. `swkotor.exe`). Set the "Patches" path to target the directory with your `.kpatch` files (i.e. `<release>/patches`).

**On Linux:** the layout is the same with a few differences. The launcher is a native executable named `KPatchLauncher` (no `.exe`) — run it with `./KPatchLauncher`. The `tools` directory also includes `create-patch.py`, the Linux equivalent of `create-patch.bat`. There are two kinds of Linux install, and the manager picks the right one from the executable you point it at:

- **The Windows game under Wine/Proton.** Because a native app can't inject into a Wine/Proton process, `bin` additionally contains `binkw32.dll` (a small proxy) and `sqlite3.dll`, which the manager stages into the game folder so the patches load when the game starts. Point the "Game" path at the game executable inside your Wine/Proton install (i.e. `swkotor.exe`).
- **KOTOR II's native Linux build (Aspyr).** This is a separate binary, an extensionless `KOTOR2` rather than a `.exe`, so point the "Game" path at that. The manager stages `KotorPatcher.so` and adds it to the game's library dependencies, so the game loads the patcher itself with no proxy and nothing to inject. See [docs/NATIVE_LINUX.md](docs/NATIVE_LINUX.md) for the details and current limitations. KOTOR I has no native Linux port, so this only applies to KOTOR II.

Either way, use "Launch" to start the game through Steam or a custom command.

### Building from Source
If you're interested in contributing or making your own patches, you're going to want to clone this repository and build from source.

This project has been built and configured with Visual Studio 2022, while there are likely alternatives that would function here, I have not tested nor validated any of them.

- Open the solution file (KotorPatchManager.sln) in Visual Studio
- Ensure that the Startup project is `KPatchLauncher`
- The Build Configuration should have `KotorPatcher` set to `Win32` (KotOR is a 32-bit game). The other projects can be `Any CPU`. This should be the default setting.
- Run `Build > Rebuild Solution` to validate that the build is configured correctly and functioning. If it fails it's possible that some dependency resolution may have failed or you may not have the necessary .Net version. 

With this configured you should be able to just run the launcher. You'll want to set the "Game" path to target your game executable (i.e. `swkotor.exe`). And set the "Patches" path to target the directory where you plan to store your `.kpatch` files.

To build the `.kpatch` file for any patch, simply open the directory (i.e. `cd Patches\AdditionalConsoleCommands`) in a batch-capable terminal (i.e. powershell or command-prompt), and run:
```
..\create-patch.bat
```
This will build and package the patch into a `.kpatch` file.

**On Linux:** there's no Visual Studio; instead you'll need the .NET 8 SDK, MinGW-w64 (`i686-w64-mingw32-g++`, for the 32-bit DLLs the game loads under Wine), and `python3`. The manager builds natively with `dotnet`, and the `build-mingw.sh` scripts cross-compile `KotorPatcher.dll` and the `binkw32.dll` proxy. Build a patch with `python3 ../create-patch.py` from within its directory (the equivalent of `..\create-patch.bat`).

To also target KOTOR II's native Linux build, run `./build-linux.sh`, which compiles `KotorPatcher.so` and stages it beside the launcher. That one needs 32-bit development libraries (`glibc-devel.i686` and `libstdc++-devel.i686` on Fedora, or your distro's equivalents).

## Usage
Available patches will appear on the left-hand side, with descriptions on the right-hand side. Select the patches you want and select "Apply", to prepare the game for use with those patches. Select "Launch", to run the game with these patches applied.

A patch that has [options](#options) shows a gear button on its row. Select it to list the patch's options beneath the patch: tick the ones you want, or pick one of a set. Selecting an option shows its description on the right-hand side, with what is selected, what is installed and the default. Options count as changes like ticking a patch does, so select "Apply" afterwards.

Patches can be uninstalled by unchecking them and applying, or using the "Uninstall All" button.

## Patches
This repository's "Patches" directory contains several example patches, such as the ScriptExtender, AdditionalConsoleCommands, Level-Cap extension, and more. In addition to this, it also contains a directory titled "Common", which has a variety of utilities and classes to aid in creation of patches.

A patch typically contains 3 parts:
- A `manifest.toml` file which specifies various patch and compatibility info
- `hooks.toml` file(s) which contain the meat and potatoes of the actual patches
- Additional C++ code which gets compiled into the patch and injected as specified by the hooks

### Manifest
- `id`: A program-friendly identifier with expressed with `[a-zA-Z0-9_-]`
- `name`: A human-friendly identifier that could appear in UIs and docs
- `version`: The patch's version following `major.minor.patch` format
- `author`: The patch's creator
- `description`: A brief description of the patch and what it does
- `requires`: List of requried patches (by `id`) for this patch to work
- `conflicts`: List of patches (by `id`) that conflict with this patch's functionality
- `supported_versions`: key/value pair of game versions and their SHA-256s
- `options`: Choices the player can make inside the patch (optional, see [Options](#options))

#### Options
A patch can offer choices of its own, so that one patch can carry an optional extra or more than one variant instead of being split into several. Each is declared as a `[[patch.options]]` table:
- `id`: A program-friendly identifier with `[a-zA-Z0-9_-]`, unique within the patch
- `name`: The name shown in the launcher
- `description`: What the option changes (optional)
- `type`: `"toggle"` (on or off, the default) or `"choice"` (one of a list)
- `choices`: For a choice, a list of `{ id = "...", name = "..." }`, at least two. An `id` uses `[a-zA-Z0-9_-]` and cannot be `true` or `false`; `name` is what the launcher shows, and is the `id` if omitted
- `default`: `true` or `false` for a toggle (off if omitted); for a choice, the `id` of one of its choices

```toml
[[patch.options]]
id = "map-notes"
name = "Map notes"
type = "toggle"
default = false

[[patch.options]]
id = "hud-style"
name = "HUD style"
type = "choice"
choices = [
    { id = "classic", name = "Classic" },
    { id = "compact", name = "Compact" },
]
default = "classic"
```

Options are resolved when patches are applied. The launcher lists a patch's options beneath the patch (see [Usage](#usage)); on the command line they are set with `--option <patch_id>.<option_id>=<value>`, alongside `--patches` (a toggle takes `true` or `false`; a choice takes one of its ids, spelled as the manifest spells it). An option that is not set takes its default. Changing an option afterwards means applying again, like ticking a patch.

A hook uses an option through its `when` field (see [Shared Fields](#shared-fields)): a hook whose condition does not hold is left out of the install. The chosen values are also written under the patch in `patch_config.toml`, where the patch's own code can read them:

```toml
[[patches]]
id = "my_patch"
dll = "patches/my_patch.dll"

[patches.options]
map-notes = true
hud-style = "classic"
```

**Managers without options.** A release from before this feature ignores `[[patch.options]]` and `when`: it installs every hook of the patch and writes no `[patches.options]` table. So:
- A patch whose hooks all have their own `address` still installs there, as if every toggle were on. Give such toggles `default = true`, and have the patch's code treat a missing table as the defaults.
- A patch with two hooks at one `address` (one per value of an option) is refused there with "Hook conflicts detected", as any two hooks at one address are. If the patch should still install on those releases, keep one hook at the address and choose between the variants in the patch's own code, from the `[patches.options]` table.

### Hooks
There are 4 different types of hooks currently, `simple`, `replace`, `detour`, and `static`. Though they all share certain fields.

#### Shared Fields
- `address`: The hexadecimal (`0x########`) address where the hook will be applied
- `type`: The type of hook, either `"simple"`, `"replace"`, `"detour"`, or `"static"`
- `when`: The [option](#options) this hook depends on (optional). `when = "map-notes"` installs the hook only while that toggle is on; `when = { option = "hud-style", is = "compact" }` only while that option holds that value (`is` may also be `true` or `false`). Hooks may share an `address` only when their `when` tests the same option for different values, so that at most one of them is ever installed

#### Simple Hooks
Simple hooks are for when you just want to replace a finite set of bytes with a new set of bytes of the same length.

- `original_bytes`: Any number of bytes starting from `address` that will be over-written
- `replacement_bytes`: Bytes equal in length to `original_bytes` that will over-write them

#### Replace Hooks
Replace hooks allow for more advanced instruction replacement. They will allocate executable memory and write out the specified instructions, and then have the code at `address` jump to this to this. Use this if you want to replace instructions with more complex logic that wouldn't fit into the existing logic.

- `original_bytes`: 5 or more bytes starting from `address` that will be replaced with a JUMP instruction and NOPs
- `replacement_bytes`: Any number of bytes that will be jumped to by the above and executed as x86, after which logic will jump back to `address+original_bytes.length`

#### Detour Hooks
Detour hooks are our most advanced option. They replace the code at address with a JUMP to a wrapper the stores register values, prepares parameters, and class an external function defined and compiled within your patch. Use this for very complex patches, especially those that need to call existing in-game function, output debug strings, or reference specific addresses.

- `original_bytes`: 5 or more bytes starting from `address` that will be replaced with a JUMP instruction and NOPs
- `function`: The name of an exported `extern "C"`/`__cdecl` function that will be compiled and run from your additional C++ code.
- `skip_original_bytes`: If `false`, the `original_bytes` will be re-run after the function finishes execution, making it a true detour. If `true`, then the `original_bytes` will never be run.
- `exclude_from_restore`: List of registers to keep modified after hook execution completes
- `parameters`: Parameters to be passed in (cdecl/stack style) to your `function`
	- `source`: the register from which the parameter will be sourced
	- `type`: The type of the parameter. Currently we support: `Int`, `Uint`, `Pointer`, `Float`, `Byte`, and `Short`

#### Static Hooks
Static hooks are applied directly to the executable file at install-time, before the game runs. This is necessary for patches that modify the PE header or other structures that must be patched before the executable loads into memory. Unlike other hook types, static hooks do not require runtime injection.

- `original_bytes`: Any number of bytes starting from `address` that will be verified before patching
- `replacement_bytes`: Bytes equal in length to `original_bytes` that will overwrite the original bytes in the file
- **Applied at install-time**: These patches are written directly to the executable file when patches are installed
- **Use cases**: PE header modifications (e.g., 4GB patch), import table modifications, or any change that must occur before the executable loads

### Building Patches
To build a patch, use the `create-patch.bat` batch file from within the patch directory. Usually this will look like:
```
..\create-patch.bat
```

### Additional Patch Files
If your patch is meant to be delivered with additional files, these should be put in a directory called `additional` in the patch directory.

## KotorPatcher (C++ DLL)
This C++ project builds the actual DLL that get's injected into the game.

The main entry point is in `dllmain.cpp`, which handles initialization and tear-down of the patcher.

The bulk of the business logic lives within `patcher.cpp`, which initializes the version specific wrapper and parses in the patch_config.toml that is generated when patches are applied.

For more details about how this system works see the [KotorPatcher README](src/KotorPatcher/README.md).


## KPatchCore (C# Class Library)
This C# Class Library includes all the necessary functionality to apply patches to the game.
This basically boils down to parsing the patch hooks and manifests, checking compatibility, and building a patch_config.toml.

For more details on this system see the [KPatchCore README](src/KPatchCore/README.md).

## KPatchLauncher (C# Launcher UI)
The C# Avalonia project is the basic UI that leverages the patching framework. Allowing for applying patches, and launching patched games.

For more details on this app see the [KPatchLauncher README](src/KPatchLauncher/README.md).

## See Also
KOTOR Patch Manager development takes place mainly in the OpenKotOR discord, if you have any questions or want to work on patching join:

[![OpenKotOR Discord](https://discordapp.com/api/guilds/739590575359262792/widget.png?style=banner2)](https://discord.gg/openkotor)

You can contact Lane on Discord @lane_d

Related [DeadlyStream thread](https://deadlystream.com/topic/11948-kotor-1-gog-reverse-engineering/)

Lane's [YouTube Channel](https://www.youtube.com/@lane_m)

## License
This project is licensed under the MIT License. See [LICENSE](LICENSE) for the full text.

Third-party components keep their own licenses: `tomlplusplus` is MIT, the bundled SQLite is public domain, and the outline of the launcher's options gear is the "cog" icon of [Material Design Icons](https://pictogrammers.com/library/mdi/) (Apache License 2.0).

This project is an unofficial fan work. It is not affiliated with, authorized by, or endorsed by BioWare, Obsidian Entertainment, LucasArts, Aspyr, or Disney. Star Wars: Knights of the Old Republic and all related trademarks are the property of their respective owners. No original game assets or executables are distributed here; you must own a copy of the game.

## Acknowledgements
Special thanks to the KotOR modding community for providing feedback and ideation for various features here.

Thanks to Mark Gillard for the `tomlplusplus` project
