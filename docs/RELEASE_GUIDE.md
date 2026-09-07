# Release Guide

There are three release paths, one per host OS the manager runs on. They are
independent and produce differently named artifacts in `releases/`, so they never
collide.

- **Windows** — `publish.bat`, produces `KotorPatchManager-v<version>.zip`
- **Linux** — `publish.sh`, produces `KotorPatchManager-linux-v<version>.tar.gz`
- **macOS** — `publish-macos.sh`, produces `KotorPatchManager-macos-<arch>-v<version>.tar.gz`
  for `x64` and `arm64`

The Linux and macOS releases both build from a Linux (or WSL) host; the "macOS"
in the name is the machine the manager will *run* on, not the machine that builds
it. No Mac is needed to cut a release.

Both stage Windows DLLs beside the manager, which the applicator copies into the
game folder at install time. Those are always Windows binaries because they run
inside the game process — on Linux the game runs under Wine/Proton, so its
in-process DLLs are still Windows PE files. Both stage `KotorPatcher.dll` (the
runtime patcher) and `sqlite3.dll` (imported by GameAPI-based patch DLLs to read
`addresses.db` at runtime — Windows ships `winsqlite3.dll`, not `sqlite3.dll`, so
it must be bundled). The Linux release additionally stages `binkw32.dll` (the
KProxy), because on Linux the manager can't inject and loads the patcher via the
proxy instead; the Windows release injects and does not need it. The difference is
only how the manager itself is built and how the patcher gets loaded (injection on
Windows, the KProxy on Linux). See `src/KProxy/README.md` and `DeploymentPolicy.cs`.

The Linux release stages one extra artifact, `KotorPatcher.so`, for KOTOR II's
native Linux build. That game is an i386 ELF rather than a PE, so it takes a
native module loaded via `DT_NEEDED` instead of a proxy. See
`docs/NATIVE_LINUX.md`.

---

# Windows Release

**Script**: `publish.bat`

**Contains**:
- KPatchLauncher.exe (single self-contained executable)
- KotorPatcher.dll (runtime patcher, staged beside the launcher)
- sqlite3.dll (address-database access for GameAPI patch DLLs, staged beside the launcher)
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
- KotorPatcher.so — the native i386 patcher, staged beside the manager for KOTOR
  II's native Linux build; the manager copies it into the game folder and adds it
  to the executable's `DT_NEEDED` list at install time
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
- `g++` with 32-bit dev libraries — builds `KotorPatcher.so`. The script verifies
  `g++ -m32` can actually link before starting, since a host `g++` without the
  32-bit packages only fails partway through the build
- `python3` — runs `create-patch.py`

