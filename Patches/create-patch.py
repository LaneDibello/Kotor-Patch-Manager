#!/usr/bin/env python3
"""Package a KOTOR patch directory into a .kpatch file.

A .kpatch is a zip holding manifest.toml, the *hooks.toml file(s), and (for DETOUR
patches) a binaries/ directory. An archive is platform-independent and may carry a
module for each platform, since the manager installs to a game of any platform from
a host of any platform.

SIMPLE patches have no C++ and package with no compiler. A patch with sources gets a
module for every target it needs, worked out from the games its own TOMLs say it
supports, narrowed by --targets when only some are wanted. What can actually be
built depends on the host: the Windows module comes from MSVC or MinGW, the native ones
from a compiler for that platform. A target this machine has no toolchain for is
skipped with a warning, or taken from binaries/ when one was built elsewhere.

Supporting another architecture means adding an entry to TARGETS, which names the
module, the compilers that produce it, and the flags they want.

Usage: run from inside a patch directory, e.g. `python3 ../create-patch.py`.
"""

# Annotations become strings rather than being evaluated at import, so the
# `Path | None` spellings below do not need Python 3.10. macOS ships 3.9 as its
# system python3, and this tool is meant to run on whatever the machine already has.
from __future__ import annotations

import argparse
import concurrent.futures
import contextlib
import os
import re
import shutil
import sqlite3
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path
from typing import NamedTuple, NoReturn

BANNER = "=" * 51

# Header fields that say what a compiled file actually is.
PE_MACHINE_I386 = 0x014C                # IMAGE_FILE_MACHINE_I386, winnt.h
PE_CHARACTERISTIC_DLL = 0x2000          # IMAGE_FILE_DLL, winnt.h
ELF_CLASS_32 = 1                        # ELFCLASS32, EI_CLASS
ELF_TYPE_DYN = 3                        # ET_DYN, elf.h
ELF_MACHINE_386 = 3                     # EM_386, elf.h
MACHO_MAGIC_64 = b"\xcf\xfa\xed\xfe"  # MH_MAGIC_64, little endian on disk
MACHO_TYPE_DYLIB = 6                    # MH_DYLIB, loader.h
MACHO_CPU_X86_64 = 0x01000007           # CPU_TYPE_X86_64, machine.h

