#!/usr/bin/env python3
"""Package a KOTOR patch directory into a .kpatch file.

A .kpatch is just a zip holding manifest.toml, the *hooks.toml file(s), and (for
DETOUR patches) binaries/windows_x86.dll.

This does not compile DETOUR DLLs; that needs the Windows MSVC toolchain. SIMPLE
patches have no C++ and package with no compiler at all. DETOUR patches package
only when a prebuilt windows_x86.dll is already present, which lets you repack a
Windows-built binary from here.

Usage: run from inside a patch directory, e.g. `python3 ../create-patch.py`.
"""

import argparse
import sys
import zipfile
from pathlib import Path

BANNER = "=" * 51


def fail(*lines: str) -> None:
    """Print an error block and exit non-zero."""
    for line in lines:
        print(line)
    sys.exit(1)


def find_hooks(patch_dir: Path) -> list[Path]:
    return sorted(patch_dir.glob("*hooks.toml"))


def find_prebuilt_dll(patch_dir: Path) -> Path | None:
    # The DLL may sit loose in the patch dir or already under binaries/.
    for candidate in (patch_dir / "windows_x86.dll",
                      patch_dir / "binaries" / "windows_x86.dll"):
        if candidate.is_file():
            return candidate
    return None


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
    cpp_files = list(patch_dir.glob("*.cpp"))
    is_detour = bool(cpp_files)
    if is_detour:
        print(f"  Patch type: DETOUR ({len(cpp_files)} C++ file(s) detected)")
        print("  Needs a prebuilt windows_x86.dll")
    else:
        print("  Patch type: SIMPLE (no C++ files detected)")
        print("  Skipping DLL compilation")

    # Step 3: reuse a prebuilt DLL for DETOUR patches. Nothing is compiled here.
    print()
    dll = None
    if is_detour:
        print("[3/5] Locating prebuilt DLL...")
        dll = find_prebuilt_dll(patch_dir)
        if dll is None:
            fail("ERROR: windows_x86.dll not found!",
                 "Compiling patch DLLs is not handled by this script.",
                 "Build the DLL with the Windows MSVC toolchain, drop",
                 "windows_x86.dll here, then re-run this to package it.")
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
