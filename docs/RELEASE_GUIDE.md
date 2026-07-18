# Release Guide

There are two release paths, one per host OS the manager runs on. They are
independent and produce differently named artifacts in `releases/`, so they never
collide.

- **Windows** — `publish.bat`, produces `KotorPatchManager-v<version>.zip`
- **Linux** — `publish.sh`, produces `KotorPatchManager-linux-v<version>.tar.gz`

Both stage the *same* three Windows DLLs (`KotorPatcher.dll`, `binkw32.dll`,
`sqlite3.dll`) beside the manager. Those are always Windows binaries because they
run inside the game process — on Linux the game runs under Wine/Proton, so its
in-process DLLs are still Windows PE files. The difference is only how the manager
itself is built and how the patcher gets loaded (injection on Windows, the KProxy
on Linux). See `src/KProxy/README.md` and `DeploymentPolicy.cs`.

---

# Windows Release

**Script**: `publish.bat`

**Contains**:
- KPatchLauncher.exe (single self-contained executable)
- create-patch.bat (for users to create patches)
- Example patches (.kpatch files) - optional
- README.txt

**Size**: ~60-80 MB (with .NET runtime embedded)

## Steps
- Run the script
- Type the version in #.#.# format
- Select 'y' or 'n' to indicate if the pre-built patches should be included
- Allow the script to finish
- You will find the results in the `releases` directory

---

# Linux Release

**Script**: `publish.sh`

**Contains**:
- KPatchLauncher (native linux-x64 self-contained executable)
- KotorPatcher.dll, binkw32.dll (KProxy), sqlite3.dll — the Windows DLLs staged
  beside the manager; the manager copies them into the game folder at install time,
  where they run under Wine/Proton alongside the game
- AddressDatabases/ (the `.db` files, beside the manager)
- create-patch.py + create-patch.bat (for users to create patches)
- Example patches (.kpatch files) - optional
- README.txt

**Why native?** On Linux the manager can't inject into a Wine process. Instead the
KProxy takes the place of the game's `binkw32.dll` and loads `KotorPatcher.dll` when
the game starts. Run the *native* Linux manager, not the Windows manager under
Wine — a Windows build under Wine reports itself as Windows and would wrongly pick
injection.

**Requirements** (run on Linux, or WSL):
- .NET SDK on `PATH` (override with `DOTNET=/path/to/dotnet`)
- `i686-w64-mingw32-g++` (MinGW-w64, i686) — cross-compiles the Windows DLLs and
  DETOUR patch DLLs
- `python3` — runs `create-patch.py`

On Debian/Ubuntu:
```bash
sudo apt install -y g++-mingw-w64-i686 dotnet-sdk-8.0 python3
```

## Steps
- Run `./publish.sh`
- Type the version in #.#.# format
- Select 'y' or 'n' to indicate if the pre-built patches should be included
- Allow the script to finish
- You will find `KotorPatchManager-linux-v<version>.tar.gz` in `releases/`

## Patches: build vs. reuse

By default `publish.sh` cross-compiles patches with MinGW. A handful of DETOUR
patches use MSVC inline assembly (`__asm { ... }`, MASM syntax) that MinGW/GCC
cannot compile, so they are skipped with a `[WARN]` and left out of the release.

Because `.kpatch` files are self-contained and platform-independent, you can
instead **reuse** the ones built on Windows (where MSVC handles that inline asm).
Point `--patches-from` at a Windows release's `patches/` folder:

```bash
# 1. On Windows: build the complete, MSVC-compiled patch set
publish.bat        ->  releases/KotorPatchManager-v<version>/patches/

# 2. On Linux: native manager + Windows DLLs, patches reused from step 1
./publish.sh --patches-from releases/KotorPatchManager-v<version>/patches
```

In reuse mode the `.kpatch` files are copied verbatim from `<dir>`; the
`additional files` folders still come from the source tree, so `<dir>` only needs
the `.kpatch` files. The script errors out early if `<dir>` is missing or has no
`.kpatch` files. Omit the flag to get the default MinGW build.

## Notes
- The manager reads the address databases natively via `Microsoft.Data.Sqlite`
  (its own bundled `e_sqlite3`). The staged `sqlite3.dll` is *only* for the patch
  DLLs on the Wine side; the two SQLite paths are unrelated.
- The release is a self-contained *folder* build (not single-file). The Avalonia
  GUI pulls in native `.so` libraries; a folder loads them in place instead of
  self-extracting to a temp dir on first launch, which is faster and survives a
  `noexec` `/tmp`. Users run `bin/KPatchLauncher`; the rest of `bin/` must ship
  alongside it. To switch to a single-file executable instead, set
  `-p:PublishSingleFile=true -p:IncludeNativeLibrariesForSelfExtract=true` in the
  publish step.
