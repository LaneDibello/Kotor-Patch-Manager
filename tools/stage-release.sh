#!/usr/bin/env bash
# Assemble a release tree and its archive from artifacts that are already built.
#
# This is the packaging half of publish.bat / publish.sh / publish-macos.sh with
# the building half removed. Those scripts build and package in one pass on one
# host, which is what a special or one-off build wants. CI builds each piece on
# the host that can produce it and has nothing left to do but lay them out, so it
# calls this instead. The two paths are deliberately parallel implementations:
# the release guide says so, and a change to the contents of a release has to be
# made in both.
#
# No compiler is invoked here. Every input is a path to something already built.
#
#   stage-release.sh --platform linux --version 1.2.3 \
#       --manager  <dir>   the published manager (its contents become bin/)
#       --modules  <dir>   the game-side modules, taken by name per platform
#       --patches  <dir>   .kpatch files and "additional files" folders (optional)
#       --out      <dir>   where the tree and the archive land
#       [--arch x64|arm64] required for macOS, which ships one archive per arch
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"

PLATFORM="" VERSION="" ARCH="" MANAGER="" MODULES="" PATCHES="" OUT=""
while [ "$#" -gt 0 ]; do
    case "$1" in
        --platform) PLATFORM="$2"; shift 2 ;;
        --version)  VERSION="$2";  shift 2 ;;
        --arch)     ARCH="$2";     shift 2 ;;
        --manager)  MANAGER="$2";  shift 2 ;;
        --modules)  MODULES="$2";  shift 2 ;;
        --patches)  PATCHES="$2";  shift 2 ;;
        --out)      OUT="$2";      shift 2 ;;
        -h|--help)  sed -n '2,20p' "$0"; exit 0 ;;
        *) echo "  [ERROR] Unknown argument: $1 (see --help)" >&2; exit 1 ;;
    esac
done

for required in PLATFORM VERSION MANAGER MODULES OUT; do
    [ -n "${!required}" ] || { echo "  [ERROR] --${required,,} is required" >&2; exit 1; }
done
[ -d "$MANAGER" ] || { echo "  [ERROR] --manager not a directory: $MANAGER" >&2; exit 1; }
[ -d "$MODULES" ] || { echo "  [ERROR] --modules not a directory: $MODULES" >&2; exit 1; }

# Which game-side modules each platform ships, and what the archive is called.
# A .kpatch is platform-independent and the manager patches a game of any platform
# from a host of any platform, so these lists are about what the manager can stage
# into a game folder, not about the machine it runs on. They mirror the publish
# scripts exactly, including the Windows release's narrower set.
case "$PLATFORM" in
    windows)
        WANTED=(KotorPatcher.dll binkw32.dll sqlite3.dll)
        RELEASE_NAME="KotorPatchManager-v$VERSION"
        ;;
    linux)
        WANTED=(KotorPatcher.dll binkw32.dll sqlite3.dll KotorPatcher.so)
        RELEASE_NAME="KotorPatchManager-linux-v$VERSION"
        ;;
    macos)
        [ -n "$ARCH" ] || { echo "  [ERROR] --arch is required for macOS" >&2; exit 1; }
        WANTED=(KotorPatcher.dll binkw32.dll sqlite3.dll KotorPatcher.so KotorPatcher.dylib)
        RELEASE_NAME="KotorPatchManager-macos-$ARCH-v$VERSION"
        ;;
    *)
        echo "  [ERROR] --platform must be windows, linux or macos (got: $PLATFORM)" >&2
        exit 1 ;;
esac

RELEASE_DIR="$OUT/$RELEASE_NAME"
BIN="$RELEASE_DIR/bin"

rm -rf "$RELEASE_DIR"
mkdir -p "$BIN" "$RELEASE_DIR/tools"

