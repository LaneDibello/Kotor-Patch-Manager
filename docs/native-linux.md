# Native Linux support (KOTOR II)

KOTOR II ships a native Linux build (the Aspyr port). It is a separate binary
from the Windows game: a 32-bit i386 ELF, not a PE, so none of the Windows
tooling maps onto it directly. This document covers how the patch manager
delivers the patcher into that native process.

KOTOR I has no native Linux port, so this path is KOTOR II only. Running the
Windows games under Wine/Proton is a different story (see the KProxy path); this
document is strictly about the native ELF.

## How the patcher gets loaded

The manager has three ways to load `KotorPatcher` into a game, chosen by
`DeploymentPolicy`:

| Method      | Target                     | How the patcher loads                     |
|-------------|----------------------------|-------------------------------------------|
| `Injection` | Windows PE (native)        | Manager injects `KotorPatcher.dll`        |
| `Proxy`     | Windows PE under Wine/Proton | Staged KProxy loads `KotorPatcher.dll`    |
| `ElfNeeded` | Native Linux ELF           | Loader maps `KotorPatcher.so` via DT_NEEDED |

For the native ELF the manager adds `KotorPatcher.so` to the game
executable's `DT_NEEDED` list. The dynamic loader then maps it at startup,
before the game's entry point runs, no matter who launches the game (Steam, a
shortcut, the manager). No injector and no proxy are involved. This is the
address-preserving edit performed by `ElfInjector` on top of the LibObjectFile
fork: `.dynamic` and `.dynstr` are relocated into a fresh `PT_LOAD` built from a
spare `PT_NOTE`, so every existing code address, symbol, and relocation stays
valid.

The ELF is `ET_EXEC` with a fixed load base (no ASLR slide), which is what lets
the patcher use the absolute addresses from `patch_config.toml` verbatim.

## Building KotorPatcher.so

The engine is a shared core behind a platform seam, so one tree builds both the
Windows DLL and the Linux shared object:

```
cd src/KotorPatcher
make so     # -> build/KotorPatcher.so (32-bit i386)
make        # both KotorPatcher.dll (MinGW) and KotorPatcher.so
```

The native build needs 32-bit development libraries (`glibc-devel.i686`,
`libstdc++-devel.i686` on Fedora, or the distro equivalents). `libstdc++` and
`libgcc` are linked statically so the module does not depend on the host's C++
runtime version.

## Install

Installing to a native ELF stages the same runtime inputs as the Windows path,
minus anything PE-specific:

1. A backup of the pristine executable (`KOTOR2.backup.<timestamp>`).
2. `KotorPatcher.so` added to the game's `DT_NEEDED` list.
3. `KotorPatcher.so` copied into the game directory (resolved at load time via
   the game's existing `RPATH [.]`).
4. `patch_config.toml` with the resolved hook addresses and bytes.
5. `addresses.db` for the detected game version.

The engine reads only `patch_config.toml`, so `KotorPatcher.so` links no SQLite.
The `addresses.db` is consumed by the manager at apply time and by a patch's own
GameAPI at runtime, not by the engine.

STATIC hooks (install-time byte patches to the executable) are applied directly
to the ELF, mapping each hook's virtual address to a file offset through the
PT_LOAD program headers. They run before the `DT_NEEDED` injection, which is
address-preserving, so the patched bytes survive that rewrite. `Patches/EnableCheats`
is the reference native-Linux static patch.

## Launch

Launch KOTOR II through Steam (app `208580`); the Linux depot is the native
build. The manager's launch path recognises the `KOTOR2` executable and starts
it through Steam. The game self-loads the patcher via `DT_NEEDED`, so there is
nothing to inject.

Direct (non-Steam) launch may fail on a system that is missing 32-bit runtime
libraries the game expects (for example `libGLU.so.1`); the Steam runtime
provides them. When in doubt, launch through Steam.

## Logs

The patcher logs to stderr, matching the Windows backend (which uses
`OutputDebugString`): nothing is written to disk by default. Steam redirects
stderr away, so to capture a run set `KPATCH_LOG` to a file path, for example
`KPATCH_LOG=/tmp/kp.log %command%` in the Steam launch options. The file is
truncated per launch. A successful run looks like:

```
[KotorPatcher] Loading config from: ./patch_config.toml
[Config] Loaded SIMPLE hook: <id> @ 0x081908E8 (5 bytes)
[KotorPatcher] Applied SIMPLE hook at 0x081908E8 (5 bytes replaced)
[KotorPatcher] patcher initialized
```

## Uninstall

Uninstall restores the backup, which reverts the `DT_NEEDED` edit byte-for-byte,
then removes the staged files (`KotorPatcher.so`, `patch_config.toml`,
`addresses.db`). If no backup is available the module is left in place so the
game still launches (the loader still needs it); it is inert without
`patch_config.toml`.

## Current limitations

- DETOUR hooks need a Linux-built patch module. Released `.kpatch` archives ship
  a Windows `windows_x86.dll` only, so SIMPLE, REPLACE, and STATIC hooks (which
  need no patch binary) are what work today without a separate Linux patch build.
