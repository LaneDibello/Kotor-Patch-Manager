#!/usr/bin/env bash
# =============================================================================
# publish-macos.sh - macOS Release Publisher
# =============================================================================
#
# The macOS twin of publish.sh. It produces a native macOS build of the manager
# and stages beside it the artifacts that get copied into the game folder. Which
# ones apply depends on the install the user points it at:
#
#   KotorPatcher.dylib <- the runtime patcher   (x86_64 Mach-O, osxcross)
#   KotorPatcher.so    <- the runtime patcher   (i386 ELF, built natively)
#   KotorPatcher.dll   <- the runtime patcher   (i686, MinGW cross-compiled)
#   binkw32.dll        <- the KProxy Bink shim  (i686, MinGW cross-compiled)
#   sqlite3.dll        <- imported by patch DLLs (copied from lib/)
#
# All five ship in every build. A .kpatch is platform-independent and the manager
# patches a game of any platform from a host of any platform, so which module is
# needed is decided by the install the user picks, not by the machine running the
# manager: the Aspyr macOS games take the dylib, a Windows copy under CrossOver
# takes the DLL through the KProxy, and the KOTOR II Linux build takes the .so.
#
# Two architectures, and they are not the same question as the game's. The Aspyr
# macOS games are x86_64 only, so KotorPatcher.dylib is x86_64 and Apple Silicon
# runs the game under Rosetta. The manager is a separate process and runs native
# either way, so it is published once per architecture and the identical dylib
# goes into both.
#
# The layout is the Linux release's, a plain bin/ directory, rather than an .app
# bundle. An .app is not just a folder with a plist: Apple's resource rules treat
# everything under Contents/MacOS as nested code, so the ~180 managed assemblies a
# self-contained .NET build puts beside its executable cannot be sealed there, and
# macOS refuses to open a bundle whose seal does not cover its contents. Shipping
# a bundle therefore means a single-file publish and moving the address databases
# and staged modules to Contents/Resources, which the manager would have to be
# taught to look in. That is a real option, and this is not it: the folder needs no
# such surgery and users launch it from a terminal instead of Finder.
#
# The executable is still ad hoc signed, which is not optional -- Apple Silicon
# refuses to execute an unsigned image whether or not it sits in a bundle.
#
# Requirements (run this on Linux, or WSL):
#   - dotnet SDK on PATH        (override with DOTNET=/path/to/dotnet)
#   - osxcross                  (for KotorPatcher.dylib; ./tools/setup-osxcross.sh)
#   - i686-w64-mingw32-g++      (MinGW-w64, for the Windows DLLs and DETOUR patches)
#   - g++ with 32-bit dev libs  (for KotorPatcher.so)
#   - python3                   (create-patch.py, for building patches)
#
# Usage:
#   ./publish-macos.sh                        both architectures, build patches
#   ./publish-macos.sh --arch arm64           just Apple Silicon
#   ./publish-macos.sh --patches-from <dir>   reuse prebuilt .kpatch from <dir>
set -euo pipefail

INVOKED_FROM="$PWD"
ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"

DOTNET="${DOTNET:-dotnet}"

# --- Arguments ----------------------------------------------------------------
# --patches-from <dir>: reuse prebuilt .kpatch files from <dir> instead of
# cross-compiling patches with MinGW. .kpatch files are self-contained and
# platform-independent, so a Windows/MSVC release's patches/ folder (which can
# build the MSVC-inline-asm patches MinGW can't) is a valid source here.
PATCHES_FROM=""
ARCHS="x64 arm64"
while [ "$#" -gt 0 ]; do
    case "$1" in
        --patches-from)
            [ "$#" -ge 2 ] || { echo "  [ERROR] --patches-from needs a directory"; exit 1; }
            PATCHES_FROM="$2"; shift 2 ;;
        --patches-from=*)
            PATCHES_FROM="${1#*=}"; shift ;;
        --arch)
            [ "$#" -ge 2 ] || { echo "  [ERROR] --arch needs a value"; exit 1; }
            ARCHS="$2"; shift 2 ;;
        --arch=*)
            ARCHS="${1#*=}"; shift ;;
        -h|--help)
            echo "Usage: $0 [--arch x64|arm64|both] [--patches-from <dir>]"
            echo
            echo "  --arch <a>            Which architecture to build the manager for."
            echo "                        'both' (the default) makes one archive each."
            echo "  --patches-from <dir>  Reuse prebuilt .kpatch files from <dir> instead of"
            echo "                        cross-compiling patches with MinGW. Point it at a"
            echo "                        Windows release's patches/ folder to include patches"
            echo "                        that only MSVC can build (inline asm)."
            exit 0 ;;
        *)
            echo "  [ERROR] Unknown argument: $1 (see --help)"; exit 1 ;;
    esac
