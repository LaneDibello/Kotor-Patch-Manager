#!/usr/bin/env python3
"""Package a KOTOR patch directory into a .kpatch file.

A .kpatch is just a zip holding manifest.toml, the *hooks.toml file(s), and (for
DETOUR patches) a binaries/ directory. An archive is platform-independent and may
carry a module for each platform, since the manager installs to a game of any
platform from a host of any platform.

SIMPLE patches have no C++ and package with no compiler. DETOUR patches need
windows_x86.dll: on Linux this cross-compiles it with MinGW-w64
(i686-w64-mingw32-g++), mirroring the MSVC build in create-patch.bat. Without that
toolchain it falls back to a prebuilt windows_x86.dll, which lets you repack an
MSVC-built binary from here.

Modules for the native games are packaged from binaries/ as they are found, under
whatever names they carry. Nothing here cross-compiles them, so a patch supporting a
native game builds them elsewhere and drops them in that directory. The names the
installer looks for are composed from the game's platform and CPU, so a new
architecture needs no change here.

Usage: run from inside a patch directory, e.g. `python3 ../create-patch.py`.
"""

# Annotations become strings rather than being evaluated at import, so the
# `Path | None` spellings below do not need Python 3.10. macOS ships 3.9 as its
# system python3, and this tool is meant to run on whatever the machine already has.
from __future__ import annotations

import argparse
import concurrent.futures
import os
import re
import shutil
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path

BANNER = "=" * 51

# The DETOUR DLL is 32-bit to match the game and statically linked so it needs no
# MinGW runtime DLLs beside it. These mirror the cl flags in create-patch.bat;
# sqlite is imported the same way (against the shipped sqlite3.dll).
CXX = os.environ.get("CXX", "i686-w64-mingw32-g++")

COMPILE_FLAGS = [
    "-std=c++17", "-shared", "-O2", "-s",
    "-static", "-static-libgcc", "-static-libstdc++",
    "-DWIN32", "-DNDEBUG", "-D_WINDOWS", "-D_USRDLL",
]

# The same flags for a compile-only step. -shared and -s only mean something to the
# linker, and -s would strip the objects the archive is built from.
OBJECT_FLAGS = [flag for flag in COMPILE_FLAGS if flag not in ("-shared", "-s")]


def fail(*lines: str) -> None:
    """Print an error block and exit non-zero."""
    for line in lines:
        print(line)
    sys.exit(1)


def find_hooks(patch_dir: Path) -> list[Path]:
    return sorted(patch_dir.glob("*hooks.toml"))


def find_sources(patch_dir: Path) -> list[Path]:
    """Every .cpp belonging to the patch: the root plus immediate subdirectories.

    Patches may group their sources into a folder (e.g. ScriptExtender/Extensions),
    so a root-only glob would silently drop them from the build. Matches the
    subdirectory sweep create-patch.bat does."""
    return sorted([*patch_dir.glob("*.cpp"), *patch_dir.glob("*/*.cpp")])


def find_prebuilt_binary(patch_dir: Path, name: str) -> Path | None:
    # A module may sit loose in the patch dir or already under binaries/.
    for candidate in (patch_dir / name, patch_dir / "binaries" / name):
        if candidate.is_file():
            return candidate
    return None


def find_compiler() -> str | None:
    """Return the MinGW C++ cross-compiler path, or None if it is not installed."""
    return shutil.which(CXX)


def tool_for(compiler: str, tool: str) -> str:
    """Find the binutils program that matches the C++ cross-compiler.

    Prefers `compiler -print-prog-name=<tool>`, which the compiler answers with its
    own tool regardless of cross prefix, version suffix, or install path. Falls back
    to swapping the compiler's name suffix. Exits (via fail) if neither resolves,
    rather than silently returning a bare name that will not exist.
    """
    probe = subprocess.run([compiler, f"-print-prog-name={tool}"],
                           capture_output=True, text=True)
    printed = probe.stdout.strip()
    # gcc/clang print an absolute path when they resolve the tool, else the bare name.
    if printed and printed != tool and Path(printed).exists():
        return printed
    guess = re.sub(r"(g\+\+|gcc|c\+\+|clang\+\+)$", tool, compiler)
    found = shutil.which(guess)
    if found:
        return found
    fail(f"ERROR: could not find {tool} for {compiler}.",
         "Install the matching binutils (e.g. mingw32-binutils on Fedora).")


