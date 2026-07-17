#!/usr/bin/env bash
# Cross-compile the Bink proxy (binkw32.dll) with MinGW-w64.
#
# 32-bit to match the game. Statically linked so it carries no mingw runtime DLL
# dependency. The exports come from bink_forwards.def (PE forwarders to the real
# Bink, renamed binkw32Hooked.dll at install time); DllMain loads KotorPatcher.
set -euo pipefail

cd "$(dirname "$0")"

CXX="${CXX:-i686-w64-mingw32-g++}"
OUT="${1:-binkw32.dll}"

"$CXX" -std=c++17 -shared -O2 -s \
    -static -static-libgcc -static-libstdc++ \
    -DWIN32 \
    kproxy.cpp bink_forwards.def \
    -o "$OUT" \
    -lkernel32

echo "built $OUT"
