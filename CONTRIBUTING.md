# Contributing to KotOR Patch Manager

Thanks for helping out! Bug reports, new patches, ports of existing patches to more game builds, and fixes to the manager itself are all welcome.

If you're not sure where to start, browse the [open issues](https://github.com/LaneDibello/Kotor-Patch-Manager/issues) or the [roadmap](docs/Roadmap2026.md). For anything large, open an issue first so the approach can be discussed before you write a lot of code.

## Repository layout

| Path | What it is |
| --- | --- |
| `src/KotorPatcher/` | C++ engine injected into the game; installs the hooks at runtime ([README](src/KotorPatcher/README.md)) |
| `src/KPatchCore/` | C# library that validates patches, checks game versions and writes `patch_config.toml` ([README](src/KPatchCore/README.md)) |
| `src/KPatchLauncher/` | Avalonia UI and CLI |
| `src/KProxy/` | `binkw32.dll` proxy that loads the patcher under Wine/Proton |
| `AddressDatabases/` | One SQLite database per supported executable ([docs](docs/AddressDatabaseSystem.md)) |
| `Patches/` | One directory per patch, plus shared helpers in `Patches/Common/` |
| `tools/` | `validate-patches.py`, `SqliteTools`, Ghidra export scripts |

For build setup on Windows, Linux and macOS, see the [README](README.md#building-from-source).

## Anatomy of a patch

```
Patches/MyPatch/
├── manifest.toml                     # required
├── hooks.toml                        # or one <version>.hooks.toml per game build
├── MyPatch.cpp                       # only for detour hooks
├── README.md                         # recommended for non-trivial patches
└── additional/                       # optional loose files shipped with the release
```

The [README](README.md#patches) is the reference for every manifest and hook field. Parameter sources and types for detour hooks are documented in the [KotorPatcher README](src/KotorPatcher/README.md#parameterinfo).

### `manifest.toml`

```toml
[patch]
id = "my_patch"               # [a-zA-Z0-9_-], must be unique
name = "My Patch"             # shown in the launcher
version = "1.0.0"             # major.minor.patch
author = "YourName"
description = "One or two sentences on what the patch does, shown in the launcher."

requires = []                 # ids of patches this one needs
conflicts = []                # ids of patches it can't be installed with

[patch.supported_versions]
kotor2_steam_aspyr = "6A522E71631DCEE93467BD2010F3B23D9145326E1E2E89305F13AB104DBBFFEF"
```

Each `supported_versions` hash has to match a build that's recorded in `AddressDatabases/`. Copy the hashes from an existing manifest rather than typing them out.

### Version-specific hook files

A patch that supports more than one build keeps one hooks file per build (for example, `kotor2-gog-aspyr.hooks.toml` and `kotor2-steam-aspyr.hooks.toml`). Each file names its build in a `[metadata]` table:

```toml
[metadata]
target_versions = ["6A522E71631DCEE93467BD2010F3B23D9145326E1E2E89305F13AB104DBBFFEF"]

# What this site is, and why the hook goes here.
[[hooks]]
address = 0x004748BA
type = "detour"
function = "MyHook"
original_bytes = [0xE8, 0x11, 0xF1, 0x3B, 0x00]   # at least 5 for detour/replace
skip_original_bytes = false
exclude_from_restore = []

[[hooks.parameters]]
source = "[esp+4]"
type = "int"
```

When you port a patch to another build, the usual change is a new `<version>.hooks.toml` and a new manifest entry. Leave the existing hook files untouched. Before you commit `original_bytes`, check them against that build's executable.

### C++ for detour hooks

Export each hook function as `extern "C"` and `__cdecl`:

```cpp
extern "C" void __cdecl MyHook(int value)
{
    // ...
}
```

`create-patch.py` generates `exports.def` from these signatures if the patch doesn't commit one. Shared helpers and engine wrappers live in `Patches/Common/`.

The code has to build with MSVC and MinGW for Windows, and with GCC/Clang wherever the patch targets the native Linux or macOS builds. Guard compiler- and platform-specific code accordingly.

## Before opening a pull request

Run the checks that CI runs:

```bash
python3 tools/validate-patches.py                      # manifest and hook metadata (Python 3.11+)
(cd Patches/MyPatch && python3 ../create-patch.py)     # build and package your patch
dotnet test KotorPatchManager.Managed.slnf -c Release  # only if you changed the manager
```

CI also builds every patch with MSVC, MinGW, Linux and macOS toolchains. If you lack a toolchain locally, CI will catch build breaks on that platform.

Then test in game on every build you claim to support. At a minimum, load a save, change areas and enter combat.

## Pull request guidelines

- **Keep each PR focused.** One patch, one port or one fix per PR, so each can be reviewed and reverted on its own.
- **Use standard branch prefixes.** Branch from an up-to-date `master` using prefixes established in the repository:
  - `patch_add/<name>` — adding a brand new patch (e.g. `patch_add/k2classmenuclothes`)
  - `patch_update/<name>` or `port/<name>` — updating an existing patch or porting to another build (e.g. `patch_update/refmodelstealthfix`)
  - `feat/<feature>` — adding new capabilities to the manager, patcher engine, or launcher (e.g. `feat/post-combat-movement-macos`)
  - `fix/<issue>` — fixing a bug in the manager, proxy, or existing patch logic
  - `docs/<topic>` — documentation additions and updates (e.g. `docs/contributing-guide`)
- **Title it the way the history does.** Start with the area, then say what the change does: `K2FinesseMeleeFix: Support the Steam build`, `patcher: Parse stack offsets one way`.
- **Explain the change.** Describe the vanilla behaviour, why it happens and how the hooks change it. Link related issues (`Closes #123`).
- **List what you tested:** which game builds and platforms, and what you did in game.
- **Add screenshots or recordings for visual changes.** Attach them to the PR description rather than committing them.
- **Bump the patch's `version`** when you change an existing patch.
- **Give non-trivial patches a `README.md`** that covers the hook sites, the reasoning behind them and any configuration (see `Patches/FrameLimiter/` or `Patches/AdditionalConsoleCommands/`).
- **Don't commit build output:** `.kpatch` files and compiled binaries stay out of the repository.

## Reporting bugs

Open an issue with:

- the game and build (Steam, GOG, native Linux or macOS) and your OS
- which patches were installed
- what happened and how to reproduce it
- any patcher log output and crash details

## License

By contributing, you agree that your contributions are licensed under the project's [LICENSE](LICENSE).
