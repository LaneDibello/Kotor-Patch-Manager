#!/usr/bin/env bash
# Build the Linux KotorPatcher.so (i386) and stage it into the launcher's build
# output so the manager finds it beside itself when installing to the Linux
# KOTOR II ELF. Linux counterpart to build-mingw.sh; the compiler flags live in
# src/KotorPatcher/Makefile (the `so` target).
#
# The launcher (KPatchLauncher.csproj) builds to bin/$(Configuration). Without an
# argument this stages into whichever config the launcher has already been built for
# (bin/Debug and/or bin/Release), defaulting to Debug so a fresh `dotnet run` finds
# it. Pass a config name (Debug or Release) to force one. Needs the 32-bit toolchain
# (g++ -m32 with glibc/libstdc++ multilib).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"

if [ "$#" -ge 1 ]; then
    configs=("$1")
else
    configs=()
    for cfg in Debug Release; do
        [ -f "$ROOT/bin/$cfg/KPatchLauncher.dll" ] && configs+=("$cfg")
    done
    [ "${#configs[@]}" -eq 0 ] && configs=(Debug)
fi

# The .so is identical across configs, so build once and copy into any extras.
first="$ROOT/bin/${configs[0]}"
mkdir -p "$first"
make -C "$ROOT/src/KotorPatcher" --no-print-directory so SO_OUT="$first/KotorPatcher.so"

for cfg in "${configs[@]:1}"; do
    dir="$ROOT/bin/$cfg"
    mkdir -p "$dir"
    cp "$first/KotorPatcher.so" "$dir/"
done

echo
echo "staged KotorPatcher.so beside the launcher in: ${configs[*]/#/bin/}"
