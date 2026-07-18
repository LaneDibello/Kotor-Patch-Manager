#!/usr/bin/env bash
# =============================================================================
# publish.sh - Linux Release Publisher
# =============================================================================
#
# The Linux twin of publish.bat. It produces a native linux-x64 build of the
# manager and stages beside it the three *Windows* artifacts that get copied
# into the game folder (which runs under Wine/Proton):
#
#   KotorPatcher.dll  <- the runtime patcher      (i686, MinGW cross-compiled)
#   binkw32.dll       <- the KProxy Bink shim      (i686, MinGW cross-compiled)
#   sqlite3.dll       <- imported by patch DLLs     (copied from lib/)
#
# On Linux the manager can't inject into a Wine process, so KProxy takes the
# place of the game's binkw32.dll and loads KotorPatcher when the game starts
# (see src/KProxy/README.md and DeploymentPolicy.cs).
#
# Requirements (run this on Linux, or WSL):
#   - dotnet SDK on PATH        (override with DOTNET=/path/to/dotnet)
#   - i686-w64-mingw32-g++      (MinGW-w64, for the Windows DLLs and DETOUR patches)
#   - python3                   (create-patch.py, for building patches)
#
# The MinGW compiler flags live in the per-component build-mingw.sh scripts and
# in create-patch.py; this script just orchestrates them, mirroring publish.bat.
#
# Usage:
#   ./publish.sh                        build patches with MinGW (default)
#   ./publish.sh --patches-from <dir>   reuse prebuilt .kpatch from <dir> (e.g. a
#                                       Windows release's patches/ folder) instead
set -euo pipefail

INVOKED_FROM="$PWD"
ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"

DOTNET="${DOTNET:-dotnet}"
RID="linux-x64"

# --- Arguments ----------------------------------------------------------------
# --patches-from <dir>: reuse prebuilt .kpatch files from <dir> instead of
# cross-compiling patches with MinGW. .kpatch files are self-contained and
# platform-independent, so a Windows/MSVC release's patches/ folder (which can
# build the MSVC-inline-asm patches MinGW can't) is a valid source here.
PATCHES_FROM=""
while [ "$#" -gt 0 ]; do
    case "$1" in
        --patches-from)
            [ "$#" -ge 2 ] || { echo "  [ERROR] --patches-from needs a directory"; exit 1; }
            PATCHES_FROM="$2"; shift 2 ;;
        --patches-from=*)
            PATCHES_FROM="${1#*=}"; shift ;;
        -h|--help)
            echo "Usage: $0 [--patches-from <dir>]"
            echo
            echo "  --patches-from <dir>  Reuse prebuilt .kpatch files from <dir> instead of"
            echo "                        cross-compiling patches with MinGW. Point it at a"
            echo "                        Windows release's patches/ folder to include patches"
            echo "                        that only MSVC can build (inline asm)."
            exit 0 ;;
        *)
            echo "  [ERROR] Unknown argument: $1 (see --help)"; exit 1 ;;
    esac
done

echo
echo "==================================================="
echo "   KotOR Patch Manager - Linux Release"
echo "==================================================="
echo

# --- Toolchain checks (fail early, like a missing msbuild would on Windows) ----
command -v "$DOTNET" >/dev/null 2>&1 || {
    echo "  [ERROR] '$DOTNET' not found. Install the .NET SDK or set DOTNET=..."
    exit 1
}
command -v i686-w64-mingw32-g++ >/dev/null 2>&1 || {
    echo "  [ERROR] i686-w64-mingw32-g++ not found. Install mingw-w64 (i686)."
    exit 1
}

