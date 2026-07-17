#!/usr/bin/env bash
# Cross-compile KotorPatcher.dll on Linux/macOS with MinGW-w64.
#
# The DLL is injected into the 32-bit game, so it must be i686. libstdc++/libgcc
# are linked statically so the result carries no mingw runtime DLL dependency.
# sqlite is not linked: the core patcher never calls it (only the per-patch DLLs
# use it, through their own GameAPI build).
set -euo pipefail

cd "$(dirname "$0")"

CXX="${CXX:-i686-w64-mingw32-g++}"
OUT="${1:-KotorPatcher.dll}"

# -s strips symbols: without it the static libstdc++ + toml++ instantiations
# balloon the DLL past 12 MB; stripped it is closer to 1.4 MB.
"$CXX" -std=c++17 -shared -O2 -s \
    -static -static-libgcc -static-libstdc++ \
    -DWIN32 -DNDEBUG -DKOTORPATCHER_EXPORTS -D_WINDOWS -D_USRDLL -DUNICODE -D_UNICODE \
    -I include -I include/wrappers -I external \
    src/config_reader.cpp \
    src/dllmain.cpp \
    src/patcher.cpp \
    src/trampoline.cpp \
    src/wrappers/wrapper_x86_win32.cpp \
    -o "$OUT" \
    -lkernel32

echo "built $OUT"