def generate_exports_def(patch_dir: Path, cpp_files: list[Path], name: str) -> Path:
    """Write exports.def listing the `extern "C" ... __cdecl` hooks, using the same
    name extraction create-patch.bat does. A committed exports.def is left alone."""
    out = patch_dir / "exports.def"
    if out.is_file():
        return out
    names = []
    for cpp in cpp_files:
        for line in cpp.read_text(errors="ignore").splitlines():
            if re.search(r"extern.*__cdecl", line):
                # Tokens split on spaces and '('; the exported name is the fifth.
                parts = [p for p in re.split(r"[ (]+", line.strip()) if p]
                if len(parts) >= 5:
                    names.append(parts[4])
    out.write_text("\n".join(["LIBRARY " + name, "EXPORTS",
                              *("    " + n for n in names)]) + "\n")
    return out


def pragma_libraries(cpp_files: list[Path]) -> list[str]:
    """Link flags for the libraries the sources request with #pragma comment(lib).

    MSVC acts on that pragma and GCC ignores it, so a patch that names opengl32
    there compiles under both and links only under MSVC. Reading the pragma keeps
    the dependency declared in one place, next to the code that needs it.
    """
    names = []
    for source in cpp_files:
        for match in re.finditer(r'#pragma\s+comment\s*\(\s*lib\s*,\s*"([^"]+)"',
                                 source.read_text(errors="replace")):
            name = re.sub(r"\.lib$", "", match.group(1), flags=re.IGNORECASE)
            if name not in names:
                names.append(name)
    return [f"-l{name}" for name in names]


def toolchain_tag(compiler: str) -> str:
    """Identify the compiler closely enough to key a cache of its object files."""
    parts = []
    for flag in ("-dumpmachine", "-dumpversion"):
        probe = subprocess.run([compiler, flag], capture_output=True, text=True)
        parts.append(probe.stdout.strip() or "unknown")
    return "-".join(parts)


def common_cache_key(common_dir: Path) -> str:
    """The compile flags, then every file under Common with its size and mtime.

    Editing, adding or deleting any source or header changes a line, which is what
    makes the cache notice. Timestamps rather than content hashes because the tree
    is large and a stat is enough to catch an edit.
    """
    lines = [" ".join(OBJECT_FLAGS)]
    for path in sorted(common_dir.rglob("*")):
        if path.is_file():
            info = path.stat()
            lines.append(f"{path.relative_to(common_dir)} {info.st_size} {info.st_mtime_ns}")
    return "\n".join(lines) + "\n"