# Resolve and validate --patches-from before any build work, so a bad path or an
# empty source directory fails fast instead of after a full manager publish.
if [ -n "$PATCHES_FROM" ]; then
    case "$PATCHES_FROM" in
        /*) : ;;                                    # already absolute
        *)  PATCHES_FROM="$INVOKED_FROM/$PATCHES_FROM" ;;
    esac
    [ -d "$PATCHES_FROM" ] || {
        echo "  [ERROR] --patches-from directory not found: $PATCHES_FROM"; exit 1; }
    ls "$PATCHES_FROM"/*.kpatch >/dev/null 2>&1 || {
        echo "  [ERROR] no .kpatch files in: $PATCHES_FROM"; exit 1; }
fi

read -rp "Enter version (#.#.# format): " VERSION
[ -n "$VERSION" ] || VERSION="test-build"

# Absolute paths: the per-component build-mingw.sh scripts cd into their own dir
# before compiling, so any output path handed to them must be absolute.
RELEASE_NAME="KotorPatchManager-linux-v$VERSION"
RELEASE_DIR="$ROOT/releases/$RELEASE_NAME"
BIN="$RELEASE_DIR/bin"

# Clean
rm -rf "$RELEASE_DIR"
mkdir -p "$BIN" "$RELEASE_DIR/tools"

# --- [1/5] Build the Windows DLLs (patcher + KProxy) and stage sqlite3 ---------
echo "[1/5] Building Windows DLLs (MinGW)..."
"$ROOT/src/KotorPatcher/build-mingw.sh" "$BIN/KotorPatcher.dll" >/dev/null
echo "  [OK] KotorPatcher.dll"
"$ROOT/src/KProxy/build-mingw.sh" "$BIN/binkw32.dll" >/dev/null
echo "  [OK] binkw32.dll (KProxy)"
cp "$ROOT/lib/sqlite3.dll" "$BIN/sqlite3.dll"
echo "  [OK] sqlite3.dll"

# --- [2/5] Publish the native manager -----------------------------------------
# Self-contained folder build (not single-file): the Avalonia GUI pulls in native
# .so libraries, and a folder loads them in place instead of self-extracting to a
# temp dir on first launch -- faster cold start, and it survives a noexec /tmp.
# Content files (AddressDatabases) are copied loose beside the exe by the csproj.
echo "[2/5] Publishing KPatchLauncher ($RID)..."
"$DOTNET" publish src/KPatchLauncher/KPatchLauncher.csproj \
    -c Release -r "$RID" --self-contained \
    -p:PublishSingleFile=false \
    -o "$BIN" >/dev/null
# Publish leaves debug symbols behind; drop them from the release.
rm -f "$BIN"/*.pdb

# The csproj links AddressDatabases into the build output, but make it explicit
# so a publish quirk can never ship the manager without its address databases.
echo "  Copying AddressDatabases..."
mkdir -p "$BIN/AddressDatabases"
cp -f "$ROOT"/AddressDatabases/*.db "$BIN/AddressDatabases/"

echo "  [OK] Manager published"

# --- [3/5] Patches ------------------------------------------------------------
echo "[3/5] Patches..."
PATCHES_SRC="$ROOT/Patches"
PATCHES_OUT_NAME="patches"
PATCHES_OUT="$RELEASE_DIR/$PATCHES_OUT_NAME"

read -rp "Include patches? (y/n): " INCLUDE_PATCHES
if [[ "$INCLUDE_PATCHES" =~ ^[Yy]$ ]]; then
    mkdir -p "$PATCHES_OUT"

    if [ -n "$PATCHES_FROM" ]; then
        # Reuse mode: copy the prebuilt, self-contained .kpatch files verbatim.
        echo "  Reusing prebuilt patches from $PATCHES_FROM"
        for kp in "$PATCHES_FROM"/*.kpatch; do
            cp -f "$kp" "$PATCHES_OUT/"
            echo "    [OK] $(basename "$kp")"
        done
    else
        # Build mode: cross-compile each patch with MinGW via create-patch.py,
        # which runs from inside the patch dir and writes the .kpatch to -o.
        echo "  Scanning $PATCHES_SRC for patches with manifest.toml..."
        for dir in "$PATCHES_SRC"/*/; do
            [ -f "$dir/manifest.toml" ] || continue
            name="$(basename "$dir")"
            echo "  Building $name..."
            if ( cd "$dir" && python3 "$PATCHES_SRC/create-patch.py" -o "$PATCHES_OUT" ) >/dev/null 2>&1; then
                echo "    [OK] $name.kpatch"
            else
                echo "    [WARN] No .kpatch produced for $name"
            fi
        done
    fi

    # "additional files" are platform-independent data in the source tree, so ship
    # them from there in both modes (mirroring publish.bat). This also means reuse
    # mode does not depend on the source dir carrying the "additional files" folders.
    for dir in "$PATCHES_SRC"/*/; do
        [ -f "$dir/manifest.toml" ] || continue
        name="$(basename "$dir")"
        if [ -d "$dir/additional" ] && [ -n "$(ls -A "$dir/additional" 2>/dev/null)" ]; then
            addl_dest="$PATCHES_OUT/$name additional files"
            mkdir -p "$addl_dest"
            cp -R "$dir/additional/." "$addl_dest/"
            echo "    [OK] Copied additional files for $name"
        fi
    done
else
    echo "  Skipping patches"
fi

# --- [4/5] Copy tools ---------------------------------------------------------
echo "[4/5] Copying tools..."
cp "$PATCHES_SRC/create-patch.py" "$RELEASE_DIR/tools/"
cp "$PATCHES_SRC/create-patch.bat" "$RELEASE_DIR/tools/"

# --- [5/5] README -------------------------------------------------------------
echo "[5/5] Creating README..."
cat > "$RELEASE_DIR/README.txt" <<EOF
KotOR Patch Manager v$VERSION (Linux)

Contents:
  bin/KPatchLauncher     - Main application (native linux-x64)
  bin/KotorPatcher.dll   - Runtime patcher (loaded inside the game under Wine)
  bin/binkw32.dll        - KProxy: loads the patcher when the game starts
  bin/sqlite3.dll        - Address database access for patch DLLs (Wine side)
  tools/create-patch.py  - Patch creation tool (needs MinGW-w64 for DETOUR patches)
  patches/               - pre-built patches

Quick Start:
  1. Run bin/KPatchLauncher
  2. Point to your KOTOR installation (Steam/GOG under Wine or Proton)
  3. Point to your patch directory of choice
  4. Apply, then launch through Steam or a custom command. Patches load via the
     KProxy (it replaces binkw32.dll; the original is kept as binkw32Hooked.dll).

Created by Lane (Discord: @lane_d)
EOF

# --- Tarball ------------------------------------------------------------------
tar -czf "releases/$RELEASE_NAME.tar.gz" -C releases "$RELEASE_NAME"

echo
echo "SUCCESS! Created releases/$RELEASE_NAME.tar.gz"
echo
