#!/usr/bin/env bash
# Build the native Linux loader (libkpatch.so) with the system compiler.
#
# 32-bit to match the Aspyr KOTOR2 ELF (i386). Statically linked against libstdc++
# and libgcc so it carries no runtime C++ ABI version dependency on the game's own
# bundled libraries. Needs 32-bit dev libs (glibc-devel.i686, libstdc++-devel.i686).
set -euo pipefail

cd "$(dirname "$0")"

CXX="${CXX:-g++}"
OUT="${1:-libkpatch.so}"

"$CXX" -std=c++17 -m32 -shared -fPIC -O2 -s \
    -static-libstdc++ -static-libgcc \
    libkpatch.cpp \
    -o "$OUT"

echo "built $OUT"