On Debian/Ubuntu:
```bash
sudo apt install -y g++-mingw-w64-i686 g++-multilib dotnet-sdk-8.0 python3
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


---

# macOS Release

**Script**: `publish-macos.sh` (run on Linux or WSL — a Mac is not needed)

**Contains**:
- `bin/KPatchLauncher` — the manager, a native macOS build. Same `bin/` layout as
  the Linux release, launched from a terminal rather than Finder (see below)
- `KotorPatcher.dylib` — the native x86_64 patcher for the Aspyr macOS games; the
  manager copies it into the game folder and adds it to the executable's
  `LC_LOAD_DYLIB` list at install time, then re-signs the binary ad hoc
- `KotorPatcher.so`, `KotorPatcher.dll`, `binkw32.dll`, `sqlite3.dll` — the other
  platforms' modules, shipped in every release for the reason below
- `AddressDatabases/` (the `.db` files, beside the manager)
- `create-patch.py` + `create-patch.bat`, example patches, `README.txt`, `LICENSE.txt`

**Why all five modules in one release?** A `.kpatch` is platform-independent and
the manager patches a game of any platform from a host of any platform. Which
module is needed follows the *install the user picks*, not the machine running the
manager, so a Mac user patching a Windows copy under CrossOver gets the KProxy
path and a Mac user patching the Aspyr build gets the dylib. Shipping all of them
costs a few megabytes and removes the question.

## Architectures

Two, and they are not the same question as the game's:

| Piece | Architecture | Why |
| --- | --- | --- |
| `KPatchLauncher` (the manager) | `x64` **and** `arm64`, one archive each | It is an ordinary desktop app and runs native on either machine |
| `KotorPatcher.dylib` (the game-side patcher) | `x86_64` only, identical in both archives | The Aspyr macOS builds are x86_64; Apple Silicon runs them under Rosetta, which loads x86_64 slices |

An arm64 `KotorPatcher.dylib` would need more than a build flag — the wrapper
generator and the `Trampoline` both emit x86 opcodes by hand — and it would have
nothing to load into until Aspyr ship a native build. See the `MAC_ARCHS` comment
in `src/KotorPatcher/Makefile`.

```bash
./publish-macos.sh                  # both architectures (default)
./publish-macos.sh --arch arm64     # just Apple Silicon
./publish-macos.sh --arch x64       # just Intel
```

## Why a folder and not an .app

An `.app` is not a folder with a plist in it. Apple's resource rules — which any
signing tool embeds verbatim — mark everything under `Contents/MacOS` as *nested
code*:

```
'^(Frameworks|SharedFrameworks|PlugIns|...|MacOS|...)/': {'nested': True}
```

A self-contained .NET build puts ~180 managed assemblies, its `.json` configs and
our staged Windows DLLs beside the executable. None are Mach-O, so none can be
sealed in `Contents/MacOS`, and macOS refuses to open a bundle whose seal does not
cover its contents. This was diagnosed the hard way: a bundle whose *executable*
was correctly ad hoc signed still would not open from Finder, while running the
same binary from a terminal worked fine. The signature was never the problem.

Shipping a real bundle therefore means a single-file publish plus moving
`AddressDatabases/` and the staged modules to `Contents/Resources`, which the
manager has to be taught to look in — five `AppContext.BaseDirectory` call sites
across `KPatchCore` and `KPatchLauncher`. That is a legitimate future change; it is
just more than a packaging tweak. Note this is not a quirk of any one signing
tool — it is why Microsoft's own Mac Catalyst bundles put managed assemblies in
`Contents/MonoBundle/` rather than in `MacOS/`.

Until then the release is a plain folder and users run `./bin/KPatchLauncher`.

## Code signing

**The macOS release cannot ship unsigned, folder or not.** Apple Silicon does not merely distrust
an unsigned arm64 image, it refuses to execute one; the process is killed before
`main`. The .NET SDK ad hoc signs the macOS apphost only when the SDK itself is
running on macOS, so a cross-publish from Linux produces an unsigned binary that
dies instantly on every M-series Mac.

`publish-macos.sh` signs it, with `tools/MachOAdHocSign` — a few lines over
`LibObjectFile`, the same library `KPatchCore` re-signs patched game binaries with
(`MachOSigning.cs`), so the two cannot drift apart. No `codesign`, no Mac, no
extra toolchain. `KotorPatcher.dylib` is signed the same way; x86_64 does not
require it, but a signed module costs nothing and settles the question if a
hardened runtime ever turns up.

Signing a *bundle* is a different and larger job — it seals every file in the
bundle, not just the executable — which is what the section above is about. A
loose executable needs only its own signature, and that is what this produces.

Ad hoc is a signature with no certificate: it states only that the image hashes to
what its code directory says, which is what the kernel wants before it will give
the process an identity. It is *not* notarization, so users still clear the
quarantine flag once after downloading — `README.txt` in the release tells them:

```bash
xattr -dr com.apple.quarantine /path/to/the/extracted/folder
```

**Requirements**:
- .NET SDK on `PATH` (override with `DOTNET=/path/to/dotnet`)
- **osxcross** with the macOS SDK — the only thing here a Linux box does not
  already have. `KotorPatcher.dylib` is Mach-O and needs the SDK and `ld64`:
  ```bash
  ./tools/setup-osxcross.sh        # ~20 minutes, needs sudo once
  ```
  It pins `MacOSX14.sdk` from the naev project's build infrastructure, which is
  the SDK the dylib target was developed against. The script finds the compiler on
  `PATH`, in `~/osxcross/target/bin`, or in `/usr/local/lib/osxcross/bin`; override
  with `CXX_MAC=`.
- `i686-w64-mingw32-g++`, `g++` with 32-bit dev libraries, `python3` — the same as
  the Linux release, and for the same artifacts

## Patches

Identical to the Linux release, including `--patches-from`. The patches are built
once and copied into both architectures' archives, since a `.kpatch` is
platform-independent.

Note that no `.kpatch` currently carries a macOS DETOUR module: `create-patch.py`
does not cross-compile one, and every patch with macOS hooks today is
SIMPLE/REPLACEMENT, which needs no module at all. A DETOUR patch for a macOS game
would build its `macos_x86_64.dylib` by hand and drop it in `binaries/`.

## Gotcha: the shared `obj/` directory

`KPatchLauncher.csproj` sets `AppendRuntimeIdentifierToOutputPath=false`, so every
RID shares one `obj/` — **including the customized apphost cached in it**.
Publishing a second RID without clearing that reuses the first one's apphost, and
the failure is silent: a `linux-x64` publish run after an `osx-x64` one ships a
Mach-O binary named `KPatchLauncher`, and vice versa. `publish-macos.sh` removes
`src/KPatchLauncher/obj` and `src/KPatchCore/obj` before each publish for exactly
this reason. Anyone building more than one RID by hand has to do the same.