done

case "$ARCHS" in
    both) ARCHS="x64 arm64" ;;
    x64|arm64|"x64 arm64"|"arm64 x64") : ;;
    *) echo "  [ERROR] --arch must be x64, arm64 or both (got: $ARCHS)"; exit 1 ;;
esac

echo
echo "==================================================="
echo "   KotOR Patch Manager - macOS Release"
echo "==================================================="
echo

# --- Toolchain checks (fail early, before any build work) ---------------------
command -v "$DOTNET" >/dev/null 2>&1 || {
    echo "  [ERROR] '$DOTNET' not found. Install the .NET SDK or set DOTNET=..."
    exit 1
}
command -v i686-w64-mingw32-g++ >/dev/null 2>&1 || {
    echo "  [ERROR] i686-w64-mingw32-g++ not found. Install mingw-w64 (i686)."
    exit 1
}
# std::thread / std::this_thread (the patcher's deferred-apply path) exist only in
# MinGW's posix threading model. The Debian/Ubuntu default is win32, where <thread>
# compiles but defines nothing and the DLL build fails deep in compilation. The build
# prefers the posix variant when present (see the Makefile and KProxy build-mingw.sh),
# so check that same compiler here and fail fast with a fix on a win32-only host.
MINGW_CXX="$(command -v i686-w64-mingw32-g++-posix 2>/dev/null || echo i686-w64-mingw32-g++)"
printf '#include <thread>\n#include <chrono>\nvoid f(){ std::thread t; std::this_thread::sleep_for(std::chrono::milliseconds(1)); }\n' \
    | "$MINGW_CXX" -std=c++17 -fsyntax-only -x c++ - >/dev/null 2>&1 || {
    echo "  [ERROR] MinGW ($MINGW_CXX) lacks std::thread (win32 threading model)."
    echo "          Install and select the posix variant, which provides it:"
    echo "            sudo apt install g++-mingw-w64-i686-posix"
    echo "            sudo update-alternatives --set i686-w64-mingw32-g++ /usr/bin/i686-w64-mingw32-g++-posix"
    exit 1
}
# KotorPatcher.so needs a working 32-bit native toolchain, which a plain `command -v g++`
# does not prove: a host g++ without the 32-bit dev libraries only fails at link time.
printf 'int main(){return 0;}\n' | g++ -m32 -x c++ - -o /dev/null >/dev/null 2>&1 || {
    echo "  [ERROR] g++ cannot build 32-bit binaries (needed for KotorPatcher.so)."
    echo "          Install the 32-bit dev libraries: glibc-devel.i686 and"
    echo "          libstdc++-devel.i686 on Fedora, gcc-multilib and g++-multilib on Debian."
    exit 1
}
# The Mach-O compiler is looked for the way the Makefile looks for it, plus the
# location tools/setup-osxcross.sh installs to, and then handed to make explicitly
# so a toolchain in a home directory needs nothing on PATH.
CXX_MAC="${CXX_MAC:-$(command -v o64-clang++ 2>/dev/null \
    || ls "$HOME/osxcross/target/bin/o64-clang++" 2>/dev/null \
    || ls /usr/local/lib/osxcross/bin/o64-clang++ 2>/dev/null \
    || true)}"
[ -n "$CXX_MAC" ] && [ -x "$CXX_MAC" ] || {
    echo "  [ERROR] No macOS cross compiler (o64-clang++) found."
    echo "          KotorPatcher.dylib is Mach-O and needs the macOS SDK and ld64."
    echo "          Install the toolchain with:"
    echo "            ./tools/setup-osxcross.sh"
    echo "          or point CXX_MAC at an existing o64-clang++."
    exit 1
}
# lipo lives beside the compiler in an osxcross install. Only used when the dylib
# is built for more than one architecture, but resolved here with everything else.
LIPO="${LIPO:-$(command -v lipo 2>/dev/null || echo "$(dirname "$CXX_MAC")/lipo")}"

