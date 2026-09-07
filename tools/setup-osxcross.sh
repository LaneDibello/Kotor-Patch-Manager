#!/usr/bin/env bash
# =============================================================================
# setup-osxcross.sh - install the macOS cross toolchain on a Linux/WSL host
# =============================================================================
#
# publish-macos.sh needs one thing a Linux box does not have: a compiler that
# emits Mach-O. KotorPatcher.dylib is C++ built against the macOS SDK and linked
# by ld64, and neither ships with GCC or a distro clang. osxcross supplies both,
# wrapping a host clang with the SDK and the linker.
#
# The SDK is the part Apple does not distribute standalone, so it is taken from
# the naev project's build infrastructure, pinned to a release tag. This is the
# same MacOSX14.sdk the patcher's dylib target was developed against; a different
# SDK is not automatically wrong, but nothing here has been built with one.
#
# Everything else in the macOS release -- the manager itself -- is .NET, which
# cross-publishes to macOS from any host with no extra toolchain at all.
#
# The build runs with a deliberately minimal PATH and no development environment
# variables. osxcross compiles cctools/ld64 from source and finds libxml2 through
# whichever xml2-config comes first, so a conda, homebrew-on-linux or pyenv prefix
# on PATH gets linked in: the resulting ld records a dependency on that prefix's
# ICU, which the loader cannot find afterwards, and every compile fails with
# "libicui18n.so.NN: cannot open shared object file". Building against the distro's
# libraries only is the fix, and dropping those prefixes is how.
#
# Usage:
#   ./tools/setup-osxcross.sh                 install to ~/osxcross
#   ./tools/setup-osxcross.sh --prefix <dir>  install elsewhere
#
# Needs sudo once, for the build dependencies. Takes 10-20 minutes, mostly
# compiling osxcross's own copy of the linker.
set -euo pipefail

PREFIX="${HOME}/osxcross"
SDK_URL="https://codeberg.org/naev/naev-infrastructure/raw/tag/v1.16.1/naev-macos/MacOSX14.sdk.tar.xz"
SDK_NAME="MacOSX14.sdk.tar.xz"

while [ "$#" -gt 0 ]; do
    case "$1" in
        --prefix)
            [ "$#" -ge 2 ] || { echo "  [ERROR] --prefix needs a directory"; exit 1; }
            PREFIX="$2"; shift 2 ;;
        --prefix=*) PREFIX="${1#*=}"; shift ;;
        -h|--help)
            echo "Usage: $0 [--prefix <dir>]"
            echo
            echo "  Clones and builds osxcross with the pinned macOS SDK, so that"
            echo "  publish-macos.sh can build KotorPatcher.dylib."
            exit 0 ;;
        *) echo "  [ERROR] Unknown argument: $1 (see --help)"; exit 1 ;;
    esac
done

# Running the whole script as root puts the toolchain in root's home, where the
# release build -- which runs as an ordinary user -- cannot read it, and leaves
# every file it touches root-owned. Only the dependency install needs privileges,
# and it asks for them itself.
if [ "$(id -u)" -eq 0 ] && [ -z "${OSXCROSS_ALLOW_ROOT:-}" ]; then
    echo "  [ERROR] Do not run this with sudo."
    echo
    echo "          The toolchain would land in $PREFIX, owned by root and out of"
    echo "          reach of publish-macos.sh. Run it as yourself; it calls sudo on"
    echo "          its own for the one step that needs it."
    echo
    echo "          To install system-wide on purpose, say so explicitly:"
    echo "            sudo OSXCROSS_ALLOW_ROOT=1 $0 --prefix /usr/local/lib/osxcross-build"
    exit 1
fi

echo
echo "==================================================="
echo "   osxcross setup (macOS cross toolchain)"
echo "==================================================="
echo "  Prefix: $PREFIX"
echo

# --- Dependencies -------------------------------------------------------------
# osxcross builds cctools/ld64 from source, which is what needs llvm, libxml2 and
# the compression libraries; clang is the compiler it wraps rather than replaces.
echo "[1/4] Installing build dependencies (sudo)..."
sudo apt-get update
sudo apt-get install -y \
    clang llvm-dev libxml2-dev uuid-dev libssl-dev libbz2-dev zlib1g-dev \
    cmake make patch git curl xz-utils bzip2 cpio python3

# --- osxcross -----------------------------------------------------------------
echo "[2/4] Fetching osxcross..."
if [ -d "$PREFIX/.git" ]; then
    echo "  Already cloned, reusing $PREFIX"
else
    git clone --depth 1 https://github.com/tpoechtrager/osxcross.git "$PREFIX"
