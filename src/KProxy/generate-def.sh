#!/usr/bin/env bash
# Regenerate bink_forwards.def from one or more real binkw32.dll files.
#
# The .def is only a list of the Bink export names, used to forward calls on to
# the real DLL (binkw32Hooked.dll). No RAD Game Tools code is copied and no Bink
# binary is redistributed; the real Bink stays the user's own file. Pass the
# binkw32.dll from each game you want covered; the export union is written out so
# a single proxy serves both K1 and K2. Needs winedump (from wine).
#
# Usage: ./generate-def.sh <binkw32.dll> [binkw32.dll ...]
set -euo pipefail

cd "$(dirname "$0")"

[ "$#" -ge 1 ] || { echo "usage: $0 <binkw32.dll> [binkw32.dll ...]"; exit 1; }

names=$(for f in "$@"; do winedump -j export "$f"; done 2>/dev/null \
    | grep -oE "_?Bink[A-Za-z0-9_]*@[0-9]+" | sort -u)

{
    echo "LIBRARY binkw32"
    echo "EXPORTS"
    while read -r n; do
        printf '    %s = binkw32Hooked.%s\n' "$n" "$n"
    done <<< "$names"
} > bink_forwards.def

echo "wrote bink_forwards.def ($(wc -l <<< "$names") exports)"
