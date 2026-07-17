#!/usr/bin/env bash
# Build KotorPatcher.dll and the KProxy binkw32.dll with MinGW, and stage sqlite3.dll,
# into the launcher's build output so the manager finds them beside itself. Thin
# wrapper over the per-component build-mingw.sh scripts (src/KotorPatcher, src/KProxy),
# which hold the actual compiler flags.
#
# The launcher (KPatchLauncher.csproj) builds to bin/$(Configuration). Without an
# argument this stages into whichever config the launcher has already been built for
# (bin/Debug and/or bin/Release), defaulting to Debug so a fresh `dotnet run` finds
# them. Pass a config name (Debug or Release) to force one. This matches Windows,
# where the vcxproj and the launcher share bin\$(Configuration).
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

# The MinGW DLLs are identical across configs, so build once and copy into any extras.
first="$ROOT/bin/${configs[0]}"
mkdir -p "$first"
"$ROOT/src/KotorPatcher/build-mingw.sh" "$first/KotorPatcher.dll"
"$ROOT/src/KProxy/build-mingw.sh" "$first/binkw32.dll"
cp "$ROOT/lib/sqlite3.dll" "$first/sqlite3.dll"

for cfg in "${configs[@]:1}"; do
    dir="$ROOT/bin/$cfg"
    mkdir -p "$dir"
    cp "$first"/{KotorPatcher.dll,binkw32.dll,sqlite3.dll} "$dir/"
done

echo
echo "staged KotorPatcher.dll, binkw32.dll, sqlite3.dll beside the launcher in: ${configs[*]/#/bin/}"
