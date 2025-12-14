# KotOR Patch Manager

The KotOR Patch Manager is a dynamic patching framework for Star Wars: Knights of the Old Republic 1 and 2.

This operates off of a runtime DLL-injection scheme which installs the applied patches live. Meaning that the user's EXEs do not get modified, and they can easily "cherry-pick" certain patches they want to use. This also has added benefits for compatibility, version management, and distribution.

The repository contains:
- A selection of pre-made Patches
- A GameAPI and creation script to aid in creation of custom patches
- C++ source for KotorPatcher.dll, the library that is injected at runtime
- C# source for KPatchCore, the framework for parsing and applying these patches
- C# source for KPatchLauncher, a basic UI for using this patching framework (will likely be replaced in the future)

## Install
*NOTE: The patch manger is still in Beta, there are likely going to be various issues within the current release. Please feel free to create GitHub Issues or contact Lane if anything strange comes up*

### Using the Release
If you're just interested in experimenting with the existing patches, then you can just download the [most recent release](https://github.com/LaneDibello/Kotor-Patch-Manager/releases) as a Zip.

This Zip file will contain the following:
- bin
	- AddressDatabases: TOML files containing address information for various kotor versions
	- KotorPatcher.dll: The DLL that we will be injecting
	- KPatchLauncher.exe: Main Launcher Program (must be in the same directory as KotorPatcher.dll and AddressDatabases to function correctly)
- patches
	- `.kpatch` files: These are the patch files that the framework can read in and apply to the game
	- "additional files": These are resources associated with certain patches that may be necessary to make use of them (i.e. modified nwscript.nss for the script extender)
- tools
	- create-patch.bat: Batch file for building `.kpatch` files. This isn't too useful in its current iteration without the added GameAPI source and examples, but it's included nonetheless.
- README.txt: A brief contents and quickstart guide

To use, just run the launcher. Set the "Game" path to target your game executable (i.e. `swkotor.exe`). Set the "Patches" path to target the directory with your `.kpatch` files (i.e. `<release>/patches`).

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

## Usage
Available patches will appear on the left-hand side, with descriptions on the right-hand side. Select the patches you want and select "Apply", to prepare the game for use with those patches. Select "Launch", to run the game with these patches applied.

Patches can be uninstalled by unchecking them and applying, or using the "Uninstall All" button.

## Patches

### Manifest

### Hooks

### Building Patches

## KotorPatcher (C++ DLL)

## KPatchCore (C# Class Library)

### Applicators

### Detectors

### Managers

### Parsers

### Validators

### Models

## KPatchLauncher (C# Launcher UI)

## See Also
You can contact Lane on Discord @lane_d
Related [DeadlyStream thread](https://deadlystream.com/topic/11948-kotor-1-gog-reverse-engineering/)

## Acknowledgements
Special thanks to the KotOR modding community for providing feedback and ideation for various features here.