# The compiler needs its own bin directory on PATH, not just its own path known:
# it spawns the target's ld by name, and without that directory in reach it finds
# the host /usr/bin/ld instead. That failure is a confusing one -- every source
# compiles to Mach-O and only the link fails, complaining about linker flags -- so
# it is worth being deliberate about.
#
# The lib directory goes with it. osxcross links ld64 against its own libxar and
# libtapi and records where to find them as a RUNPATH holding the absolute path of
# the directory it was built in, so a toolchain installed by copying a build made
# somewhere else still names the directory it came from and the linker will not
# start. Naming it here covers that without the machine having been told through
# ldconfig.
#
# Both are scoped to the one command that needs them rather than exported, so
# nothing else in the build picks up a foreign linker or libstdc++.
OSXCROSS_BIN="$(cd "$(dirname "$CXX_MAC")" && pwd)"
OSXCROSS_LIB="$(cd "$OSXCROSS_BIN/.." && pwd)/lib"
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


read -rp "Include patches? (y/n): " INCLUDE_PATCHES

# --- Shared runtime artifacts --------------------------------------------------
# Identical in every architecture's release, so they are built once into a staging
# directory and copied into each bundle. The dylib is x86_64 whatever the manager
# is, because it loads into the game and the games are x86_64.
STAGING="$ROOT/releases/.macos-staging"
rm -rf "$STAGING"
mkdir -p "$STAGING"

STEPS=$(( 2 + $(echo "$ARCHS" | wc -w) ))
echo "[1/$STEPS] Building runtime artifacts..."
"$ROOT/src/KotorPatcher/build-mingw.sh" "$STAGING/KotorPatcher.dll" >/dev/null
echo "  [OK] KotorPatcher.dll"
"$ROOT/src/KProxy/build-mingw.sh" "$STAGING/binkw32.dll" >/dev/null
echo "  [OK] binkw32.dll (KProxy)"
cp "$ROOT/lib/sqlite3.dll" "$STAGING/sqlite3.dll"
echo "  [OK] sqlite3.dll"
make -C "$ROOT/src/KotorPatcher" --no-print-directory so SO_OUT="$STAGING/KotorPatcher.so" >/dev/null
echo "  [OK] KotorPatcher.so (native Linux)"
env PATH="$OSXCROSS_BIN:$PATH" LD_LIBRARY_PATH="$OSXCROSS_LIB${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
    make -C "$ROOT/src/KotorPatcher" --no-print-directory dylib \
    CXX_MAC="$CXX_MAC" LIPO="$LIPO" DYLIB_OUT="$STAGING/KotorPatcher.dylib" >/dev/null
# Ad hoc signed for the same reason the manager is: an unsigned image is refused
# outright on Apple Silicon. This one is x86_64 and runs under Rosetta, where that
# rule does not apply, but a signed module costs nothing and stops the question
# ever arising if a native game or a hardened runtime turns up.
"$DOTNET" run --project "$ROOT/tools/MachOAdHocSign/MachOAdHocSign.csproj" -c Release --verbosity quiet \
    -- "$STAGING/KotorPatcher.dylib" KotorPatcher >/dev/null
echo "  [OK] KotorPatcher.dylib (native macOS, x86_64)"


# --- Patches ------------------------------------------------------------------
# Built once and copied into each architecture's release: a .kpatch is
# platform-independent, so the two archives ship the same set.
PATCHES_SRC="$ROOT/Patches"
PATCHES_STAGE="$STAGING/patches"

echo "[2/$STEPS] Patches..."
if [[ "$INCLUDE_PATCHES" =~ ^[Yy]$ ]]; then
    mkdir -p "$PATCHES_STAGE"

    if [ -n "$PATCHES_FROM" ]; then
        # Reuse mode: copy the prebuilt, self-contained .kpatch files verbatim.
        echo "  Reusing prebuilt patches from $PATCHES_FROM"
        for kp in "$PATCHES_FROM"/*.kpatch; do
            cp -f "$kp" "$PATCHES_STAGE/"
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
            if ( cd "$dir" && python3 "$PATCHES_SRC/create-patch.py" -o "$PATCHES_STAGE" ) >/dev/null 2>&1; then
                echo "    [OK] $name.kpatch"
            else
                echo "    [WARN] No .kpatch produced for $name"
            fi
        done
    fi

    # "additional files" are platform-independent data in the source tree, so ship
    # them from there in both modes (mirroring publish.sh). This also means reuse
    # mode does not depend on the source dir carrying the "additional files" folders.
    for dir in "$PATCHES_SRC"/*/; do
        [ -f "$dir/manifest.toml" ] || continue
        name="$(basename "$dir")"
        if [ -d "$dir/additional" ] && [ -n "$(ls -A "$dir/additional" 2>/dev/null)" ]; then
            addl_dest="$PATCHES_STAGE/$name additional files"
            mkdir -p "$addl_dest"
            cp -R "$dir/additional/." "$addl_dest/"
            echo "    [OK] Copied additional files for $name"
        fi
    done