def ensure_common_library(patch_dir: Path, common_dir: Path, lib_dir: Path,
                          compiler: str) -> Path:
    """Build Common/GameAPI into a static library once and reuse it across patches.

    Every DETOUR patch links the same translation units, and recompiling them per
    patch dominated the build. Archiving them keeps the "only pay for what you use"
    property: the linker pulls a member in only to resolve an undefined symbol, so
    the DLL is unchanged and this is purely a compile-time saving.

    The cache lives in Patches/build rather than under Common, whose tree the key
    walks -- a cache stored there would contain its own outputs and never hit.
    """
    cache_dir = patch_dir.parent / "build" / "common" / toolchain_tag(compiler)
    library = cache_dir / "libcommon.a"
    stamp = cache_dir / "sources.stamp"
    key = common_cache_key(common_dir)

    try:
        if library.is_file() and stamp.read_text() == key:
            return library
    except OSError:
        pass  # An unreadable stamp costs a rebuild, never correctness.

    sources = [*sorted(common_dir.glob("*.cpp")),
               *sorted((common_dir / "GameAPI").glob("*.cpp"))]
    print(f"  Building the shared Common/GameAPI library ({len(sources)} sources)...")
    cache_dir.mkdir(parents=True, exist_ok=True)

    # Staged inside the cache directory so the rename below stays on one filesystem.
    with tempfile.TemporaryDirectory(dir=cache_dir) as tmp:
        def compile_one(source: Path):
            # Common/Foo.cpp and Common/GameAPI/Foo.cpp share a stem, so the object
            # name carries the directory too.
            obj = Path(tmp) / f"{source.parent.name}-{source.stem}.o"
            return obj, subprocess.run(
                [compiler, *OBJECT_FLAGS, "-I", str(common_dir), "-I", str(lib_dir),
                 "-c", str(source), "-o", str(obj)],
                capture_output=True, text=True)

        objects = []
        with concurrent.futures.ThreadPoolExecutor() as pool:
            for obj, result in pool.map(compile_one, sources):
                if result.returncode != 0:
                    fail("ERROR: Common library compilation failed!", result.stderr)
                objects.append(obj)

        archive = Path(tmp) / library.name
        result = subprocess.run(
            [tool_for(compiler, "ar"), "rcs", str(archive), *(str(o) for o in objects)],
            capture_output=True, text=True)
        if result.returncode != 0:
            fail("ERROR: Common library archiving failed!", result.stderr)
        # Atomic, so two patches building in parallel cannot read a half-written
        # archive: each writes its own copy and the last rename wins.
        os.replace(archive, library)

    stamp.write_text(key)
    return library


def compile_dll(patch_dir: Path, name: str, compiler: str) -> Path:
    """Cross-compile windows_x86.dll for a DETOUR patch with MinGW-w64.

    Builds the patch sources plus the shared Common/GameAPI library and links the
    sqlite3 C API against the shipped sqlite3.dll. Exits (via fail) on any error.
    """
    common_dir = patch_dir.parent / "Common"
    lib_dir = patch_dir.parent.parent / "lib"
    if not common_dir.is_dir() or not lib_dir.is_dir():
        fail("ERROR: cannot find ../Common and ../../lib next to the patch.",
             "The MinGW build expects the standard Patches/ layout.")

    cpp_files = find_sources(patch_dir)
    common_lib = ensure_common_library(patch_dir, common_dir, lib_dir, compiler)
    exports = generate_exports_def(patch_dir, cpp_files, name)
    out = patch_dir / "windows_x86.dll"

    with tempfile.TemporaryDirectory() as tmp:
        # the vendored sqlite ships only an MSVC import lib; synthesize a MinGW
        # one from its .def so the C API links against the same sqlite3.dll
        # used at runtime.
        import_lib = Path(tmp) / "libsqlite3.a"
        result = subprocess.run(
            [tool_for(compiler, "dlltool"), "-d", str(lib_dir / "sqlite3.def"),
             "-D", "sqlite3.dll", "-l", str(import_lib)],
            capture_output=True, text=True)
        if result.returncode != 0:
            fail("ERROR: failed to build the sqlite3 import lib.", result.stderr)

        result = subprocess.run(
            [compiler, *COMPILE_FLAGS,
             "-I", str(common_dir), "-I", str(lib_dir),
             *(str(s) for s in cpp_files), str(exports),
             "-o", str(out),
             # After the patch objects: the linker resolves left to right, and only
             # the archive members those objects need are pulled in.
             str(common_lib),
             "-L", tmp, "-lsqlite3", "-lkernel32", *pragma_libraries(cpp_files)],
            capture_output=True, text=True)
        if result.returncode != 0:
            fail("ERROR: DLL compilation failed!", result.stderr)

    return out


