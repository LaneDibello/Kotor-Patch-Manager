# KProxy

KProxy is a small `binkw32.dll` proxy that loads `KotorPatcher.dll` on Linux,
where the manager can't inject it the way it does on Windows. This can also be
used on Windows, but the manager's injection method is the only wired way to do it.

## Why

KOTOR 1 and 2 statically import `binkw32.dll` (RAD's Bink video). Under
Wine/Proton there is no builtin `binkw32`, so the wine loader loads this DLL from
the game folder as part of the game's own startup, before the entry point runs.
That gives an early hook that does not depend on who launched the game (Steam,
Proton, a shortcut, or the manager) and needs no injection into a running
process. `DllMain` then loads `KotorPatcher.dll`, which applies the runtime hooks
from `patch_config.toml`.

On Windows the manager injects `KotorPatcher.dll` directly and KProxy is not used.

## How it forwards Bink

`bink_forwards.def` forwards every Bink export to `binkw32Hooked.dll`, so video
still works. At install time the game's real `binkw32.dll` is renamed to
`binkw32Hooked.dll` and KProxy takes its place.

The `.def` is only a list of export names for forwarding (interoperability). No
RAD code is copied and no Bink binary is redistributed; the real Bink remains
the user's own `binkw32Hooked.dll`.

## Build

```
./build-mingw.sh              # produces binkw32.dll (needs mingw-w64, i686)
```

## Regenerate the forwarder list

The committed `bink_forwards.def` covers the union of K1 and K2 exports. To
regenerate it from real DLLs (needs winedump from wine):

```
./generate-def.sh /path/to/kotor1/binkw32.dll /path/to/kotor2/binkw32.dll
```

## Install layout (per game folder)

```
binkw32.dll          <- KProxy
binkw32Hooked.dll    <- the game's original Bink, renamed
KotorPatcher.dll     <- the patcher (loaded by KProxy)
patch_config.toml    <- hooks to apply
```