# --- Manager ------------------------------------------------------------------
# The manager looks beside itself for the modules it stages into the game folder,
# so the whole publish output lands in bin/ and everything else joins it there.
echo "[1/5] Staging the manager..."
cp -R "$MANAGER/." "$BIN/"
# Publish leaves debug symbols behind; drop them from the release.
find "$BIN" -name '*.pdb' -delete

# The csproj links AddressDatabases into the build output, but make it explicit so
# a publish quirk can never ship the manager without its address databases.
mkdir -p "$BIN/AddressDatabases"
cp -f "$ROOT"/AddressDatabases/*.db "$BIN/AddressDatabases/"
echo "  [OK] $(ls -1 "$BIN"/AddressDatabases/*.db | wc -l) address database(s)"

# --- Game-side modules --------------------------------------------------------
# Missing is fatal here, unlike in the publish scripts. There a warning lets a
# developer finish a build on a host without some toolchain; a tagged release has
# no such excuse, and every module was built by a job that had to succeed.
echo "[2/5] Staging the game-side modules..."
for module in "${WANTED[@]}"; do
    [ -f "$MODULES/$module" ] || {
        echo "  [ERROR] $module missing from $MODULES" >&2; exit 1; }
    cp -f "$MODULES/$module" "$BIN/$module"
    echo "  [OK] $module"
done

# --- Patches ------------------------------------------------------------------
# Copied whole: publish-patches.py already put the .kpatch files and the
# "additional files" folders side by side in one directory.
echo "[3/5] Staging the patches..."
if [ -n "$PATCHES" ] && [ -d "$PATCHES" ]; then
    mkdir -p "$RELEASE_DIR/patches"
    cp -R "$PATCHES/." "$RELEASE_DIR/patches/"
    echo "  [OK] $(find "$RELEASE_DIR/patches" -maxdepth 1 -name '*.kpatch' | wc -l) patch(es)"
else
    echo "  Skipping patches"
fi

# --- Tools and LICENSE --------------------------------------------------------
echo "[4/5] Staging the tools..."
cp "$ROOT/Patches/create-patch.bat" "$RELEASE_DIR/tools/"
cp "$ROOT/Patches/create-patch.py" "$RELEASE_DIR/tools/"
cp "$ROOT/LICENSE" "$RELEASE_DIR/LICENSE.txt"
echo "  [OK] tools and LICENSE.txt"

# --- README -------------------------------------------------------------------
echo "[5/5] Writing the README..."
case "$PLATFORM" in
windows)
cat > "$RELEASE_DIR/README.txt" <<EOF
KotOR Patch Manager v$VERSION

Contents:
  bin/KPatchLauncher.exe - Main application
  bin/KotorPatcher.dll   - Runtime patcher loaded into the game
  bin/binkw32.dll        - KProxy: loads the patcher when the game starts,
                           used when Options > "Use library proxy" is on
  bin/sqlite3.dll        - Address database access for GameAPI patch DLLs
  tools/create-patch.bat - Patch creation tool (MSVC; builds the Windows module)
  tools/create-patch.py  - Patch creation tool (any toolchain on PATH; the one
                           that can also build the Linux and macOS modules)
  patches/ - pre-built patches I've been developing with this project
  LICENSE.txt - MIT License

Quick Start:
  1. Run bin/KPatchLauncher.exe
  2. Point to your KOTOR installation
  3. Point to your patch directory of choice
  4. Apply and enjoy!

Deployment (Options menu):
  unchecked - the manager starts the game and injects the patcher
  checked   - KProxy replaces the game's binkw32.dll and loads the patcher
              itself; the original is kept as binkw32Hooked.dll
  The choice is locked while patches are installed.

Created by Lane (Discord: @lane_d)
EOF
;;
linux)
cat > "$RELEASE_DIR/README.txt" <<EOF
KotOR Patch Manager v$VERSION (Linux)

Contents:
  bin/KPatchLauncher     - Main application (native linux-x64)
  bin/KotorPatcher.dll   - Runtime patcher (loaded inside the game under Wine)
  bin/KotorPatcher.so    - Runtime patcher for the native Linux build of KOTOR II
  bin/binkw32.dll        - KProxy: loads the patcher when the game starts
  bin/sqlite3.dll        - Address database access for patch DLLs (Wine side)
  tools/create-patch.py  - Patch creation tool (needs MinGW-w64 for DETOUR patches)
  patches/               - pre-built patches
  LICENSE.txt            - MIT License

Quick Start:
  1. Run bin/KPatchLauncher
  2. Point to your KOTOR installation (Steam/GOG under Wine or Proton)
  3. Point to your patch directory of choice
  4. Apply, then launch through Steam or a custom command. For a Windows build
     under Wine or Proton the KProxy loads the patches (it replaces binkw32.dll;
     the original is kept as binkw32Hooked.dll). The native Linux build of
     KOTOR II instead names KotorPatcher.so in its own dependency list, so the
     loader maps it at startup.

Created by Lane (Discord: @lane_d)
EOF
;;
macos)
cat > "$RELEASE_DIR/README.txt" <<EOF
KotOR Patch Manager v$VERSION (macOS, $ARCH)

Contents:
  bin/KPatchLauncher     - Main application (native osx-$ARCH)
  bin/KotorPatcher.dylib - Runtime patcher for the Aspyr macOS games
  bin/KotorPatcher.so    - Runtime patcher for the native Linux build of KOTOR II
  bin/KotorPatcher.dll   - Runtime patcher for the Windows builds
  bin/binkw32.dll        - KProxy: loads the patcher when a Windows build starts
  bin/sqlite3.dll        - Address database access for patch DLLs (Windows side)
  tools/create-patch.py  - Patch creation tool (needs MinGW-w64 for DETOUR patches)
  patches/               - pre-built patches
  LICENSE.txt            - MIT License

Quick Start:
  1. Keep this folder together. The manager loads the files beside it in bin/.
  2. macOS quarantines anything downloaded from the internet, so clear the flag
     once before the first run:
       xattr -dr com.apple.quarantine /path/to/this/folder
  3. Run the manager from Terminal:
       cd /path/to/this/folder
       ./bin/KPatchLauncher
  4. Point it at your KOTOR installation (the .app bundle itself, or the folder
     holding it), and point it at the patches directory.
  5. Apply, then launch the game through Steam as usual. The Aspyr macOS builds
     name KotorPatcher.dylib in their own dependency list, so the loader maps it
     at startup -- no injector and nothing to run at launch time.

Notes:
  - This is a plain folder rather than a double-clickable .app, so it is launched
    from Terminal. The manager itself is an ordinary window once it is up.
  - This build of the manager is $ARCH. The games themselves are x86_64 and run
    under Rosetta on Apple Silicon; the patcher module that goes into the game is
    x86_64 in both releases for that reason.
  - Patching rewrites the game binary and re-signs it ad hoc. The manager keeps a
    backup and can restore it.

Created by Lane (Discord: @lane_d)
EOF
;;
esac

# --- Archive ------------------------------------------------------------------
# Windows gets a .zip because that is what a Windows user can open with nothing
# installed; the others get a .tar.gz, which preserves the executable bit that
# bin/KPatchLauncher needs and that a zip would drop.
if [ "$PLATFORM" = "windows" ]; then
    ( cd "$OUT" && rm -f "$RELEASE_NAME.zip" && zip -qr "$RELEASE_NAME.zip" "$RELEASE_NAME" )
    ARCHIVE="$RELEASE_NAME.zip"
else
    chmod +x "$BIN/KPatchLauncher"
    tar -czf "$OUT/$RELEASE_NAME.tar.gz" -C "$OUT" "$RELEASE_NAME"
    ARCHIVE="$RELEASE_NAME.tar.gz"
fi

echo
echo "SUCCESS! Created $OUT/$ARCHIVE"
