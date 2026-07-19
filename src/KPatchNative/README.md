# KPatchNative (libkpatch.so)

The native Linux loader for the patcher runtime, targeting the **Aspyr KOTOR2**
build, which ships as an i386 ELF. The Windows `KotorPatcher.dll` and its
byte-address hooks do not apply to that binary (different executable, different
addresses, System-V calling convention), so this is a separate native track.

## How it loads

`libkpatch.so` is dropped in the game directory and added to the game
executable's `DT_NEEDED` list, so the dynamic loader maps it at startup, before
the game's entry point runs. The manager performs that edit with the LibObjectFile
fork's `ElfDynamicEditing.AddNeededLibrary` (append-and-repoint, address
preserving, no `patchelf` needed). The game already carries `RPATH [.]` and loads
its own bundled `.so` files from the game folder, so the bare name resolves.

The `__attribute__((constructor))` in `libkpatch.cpp` is our in-process entry
point. Right now it only records that it loaded; reading the patch config and
applying hooks comes later.

## Build

```sh
./build-native.sh            # -> libkpatch.so
```

32-bit to match the game; statically linked against libstdc++/libgcc so it has no
C++ ABI version dependency on the game's bundled libraries. Needs the 32-bit dev
libraries (`glibc-devel.i686`, `libstdc++-devel.i686` on Fedora).

## Smoke test

```sh
LD_PRELOAD=./libkpatch.so KPATCH_LOG=/tmp/libkpatch.log /path/to/any/32-bit/program
cat /tmp/libkpatch.log        # -> [libkpatch] loaded into pid <n>
```

`KPATCH_LOG` overrides the log path (default `/tmp/libkpatch.log`), since a game's
stderr is usually not visible under Steam.
