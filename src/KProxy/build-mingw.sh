#!/usr/bin/env bash
# Cross-compile the Bink proxy (binkw32.dll) with MinGW-w64.
#
# 32-bit to match the game. Statically linked so it carries no mingw runtime DLL
# dependency. The exports come from bink_forwards.def (PE forwarders to the real
# Bink, renamed binkw32Hooked.dll at install time); DllMain loads KotorPatcher.
set -euo pipefail

cd "$(dirname "$0")"

# Prefer MinGW's posix threading variant when present (Debian/Ubuntu ship it as a
# separate binary; the shared patcher core uses std::thread, which the win32 model
# lacks). Falls back to the plain name where the default is already posix (Fedora).
CXX="${CXX:-$(command -v i686-w64-mingw32-g++-posix 2>/dev/null || echo i686-w64-mingw32-g++)}"
OUT="${1:-binkw32.dll}"

"$CXX" -std=c++17 -shared -O2 -s \
    -static -static-libgcc -static-libstdc++ \
    -DWIN32 \
    kproxy.cpp bink_forwards.def \
    -o "$OUT" \
    -lkernel32

echo "built $OUT"