else
    echo "  Skipping patches"
fi

# --- Per-architecture releases -------------------------------------------------
step=3
for ARCH in $ARCHS; do
    RID="osx-$ARCH"
    RELEASE_NAME="KotorPatchManager-macos-$ARCH-v$VERSION"
    RELEASE_DIR="$ROOT/releases/$RELEASE_NAME"
    # The manager looks beside itself for the modules it stages into the game
    # folder, so everything shipping with it lives here, as in the Linux release.
    BIN="$RELEASE_DIR/bin"

    echo "[$step/$STEPS] Publishing KPatchLauncher ($RID)..."
    step=$((step + 1))

    rm -rf "$RELEASE_DIR"
    mkdir -p "$BIN" "$RELEASE_DIR/tools"

    # The launcher csproj sets AppendRuntimeIdentifierToOutputPath=false, so every
    # RID shares one obj/ directory -- including the customized apphost cached in
    # it. Publishing a second RID without clearing that reuses the first one's
    # apphost and silently ships, say, an ELF binary in the macOS release. Nuke the
    # intermediates so each architecture starts from the runtime pack's own apphost.
    rm -rf "$ROOT/src/KPatchLauncher/obj" "$ROOT/src/KPatchCore/obj"

    # Self-contained folder build (not single-file): the Avalonia GUI pulls in native
    # .dylib libraries, and a folder loads them in place instead of self-extracting to
    # a temp dir on first launch -- faster cold start, and it survives a noexec /tmp.
    "$DOTNET" publish src/KPatchLauncher/KPatchLauncher.csproj \
        -c Release -r "$RID" --self-contained \
        -p:PublishSingleFile=false \
        -o "$BIN" >/dev/null
    # Publish leaves debug symbols behind; drop them from the release.
    rm -f "$BIN"/*.pdb

    # The .NET SDK ad hoc signs the macOS apphost only when it is itself running on
    # macOS, so a cross-publish produces an unsigned one. On Apple Silicon that is
    # fatal rather than untidy: the kernel refuses to execute an unsigned arm64
    # image at all. Sign it here, with the same library KPatchCore re-signs game
    # binaries with. Nothing after this may modify the file.
    "$DOTNET" run --project "$ROOT/tools/MachOAdHocSign/MachOAdHocSign.csproj" -c Release --verbosity quiet \
        -- "$BIN/KPatchLauncher" KPatchLauncher

    # The csproj links AddressDatabases into the build output, but make it explicit
    # so a publish quirk can never ship the manager without its address databases.
    echo "  Copying AddressDatabases..."
    mkdir -p "$BIN/AddressDatabases"
    cp -f "$ROOT"/AddressDatabases/*.db "$BIN/AddressDatabases/"

    # The game-side modules, beside the launcher where it looks for them.
    cp -f "$STAGING/KotorPatcher.dll" "$STAGING/binkw32.dll" "$STAGING/sqlite3.dll" \
          "$STAGING/KotorPatcher.so" "$STAGING/KotorPatcher.dylib" "$BIN/"

    echo "  [OK] Manager published"

    # --- Patches, tools, LICENSE, README --------------------------------------
    if [ -d "$PATCHES_STAGE" ]; then
        cp -R "$PATCHES_STAGE" "$RELEASE_DIR/patches"
    fi

    cp "$PATCHES_SRC/create-patch.py" "$RELEASE_DIR/tools/"
    cp "$PATCHES_SRC/create-patch.bat" "$RELEASE_DIR/tools/"
    cp "$ROOT/LICENSE" "$RELEASE_DIR/LICENSE.txt"

    cat > "$RELEASE_DIR/README.txt" <<EOF
KotOR Patch Manager v$VERSION (macOS, $ARCH)

Contents:
  bin/KPatchLauncher     - Main application (native $RID)
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

    tar -czf "releases/$RELEASE_NAME.tar.gz" -C releases "$RELEASE_NAME"
    echo "  [OK] releases/$RELEASE_NAME.tar.gz"
done

rm -rf "$STAGING"

echo
echo "SUCCESS! macOS release(s) built for: $ARCHS"
echo