def build(patch_dir: Path, name: str, out_dir: Path | None = None) -> None:
    # Step 1: validate required files
    print("[1/5] Validating patch files...")
    manifest = patch_dir / "manifest.toml"
    if not manifest.is_file():
        fail("ERROR: manifest.toml not found!",
             "Please create a manifest.toml file in this directory.")
    print("  [OK] manifest.toml found")

    hooks = find_hooks(patch_dir)
    if not hooks:
        fail("ERROR: No hooks files found (*hooks.toml)!",
             "Please create at least one hooks file in this directory.")
    print("  [OK] hooks file(s) found")

    # Step 2: detect patch type
    print()
    print("[2/5] Detecting patch type...")
    cpp_files = find_sources(patch_dir)
    is_detour = bool(cpp_files)
    if is_detour:
        print(f"  Patch type: DETOUR ({len(cpp_files)} C++ file(s) detected)")
    else:
        print("  Patch type: SIMPLE (no C++ files detected)")
        print("  Skipping DLL compilation")

    # Step 3: get the DETOUR DLL. Cross-compile with MinGW when it is available,
    # otherwise fall back to a prebuilt (MSVC-built) binary.
    print()
    dll = None
    if is_detour:
        compiler = find_compiler()
        if compiler:
            print("[3/5] Compiling patch DLL (MinGW)...")
            dll = compile_dll(patch_dir, name, compiler)
            print(f"  [OK] Compiled: {dll.name}")
        else:
            print("[3/5] Locating prebuilt DLL...")
            dll = find_prebuilt_binary(patch_dir, "windows_x86.dll")
            if dll is None:
                fail("ERROR: windows_x86.dll not found and no MinGW toolchain!",
                     f"Install {CXX} to cross-compile, or build the DLL with the",
                     "Windows MSVC toolchain and drop windows_x86.dll here.")
            print(f"  [OK] Using prebuilt DLL: {dll.name}")
    else:
        print("[3/5] Skipping DLL compilation (SIMPLE patch)")

    # Step 4: package the .kpatch
    print()
    print("[4/5] Creating .kpatch package...")
    if out_dir is not None:
        out_dir.mkdir(parents=True, exist_ok=True)
    out = (out_dir or patch_dir) / f"{name}.kpatch"
    out.unlink(missing_ok=True)

    print("  Copying files...")
    # Forward-slash arcnames are what the loader (PatchRepository) expects.
    with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as archive:
        archive.write(manifest, "manifest.toml")
        print("  [OK] manifest.toml")
        for hook in hooks:
            archive.write(hook, hook.name)
            print(f"  [OK] {hook.name}")
        if dll is not None:
            archive.write(dll, "binaries/windows_x86.dll")
            print("  [OK] binaries/windows_x86.dll")
        # Whatever else the author has built goes in as-is. Listing the names here would
        # mean editing this every time a platform or architecture is added.
        for extra in sorted((patch_dir / "binaries").glob("*")):
            if not extra.is_file():
                continue
            # Skip the Windows DLL only when it went in above, which it does not for a
            # SIMPLE patch that still ships one somebody built elsewhere.
            if dll is not None and extra.name == "windows_x86.dll":
                continue
            archive.write(extra, f"binaries/{extra.name}")
            print(f"  [OK] binaries/{extra.name}")
    print("  Creating archive...")

    # Step 5: verify
    print()
    print("[5/5] Verifying package...")
    if not out.is_file():
        fail("  ERROR: Package verification failed")
    # Show just the name unless it landed in a different directory.
    created = out.name if out_dir is None else out
    print(f"  [OK] Package created: {created}")
    print()
    print("  Package contents:")
    with zipfile.ZipFile(out) as archive:
        for entry in archive.namelist():
            print(f"      {entry}")
    print()
    print(BANNER)
    print("  SUCCESS! Patch created successfully.")
    print(BANNER)
    print()


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Package a KOTOR patch into a .kpatch file.")
    parser.add_argument(
        "name", nargs="?",
        help="patch name (default: the patch directory name)")
    parser.add_argument(
        "-o", "--out-dir", type=Path, default=None,
        help="write the .kpatch here instead of the patch directory "
             "(created if needed); handy for collecting patches in one folder")
    args = parser.parse_args()

    print()
    print(BANNER)
    print("  KotOR Patch Manager - Patch Creation Tool")
    print(BANNER)
    print()

    patch_dir = Path.cwd()
    if args.name:
        name = args.name
        print(f"Using provided patch name: {name}")
    else:
        name = patch_dir.name
        print(f"Using current directory name: {name}")
    print()

    build(patch_dir, name, args.out_dir)


if __name__ == "__main__":
    main()