VSWHERE = Path(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")) \
    / "Microsoft Visual Studio" / "Installer" / "vswhere.exe"
VS_CXX_COMPONENT = "Microsoft.VisualStudio.Component.VC.Tools.x86.x64"
VCVARS32 = Path("VC") / "Auxiliary" / "Build" / "vcvars32.bat"


class BuildFailed(Exception):
    """A compiler ran and rejected the patch. Raised rather than exiting so the
    remaining targets still get their turn and one run reports every breakage."""


def fail(*lines: str) -> NoReturn:
    """Print an error block and exit non-zero."""
    for line in lines:
        print(line)
    sys.exit(1)


class GameApi(NamedTuple):
    """What a Windows patch module links beyond its own sources."""

    library: Path
    lib_dir: Path
    exports: Path


class Toolchain(NamedTuple):
    """One compiler that can produce a target, and the flags it wants."""

    driver: object
    env: str
    compilers: tuple
    flags: tuple


class Target(NamedTuple):
    """One module a .kpatch can carry, and the toolchains that can produce it."""

    module: str
    platform: str
    toolchains: tuple
    links_game_api: bool


class UnixDriver:
    """gcc, clang and the MinGW cross compilers, which share a command line."""

    library = "libcommon.a"

    def locate(self, candidate: str) -> str | None:
        return shutil.which(candidate)

    def environment(self, compiler: str) -> dict:
        """The environment one invocation needs, scoped to it rather than exported.

        A compiler named by an absolute path can sit outside PATH, and a cross
        compiler spawns its target's ld by name, so its own directory goes on.
        osxcross needs the sibling lib directory too: its ld64 links against its
        own libxar and libtapi through a RUNPATH holding the directory it was built
        in, which a toolchain installed by copying no longer has.
        """
        bin_dir = Path(compiler).resolve().parent
        environment = dict(os.environ)
        environment["PATH"] = f"{bin_dir}{os.pathsep}{environment.get('PATH', '')}"
        lib_dir = bin_dir.parent / "lib"
        if lib_dir.is_dir():
            environment["LD_LIBRARY_PATH"] = (
                f"{lib_dir}{os.pathsep}{environment.get('LD_LIBRARY_PATH', '')}")
        return environment

    def identity(self, compiler: str) -> str:
        parts = []
        for flag in ("-dumpmachine", "-dumpversion"):
            probe = subprocess.run([compiler, flag], capture_output=True, text=True)
            parts.append(probe.stdout.strip() or "unknown")
        return "-".join(parts)

    def tool(self, compiler: str, name: str) -> str:
        """The binutils program matching a compiler, for the target it builds.

        Resolved from the compiler's own triple, which is the only answer that is
        right for a cross compiler. -print-prog-name is trustworthy for gcc alone:
        clang does not drive ar, so it reports whatever is on PATH, which for a
        cross compiler is the host's and the wrong object format. The compiler's
        own directory is searched before PATH, since a toolchain installed outside
        PATH keeps its tools beside it.
        """
        triple = subprocess.run([compiler, "-dumpmachine"],
                                capture_output=True, text=True).stdout.strip()
        candidates = [f"{triple}-{name}"] if triple else []
        if "clang" in Path(compiler).name:
            candidates.append(f"llvm-{name}")
        else:
            printed = subprocess.run([compiler, f"-print-prog-name={name}"],
                                     capture_output=True, text=True).stdout.strip()
            if printed and printed != name:
                candidates.append(printed)
        candidates.append(name)

        bin_dir = Path(compiler).resolve().parent
        for candidate in candidates:
            if not Path(candidate).is_absolute():
                sibling = bin_dir / candidate
                if sibling.is_file():
                    return str(sibling)
            found = shutil.which(candidate)
            if found:
                return found
        fail(f"ERROR: could not find {name} for {compiler}.",
             f"Tried: {', '.join(candidates)}")

    def build_library(self, compiler: str, flags: tuple, sources: list,
                      includes: list, cache_dir: Path) -> Path:
        # -shared and -s are link-time only, and -s would strip the objects.
        object_flags = [f for f in flags if f not in ("-shared", "-s")]
        include_args = [a for path in includes for a in ("-I", str(path))]
        environment = self.environment(compiler)
        objects_dir = cache_dir / "obj"
        objects_dir.mkdir(parents=True, exist_ok=True)

        def one(source: Path):
            # Named for the directory too: Common/GameAPI/Camera.cpp and a patch's
            # own Camera.cpp would otherwise collide on one object name.
            obj = objects_dir / f"{source.parent.name}-{source.stem}.o"
            return obj, subprocess.run(
                [compiler, *object_flags, *include_args, "-c", str(source),
                 "-o", str(obj)],
                capture_output=True, text=True, env=environment)

        objects = []
        with concurrent.futures.ThreadPoolExecutor() as pool:
            for obj, result in pool.map(one, sources):
                if result.returncode != 0:
                    fail("ERROR: Common library compilation failed!", result.stderr)
                objects.append(obj)

        library = cache_dir / self.library
        archive = subprocess.run(
            [self.tool(compiler, "ar"), "rcs", str(library),
             *(str(o) for o in objects)],
            capture_output=True, text=True, env=environment)
        if archive.returncode != 0:
            fail("ERROR: Common library archiving failed!", archive.stderr)
        return library

    def build_module(self, compiler: str, flags: tuple, sources: list,
                     includes: list, output: Path,
                     game_api: GameApi | None = None) -> None:
        environment = self.environment(compiler)
        include_args = [a for path in includes for a in ("-I", str(path))]
        libraries: list = []
        with contextlib.ExitStack() as stack:
            if game_api is not None:
                # The vendored sqlite ships only an MSVC import lib, so synthesize a
                # MinGW one from the same .def and bind to the same sqlite3.dll.
                tmp = stack.enter_context(tempfile.TemporaryDirectory())
                import_lib = Path(tmp) / "libsqlite3.a"
                made = subprocess.run(
                    [self.tool(compiler, "dlltool"),
                     "-d", str(game_api.lib_dir / "sqlite3.def"),
                     "-D", "sqlite3.dll", "-l", str(import_lib)],
                    capture_output=True, text=True, env=environment)
                if made.returncode != 0:
                    raise BuildFailed("failed to build the sqlite3 import lib.\n"
                                      + made.stderr)
                # Read before exports.def joins the sources: the pragma lives in
                # the patch's C++, and cl acts on it without being told.
                requested = pragma_libraries(sources)
                sources = [*sources, game_api.exports]
                # After the patch objects: the linker resolves left to right and
                # pulls in only the archive members they need.
                libraries = [str(game_api.library), "-L", tmp,
                             "-lsqlite3", "-lkernel32", *requested]

            result = subprocess.run(
                [compiler, *flags, *include_args,
                 *(str(s) for s in sources), "-o", str(output), *libraries],
                capture_output=True, text=True, env=environment)
        if result.returncode != 0:
            raise BuildFailed(result.stderr)


class MsvcDriver:
    """cl, which shares no flag syntax with the gcc family."""

    library = "Common.lib"

    _environment: dict | None = None
    _looked = False

    def locate(self, candidate: str) -> str | None:
        environment = self.environment(candidate)
        if environment is None:
            return None
        return shutil.which(candidate, path=environment.get("PATH"))

    def environment(self, compiler: str | None = None) -> dict | None:
        """The environment cl needs, or None when Visual Studio is not installed.

        A developer prompt has already set it, so an ambient cl is taken as is.
        Otherwise it is built the way that prompt does: vswhere reports where VS
        landed, vcvars32.bat sets the include and library paths, and the
        environment it leaves is read back. VCVARSALL overrides the search, as it
        does in create-patch.bat.
        """
        if MsvcDriver._looked:
            return MsvcDriver._environment
        MsvcDriver._looked = True

        if shutil.which("cl"):
            MsvcDriver._environment = dict(os.environ)
            return MsvcDriver._environment

        vcvars = os.environ.get("VCVARSALL")
        if not vcvars:
            # vswhere ships with the installer at a fixed, versionless location
            # because it is the thing that finds everything else.
            vswhere = shutil.which("vswhere") or str(VSWHERE)
            if Path(vswhere).is_file():
                found = subprocess.run(
                    [vswhere, "-latest", "-products", "*",
                     "-requires", VS_CXX_COMPONENT, "-property", "installationPath"],
                    capture_output=True, text=True)
                roots = found.stdout.strip().splitlines()
                if found.returncode == 0 and roots:
                    vcvars = str(Path(roots[0]) / VCVARS32)

        if vcvars and Path(vcvars).is_file():
            # Passed as one string, not a list: list2cmdline backslash-escapes the
            # quotes around the path, which cmd reads as a literal and rejects.
            dump = subprocess.run(f'"{vcvars}" >nul && set', shell=True,
                                  capture_output=True, text=True)
            if dump.returncode == 0:
                environment = dict(os.environ)
                for line in dump.stdout.splitlines():
                    name, separator, value = line.partition("=")
                    if separator and name:
                        # Upper-cased to match os.environ, which does the same on
                        # Windows. `set` reports the stored casing, so Path would
                        # otherwise sit beside PATH and the original would win.
                        environment[name.upper()] = value
                MsvcDriver._environment = environment
        return MsvcDriver._environment

    def identity(self, compiler: str) -> str:
        toolset = (self.environment() or {}).get("VCTOOLSVERSION", "unknown")
        return f"msvc-{toolset}-x86"

    def build_library(self, compiler: str, flags: tuple, sources: list,
                      includes: list, cache_dir: Path) -> Path:
        environment = self.environment()
        objects_dir = cache_dir / "obj"
        objects_dir.mkdir(parents=True, exist_ok=True)
        for stale in objects_dir.glob("*.obj"):
            stale.unlink()

        # /Fo names a directory because objects take the source basename, so without
        # a private one Common/GameAPI/Camera.obj and a patch's own would collide.
        result = subprocess.run(
            [compiler, "/c", "/nologo", "/MP", *flags,
             *(f"/I{path}" for path in includes),
             f"/Fo{objects_dir}{os.sep}", *(str(s) for s in sources)],
            capture_output=True, text=True, env=environment)
        if result.returncode != 0:
            fail("ERROR: Common library compilation failed!",
                 result.stdout, result.stderr)

        library = cache_dir / self.library
        # Resolved against the vcvars PATH: CreateProcess searches the parent
        # process's PATH for a bare name, not the environment handed to it.
        librarian = shutil.which("lib", path=(environment or {}).get("PATH")) or "lib"
        archive = subprocess.run(
            [librarian, "/nologo", f"/OUT:{library}",
             *(str(o) for o in sorted(objects_dir.glob("*.obj")))],
            capture_output=True, text=True, env=environment)
        if archive.returncode != 0:
            fail("ERROR: Common library archiving failed!",
                 archive.stdout, archive.stderr)
        return library

    def build_module(self, compiler: str, flags: tuple, sources: list,
                     includes: list, output: Path,
                     game_api: GameApi | None = None) -> None:
        environment = self.environment()
        build_dir = output.parent / "obj"
        build_dir.mkdir(parents=True, exist_ok=True)
        # Everything after /link goes to the linker, and /OUT: only means anything
        # there: cl ignores it as an unknown option and names the DLL after the
        # first source instead.
        link = ["/link"]
        if game_api is not None:
            link += [f"/DEF:{game_api.exports}",
                     f"/LIBPATH:{game_api.lib_dir}", "sqlite3.lib",
                     str(game_api.library),
                     # Keeps the import library and its .exp out of the patch dir.
                     f"/IMPLIB:{build_dir.parent / 'windows_x86.lib'}"]
        link.append(f"/OUT:{output}")
        result = subprocess.run(
            [compiler, "/LD", "/nologo", "/MP", *flags,
             *(f"/I{path}" for path in includes), f"/Fo{build_dir}{os.sep}",
             *(str(s) for s in sources), *link],
            capture_output=True, text=True, env=environment, cwd=output.parent)
        if result.returncode != 0:
            raise BuildFailed(result.stdout + result.stderr)


UNIX = UnixDriver()
MSVC = MsvcDriver()

# MSVC leads because create-patch.bat has always used it, so a machine with Visual
# Studio keeps producing the DLL it produced before. MinGW covers everything else.
WINDOWS_TOOLCHAINS = (
    Toolchain(
        driver=MSVC, env="CL_WIN", compilers=("cl",),
        # /MT rather than /MD: the DLL must not import vcruntime140.dll or
        # msvcp140.dll, which a user's machine or a Wine prefix may not have.
        flags=("/O2", "/MT", "/W3", "/EHsc", "/std:c++17")),
    Toolchain(
        driver=UNIX, env="CXX_WIN",
        # The cross name first; on Windows a MinGW g++ is native and produces this
        # target directly, while elsewhere the plain names are the host compiler.
        compilers=("i686-w64-mingw32-g++", "g++", "clang++"),
        flags=("-std=c++17", "-shared", "-O2", "-s", "-static",
               "-static-libgcc", "-static-libstdc++",
               "-DWIN32", "-DNDEBUG", "-D_WINDOWS", "-D_USRDLL")),
)

# Module names match DeploymentPolicy.PatchBinaryFileName on the manager side:
# {platform}_{architecture}{extension}. These three have a game to run in: the
# Windows build, Aspyr's native Linux KOTOR II, and Aspyr's x86_64 macOS builds.
#
# Only the Windows module links Common/GameAPI and sqlite. A native module loads
# into a process that has already resolved what it calls, so it links nothing.
TARGETS = {
    "windows_x86": Target(
        module="windows_x86.dll", platform="Windows",
        toolchains=WINDOWS_TOOLCHAINS, links_game_api=True),
    "linux_x86": Target(
        module="linux_x86.so", platform="Linux",
        toolchains=(Toolchain(
            driver=UNIX, env="CXX_LINUX",
            compilers=("g++", "clang++"),
            flags=("-m32", "-O2", "-fPIC", "-shared",
                   "-fno-exceptions", "-fno-rtti", "-Wall", "-Wextra")),),
        links_game_api=False),
    "macos_x86_64": Target(
        module="macos_x86_64.dylib", platform="macOS",
        toolchains=(Toolchain(
            driver=UNIX, env="CXX_MAC",
            # osxcross on a Linux host, the system compiler on a Mac. osxcross
            # installs outside PATH, so a Linux host needs it on PATH or named in
            # CXX_MAC.
            compilers=("o64-clang++", "clang++"),
            # KOTOR II asks for 10.9.5, so a patch module never raises the bar.
            flags=("-arch", "x86_64", "-O2", "-fPIC", "-dynamiclib",
                   "-mmacosx-version-min=10.9", "-fno-exceptions", "-fno-rtti",
                   "-install_name", "@executable_path/macos_x86_64.dylib")),),
        links_game_api=False),
}

# Which development package a header comes from, for the compilers that look for
# one and do not find it. OpenGL is the only library a patch needs beyond what the
# game already loads.
MISSING_HEADER = re.compile(r"fatal error: ([^:\n]+): No such file or directory")
HEADER_PACKAGES = {
    "GL/gl.h": "libgl-dev on Debian and Ubuntu, mesa-libGL-devel on Fedora",
}

SHA256 = re.compile(r"[0-9A-Fa-f]{64}")
DETOUR_HOOK = re.compile(r'^\s*type\s*=\s*"detour"', re.MULTILINE)
FIRST_HOOK = re.compile(r"^\s*\[\[hooks\]\]", re.MULTILINE)


def find_hooks(patch_dir: Path) -> list[Path]:
    return sorted(patch_dir.glob("*hooks.toml"))


def find_sources(patch_dir: Path) -> list[Path]:
    """Every .cpp belonging to the patch: the root plus immediate subdirectories.

    Patches may group sources into a folder (e.g. ScriptExtender/Extensions), so a
    root-only glob would silently drop them."""
    return sorted([*patch_dir.glob("*.cpp"), *patch_dir.glob("*/*.cpp")])


def find_prebuilt_binary(patch_dir: Path, name: str) -> Path | None:
    """A module an author built elsewhere and left in binaries/.

    Only binaries/, never the patch root: the root is where a build used to leave
    its output, so looking there would let a stale module stand in for one this
    host cannot build."""
    candidate = patch_dir / "binaries" / name
    return candidate if candidate.is_file() else None


def platform_of_version(patches_dir: Path) -> dict:
    """SHA-256 to platform, from the address databases beside the Patches tree.

    The databases are where a build's identity is recorded; the manifests only
    quote it."""
    databases = patches_dir.resolve().parent / "AddressDatabases"
    known = {}
    for database in sorted(databases.glob("*.db")):
        with contextlib.closing(
                sqlite3.connect(f"file:{database}?mode=ro", uri=True)) as con:
            for sha, platform in con.execute(
                    "SELECT sha256_hash, platform FROM game_version"):
                known[sha.upper()] = platform
    if not known:
        fail(f"ERROR: no game versions found in {databases}.",
             "Which modules a patch needs is worked out from the games it supports,",
             "so the address databases have to be reachable.")
    return known


def required_targets(patch_dir: Path) -> list:
    """The targets a patch needs modules for, worked out from its own TOMLs.

    A module is needed wherever the patch has DETOUR hooks, since those are what
    load one. A patch with sources and no hooks at all is DLL_ONLY: it installs its
    hooks from the module's entry point, so it needs one everywhere it is supported.
    Read with regexes rather than a TOML parser, because tomllib arrived in 3.11 and
    this runs on whatever python3 a machine has, which on macOS is still 3.9.
    """
    supported = set(SHA256.findall(
        (patch_dir / "manifest.toml").read_text(errors="replace")))
    wanted, detoured = set(), False
    for hook_file in find_hooks(patch_dir):
        text = hook_file.read_text(errors="replace")
        if not DETOUR_HOOK.search(text):
            continue
        detoured = True
        # target_versions sits in [metadata], above the first [[hooks]].
        head = FIRST_HOOK.split(text, 1)[0]
        wanted |= set(SHA256.findall(head)) or supported
    versions = wanted if detoured else supported

    platforms = platform_of_version(patch_dir.parent)
    needed = {platforms.get(v.upper()) for v in versions}
    targets = sorted(name for name, target in TARGETS.items()
                     if target.platform in needed)
    if not targets:
        unresolved = sorted(v for v in versions if v.upper() not in platforms)
        fail("ERROR: cannot tell which modules this patch needs.",
             "No game version it supports resolves to a platform. Unrecognised:",
             *(f"  {v}" for v in unresolved or versions))
    return targets


def select_targets(patch_dir: Path, only: list | None) -> list:
    """The targets to build: what the patch needs, narrowed by --targets.

    A filter rather than a choice. What a patch needs follows from its own hooks,
    so naming a target it has no hooks for would ask for a module nothing loads."""
    needed = required_targets(patch_dir)
    if not only:
        return needed
    unknown = [name for name in only if name not in TARGETS]
    if unknown:
        fail(f"ERROR: unknown target(s): {', '.join(unknown)}",
             f"Known targets: {', '.join(sorted(TARGETS))}")
    unwanted = [name for name in only if name not in needed]
    if unwanted:
        fail(f"ERROR: this patch needs no {', '.join(unwanted)} module.",
             f"It needs: {', '.join(needed)}")
    return [name for name in needed if name in only]


def binary_target(path: Path) -> str | None:
    """The target a compiled file is for, read from its own header, or None.

    A compiler that runs is not one that produced the target: a host g++ handed the
    Windows flags writes a perfectly good ELF and names it windows_x86.dll. The
    architecture alone is not enough either, since the games load these, so the
    file has to say it is a library rather than an executable."""
    try:
        with path.open("rb") as handle:
            magic = handle.read(4)
            if magic[:2] == b"MZ":
                handle.seek(0x3C)                       # e_lfanew
                pe = int.from_bytes(handle.read(4), "little")
                handle.seek(pe)
                if handle.read(4) != b"PE\0\0":
                    return None
                machine = int.from_bytes(handle.read(2), "little")
                handle.seek(pe + 22)                    # COFF Characteristics
                flags = int.from_bytes(handle.read(2), "little")
                return ("windows_x86"
                        if machine == PE_MACHINE_I386
                        and flags & PE_CHARACTERISTIC_DLL else None)
            if magic == b"\x7fELF":
                elf_class = handle.read(1)[0]
                handle.seek(16)                         # e_type, then e_machine
                kind = int.from_bytes(handle.read(2), "little")
                machine = int.from_bytes(handle.read(2), "little")
                return ("linux_x86"
                        if elf_class == ELF_CLASS_32 and kind == ELF_TYPE_DYN
                        and machine == ELF_MACHINE_386 else None)
            if magic == MACHO_MAGIC_64:
                cpu = int.from_bytes(handle.read(4), "little")
                handle.seek(12)                         # filetype
                kind = int.from_bytes(handle.read(4), "little")
                return ("macos_x86_64"
                        if cpu == MACHO_CPU_X86_64
                        and kind == MACHO_TYPE_DYLIB else None)
    except OSError:
        return None
    return None


def compiler_candidates(toolchain: Toolchain) -> list:
    """Compilers to try, in the order they should win: an explicit override for this
    toolchain, then its presets, then whatever the environment calls its compiler."""
    ordered = [os.environ.get(toolchain.env), *toolchain.compilers,
               os.environ.get("CXX")]
    candidates = []
    for name in ordered:
        if name and name not in candidates:
            candidates.append(name)
    return candidates


_TOOLCHAIN_FOR_TARGET: dict = {}


def find_target_toolchain(target_name: str):
    """The first toolchain on this host that really produces the target, as a
    (toolchain, compiler) pair, or None.

    Neither a compiler existing nor it compiling without error proves it emitted the
    target, so the probe builds a throwaway with the real flags and reads the header
    of what came out."""
    if target_name in _TOOLCHAIN_FOR_TARGET:
        return _TOOLCHAIN_FOR_TARGET[target_name]

    target = TARGETS[target_name]
    chosen = None
    for toolchain in target.toolchains:
        for candidate in compiler_candidates(toolchain):
            compiler = toolchain.driver.locate(candidate)
            if not compiler:
                continue
            with tempfile.TemporaryDirectory() as tmp:
                source = Path(tmp) / "probe.cpp"
                source.write_text('extern "C" void probe(void) {}\n')
                output = Path(tmp) / target.module
                try:
                    toolchain.driver.build_module(
                        compiler, toolchain.flags, [source], [], output)
                    produced = binary_target(output)
                except BuildFailed:
                    produced = None
            if produced == target_name:
                chosen = (toolchain, compiler)
                break
        if chosen:
            break
    _TOOLCHAIN_FOR_TARGET[target_name] = chosen
    return chosen


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
    """Link flags for libraries the sources request with #pragma comment(lib).

    MSVC acts on that pragma and GCC ignores it, so a patch naming opengl32 there
    compiles under both and links only under MSVC."""
    names = []
    for source in cpp_files:
        for match in re.finditer(r'#pragma\s+comment\s*\(\s*lib\s*,\s*"([^"]+)"',
                                 source.read_text(errors="replace")):
            name = re.sub(r"\.lib$", "", match.group(1), flags=re.IGNORECASE)
            if name not in names:
                names.append(name)
    return [f"-l{name}" for name in names]


def common_cache_key(common_dir: Path, flags: tuple) -> str:
    """The compile flags, then every file under Common with its size and mtime.

    Timestamps rather than content hashes: the tree is large and a stat catches an
    edit."""
    lines = [" ".join(flags)]
    for path in sorted(common_dir.rglob("*")):
        if path.is_file():
            info = path.stat()
            lines.append(
                f"{path.relative_to(common_dir)} {info.st_size} {info.st_mtime_ns}")
    return "\n".join(lines) + "\n"


def ensure_common_library(patch_dir: Path, common_dir: Path, lib_dir: Path,
                          toolchain: Toolchain, compiler: str) -> Path:
    """Build Common/GameAPI into a static library once and reuse it across patches.

    Keyed by toolchain, since an archive only links against objects from the one
    that built it. Kept in Patches/build rather than under Common, whose tree the
    cache key walks.

    Not safe to run concurrently against a cold cache: several builds would compile
    the same objects into one directory and write the archive at once. A caller
    building many patches builds one serially first, as publish-patches.ps1 does."""
    driver = toolchain.driver
    cache_dir = (patch_dir.parent / "build" / "common"
                 / driver.identity(compiler))
    library = cache_dir / driver.library
    stamp = cache_dir / "sources.stamp"
    key = common_cache_key(common_dir, toolchain.flags)

    try:
        if library.is_file() and stamp.read_text() == key:
            return library
    except OSError:
        pass  # An unreadable stamp costs a rebuild, never correctness.

    sources = [*sorted(common_dir.glob("*.cpp")),
               *sorted((common_dir / "GameAPI").glob("*.cpp"))]
    print(f"  Building the shared Common/GameAPI library ({len(sources)} sources)...")
    cache_dir.mkdir(parents=True, exist_ok=True)
    library = driver.build_library(compiler, toolchain.flags, sources,
                                   [common_dir, lib_dir], cache_dir)
    # Stamped last, so an interrupted build leaves the cache stale rather than
    # marked valid with a half-built library in it.
    stamp.write_text(key)
    return library


def compile_module(patch_dir: Path, name: str, target_name: str,
                   toolchain: Toolchain, compiler: str) -> Path:
    """Build one target's module for a patch and return where it landed."""
    target = TARGETS[target_name]
    sources = find_sources(patch_dir)

    out = patch_dir / "build" / target.module
    out.parent.mkdir(parents=True, exist_ok=True)

    if not target.links_game_api:
        toolchain.driver.build_module(compiler, toolchain.flags, sources, [], out)
        return out

    common_dir = patch_dir.parent / "Common"
    lib_dir = patch_dir.parent.parent / "lib"
    if not common_dir.is_dir() or not lib_dir.is_dir():
        fail("ERROR: cannot find ../Common and ../../lib next to the patch.",
             "The build expects the standard Patches/ layout.")

    game_api = GameApi(
        library=ensure_common_library(patch_dir, common_dir, lib_dir,
                                      toolchain, compiler),
        lib_dir=lib_dir,
        exports=generate_exports_def(patch_dir, sources, name))
    toolchain.driver.build_module(compiler, toolchain.flags, sources,
                                  [common_dir, lib_dir], out, game_api)
    return out


def build(patch_dir: Path, name: str, out_dir: Path | None = None,
          only: list | None = None) -> None:
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

    # Step 3: a module per declared target. A target with no toolchain here is
    # skipped with a warning; a toolchain that runs and fails is an error, since
    # only the second means the patch has stopped building.
    print()
    modules = {}
    broken = []
    chosen_targets = select_targets(patch_dir, only) if is_detour else []
    if is_detour:
        wanted = chosen_targets
        print(f"[3/5] Building modules for: {', '.join(wanted)}")
        for target_name in wanted:
            target = TARGETS[target_name]
            selected = find_target_toolchain(target_name)
            if selected is None:
                prebuilt = find_prebuilt_binary(patch_dir, target.module)
                if prebuilt is not None:
                    modules[target_name] = prebuilt
                    print(f"  [OK] {target.module} (prebuilt; no toolchain here)")
                else:
                    print(f"  [WARN] no toolchain for {target_name}; this package "
                          f"will not carry {target.module}")
                continue
            toolchain, compiler = selected
            try:
                modules[target_name] = compile_module(
                    patch_dir, name, target_name, toolchain, compiler)
            except BuildFailed as failure:
                broken.append(target_name)
                print(f"  [FAIL] {target.module}")
                for line in str(failure).splitlines():
                    print(f"         {line}")
                for header in MISSING_HEADER.findall(str(failure)):
                    header = header.strip()
                    package = HEADER_PACKAGES.get(header)
                    hint = (f"{header} comes from {package}." if package
                            else f"install the package providing {header}.")
                    print(f"         {hint}")
                continue
            print(f"  [OK] {target.module} ({Path(compiler).name})")
        if broken:
            fail(f"ERROR: {len(broken)} target(s) failed to build: "
                 f"{', '.join(broken)}.",
                 "No package was written.")
        if not modules:
            fail("ERROR: no module could be built for this DETOUR patch.",
                 "The manager refuses a DETOUR patch that carries no module, so",
                 "there would be nothing to install. Install a toolchain for one of",
                 f"{', '.join(wanted)}, or drop a prebuilt module in binaries/.")
    else:
        print("[3/5] Skipping module compilation (SIMPLE patch)")

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
        packaged = set()
        for target_name in sorted(modules):
            module = TARGETS[target_name].module
            archive.write(modules[target_name], f"binaries/{module}")
            packaged.add(module)
            print(f"  [OK] binaries/{module}")
        # Whatever else the author has built goes in as-is. Listing the names here would
        # mean editing this every time a platform or architecture is added.
        for extra in sorted((patch_dir / "binaries").glob("*")):
            if not extra.is_file() or extra.name in packaged:
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
    shutil.rmtree(patch_dir / "build", ignore_errors=True)

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
        "--targets", default="",
        help="comma-separated subset of the targets this patch needs, "
             "for building only some of them")
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

    only = [t.strip() for t in args.targets.split(",") if t.strip()]
    build(patch_dir, name, args.out_dir, only)


if __name__ == "__main__":
    main()