fi

# --- SDK ----------------------------------------------------------------------
# osxcross picks up whatever SDK tarballs sit in tarballs/ and builds a wrapper
# per SDK it finds, so dropping this one in is the whole configuration step.
echo "[3/4] Fetching the macOS SDK..."
mkdir -p "$PREFIX/tarballs"
if [ -f "$PREFIX/tarballs/$SDK_NAME" ]; then
    echo "  Already downloaded: tarballs/$SDK_NAME"
else
    curl -fL --progress-bar -o "$PREFIX/tarballs/$SDK_NAME.part" "$SDK_URL"
    mv "$PREFIX/tarballs/$SDK_NAME.part" "$PREFIX/tarballs/$SDK_NAME"
fi

# --- Build --------------------------------------------------------------------
# Only the distribution's own tools, and none of the variables that point a build
# at somewhere else. See the note at the top: anything else on PATH that ships its
# own libxml2 leaves an ld that cannot start.
SYSTEM_PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"

# UNATTENDED skips the licence prompt, which otherwise waits forever in a script.
echo "[4/4] Building the toolchain (this takes a while)..."
( cd "$PREFIX" && env -u LD_LIBRARY_PATH -u LIBRARY_PATH -u CPATH -u C_INCLUDE_PATH \
      -u CPLUS_INCLUDE_PATH -u PKG_CONFIG_PATH -u PKG_CONFIG_LIBDIR \
      -u CMAKE_PREFIX_PATH -u CONDA_PREFIX -u CFLAGS -u CXXFLAGS -u LDFLAGS \
      PATH="$SYSTEM_PATH" UNATTENDED=1 ./build.sh )

CXX_MAC="$PREFIX/target/bin/o64-clang++"
[ -x "$CXX_MAC" ] || {
    echo "  [ERROR] Build finished but $CXX_MAC is missing."
    exit 1
}

# --- Verify -------------------------------------------------------------------
# osxcross's own tests report a broken linker as "failed (ignored)" and carry on,
# so a build can finish looking successful and produce a toolchain that cannot
# link anything. Check it here instead, where a failure can say what to do.
echo
echo "Verifying the toolchain..."

# The linker is the piece that picks up a foreign libxml2, and an unresolved
# dependency shows up plainly rather than only when something tries to link.
MISSING="$(ldd "$PREFIX/target/bin/x86_64-apple-darwin23-ld" 2>/dev/null \
    | grep 'not found' | awk '{print $1}' | sort -u | tr '\n' ' ')"
if [ -n "$MISSING" ]; then
    echo "  [ERROR] The linker cannot start: missing $MISSING"
    echo
    echo "          It was built against libraries from a prefix on your PATH"
    echo "          (conda, homebrew, pyenv) rather than the distribution's, and"
    echo "          those are not on the loader's search path. Check which one:"
    echo "            command -v xml2-config     # should be /usr/bin/xml2-config"
    echo
    echo "          Rebuild from scratch, which this script does with a clean PATH:"
    echo "            rm -rf \"$PREFIX/build\" \"$PREFIX/target\""
    echo "            $0 --prefix \"$PREFIX\""
    echo "          (the SDK is already downloaded and is kept)"
    exit 1
fi

# Then the whole chain, on something shaped like what it will really build: a
# C++ dylib for the deployment target the patcher asks for.
PROBE="$(mktemp -d)"
trap 'rm -rf "$PROBE"' EXIT
printf '#include <string>\nstd::string f(){ return "ok"; }\n' > "$PROBE/probe.cpp"
# With the toolchain's bin on PATH, as it has to be for anything to link: clang
# spawns the target's ld by name and silently falls back to the host's otherwise.
if PATH="$PREFIX/target/bin:$PATH" \
        "$CXX_MAC" -std=c++17 -mmacosx-version-min=10.9 -arch x86_64 -dynamiclib \
        -o "$PROBE/probe.dylib" "$PROBE/probe.cpp" 2>"$PROBE/err"; then
    echo "  [OK] compiled and linked a test dylib"
else
    echo "  [ERROR] The toolchain cannot build a dylib:"
    sed 's/^/          /' "$PROBE/err"
    echo
    echo "          A complaint about linker flags from /usr/bin/ld means the host"
    echo "          linker was used: $PREFIX/target/bin has to be reachable."
    exit 1
fi

echo
echo "SUCCESS! macOS cross compiler at:"
echo "  $CXX_MAC"
echo
echo "publish-macos.sh finds it here on its own. To use it by hand, put it on PATH:"
echo "  export PATH=\"$PREFIX/target/bin:\$PATH\""
echo
