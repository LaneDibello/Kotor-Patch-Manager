#!/bin/zsh
# Builds binaries/macos_x86_64.dylib and packs K1WidescreenPatch.kpatch.
#   build_mac.sh [outdir]      (default: ./build)
# The Aspyr build is x86_64 (Rosetta on Apple Silicon); settings match the
# released dylib: macOS 10.9 minimum, @executable_path install name.
set -eo pipefail
HERE=${0:A:h}
OUT=${1:-$HERE/build}
mkdir -p "$OUT/binaries"
clang++ -arch x86_64 -std=c++17 -O2 -mmacosx-version-min=10.9 -dynamiclib -w \
    -install_name @executable_path/macos_x86_64.dylib \
    -o "$OUT/binaries/macos_x86_64.dylib" "$HERE/mac_widescreen.cpp" "$HERE/kmrp_engine_fixes.cpp"
codesign --force --sign - "$OUT/binaries/macos_x86_64.dylib"
cp -f "$HERE/manifest.toml" "$HERE/kotor1-steam-aspyr-macos.hooks.toml" "$OUT/"
(cd "$OUT" && rm -f K1WidescreenPatch.kpatch && zip -q -X K1WidescreenPatch.kpatch manifest.toml kotor1-steam-aspyr-macos.hooks.toml binaries/macos_x86_64.dylib)
echo "$OUT/K1WidescreenPatch.kpatch"
