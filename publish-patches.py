#!/usr/bin/env python3
"""Build every patch and collect the .kpatch files, on any host.

The cross-platform counterpart to publish-patches.ps1, which drives
create-patch.bat on Windows. This drives create-patch.py, so a run covers
whichever platforms the host has toolchains for.

--target narrows a run to one module: the patches that need it, that module
alone, and a failure if one does not appear. That is a job per target, on the
host that can produce it. Without it every patch is built for everything the
host can reach, which is what a release wants.

Exit status is 0 when every patch selected was packaged, 1 otherwise.
"""

from __future__ import annotations

import argparse
import concurrent.futures
import importlib.util
import os
import shutil
import subprocess
import sys
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent
PATCHES = ROOT / "Patches"
CREATE_PATCH = PATCHES / "create-patch.py"


def create_patch():
    """create-patch.py as a module, so the rule that says which modules a patch
    needs has one implementation rather than two."""
    spec = importlib.util.spec_from_file_location("create_patch", CREATE_PATCH)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def missing_modules(patch_dir: Path, out_dir: Path, builder) -> list:
    """Modules a packaged patch needs and does not carry.

    Only a missing toolchain can cause this: one that runs and fails takes the
    whole patch down with it."""
    if not compiles_sources(patch_dir):
        return []
    archive = out_dir / f"{patch_dir.name}.kpatch"
    if not archive.is_file():
        return []
    with zipfile.ZipFile(archive) as package:
        carried = {name.split("/")[1] for name in package.namelist()
                   if name.startswith("binaries/")}
    return [builder.TARGETS[t].module
            for t in builder.required_targets(patch_dir)
            if builder.TARGETS[t].module not in carried]


def compiles_sources(patch_dir: Path) -> bool:
    return bool(next(patch_dir.glob("*.cpp"), None)
                or next(patch_dir.glob("*/*.cpp"), None))


def build_order(patches: list) -> list:
    """Patches with one that compiles sources first.

    They share the cached Common/GameAPI library, so building one before the rest
    fan out means it is written once rather than by several jobs at a time.
    """
    first = next((d for d in patches if compiles_sources(d)), None)
    return patches if first is None else [first, *(d for d in patches if d != first)]


def build(patch_dir: Path, out_dir: Path, target: str) -> tuple:
    command = [sys.executable, str(CREATE_PATCH), "-o", str(out_dir)]
    if target:
        command += ["--targets", target]
    result = subprocess.run(command, cwd=patch_dir,
                            capture_output=True, text=True)
    return patch_dir, result


def report(patch_dir: Path, result: subprocess.CompletedProcess) -> bool:
    if result.returncode == 0:
        print(f"    [OK]   {patch_dir.name}")
        return True
    print(f"    [FAIL] {patch_dir.name}")
    lines = [l for l in (result.stdout + result.stderr).splitlines() if l.strip()]
    for line in lines[-12:]:
        print(f"           {line}")
    return False


def copy_additional(patch_dir: Path, out_dir: Path) -> None:
    """Data a patch ships beside its archive, which no build produces."""
    source = patch_dir / "additional"
    if not source.is_dir() or not any(source.iterdir()):
        return
    shutil.copytree(source, out_dir / f"{patch_dir.name} additional files",
                    dirs_exist_ok=True)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("-o", "--out-dir", type=Path, default=ROOT / "build" / "patches",
                        help="where the .kpatch files and additional files land")
    parser.add_argument("-j", "--jobs", type=int, default=0,
                        help="parallel builds (default: the processor count)")
    parser.add_argument("--target", default="",
                        help="build one target only: the patches that need it, "
                             "that module alone, and fail if any is missing. "
                             "Without it every patch is built for whatever the "
                             "host can reach, which is what a release wants.")
    parser.add_argument("--strict", action="store_true",
                        help="fail when a patch is packaged without a module it "
                             "needs. Off by default, because a developer host is "
                             "not expected to have every toolchain; a release is.")
    args = parser.parse_args()
    # Resolved against the current directory once, here. create-patch.py runs with
    # cwd set to the patch's own directory, so a relative -o would land inside
    # every patch instead of in one shared place -- silently, since the summary
    # then looks for the archives where they were asked for and finds none.
    args.out_dir = args.out_dir.resolve()

    if not CREATE_PATCH.is_file():
        sys.exit(f"ERROR: {CREATE_PATCH} not found.")
    builder = create_patch()
    patches = sorted(d for d in PATCHES.iterdir()
                     if (d / "manifest.toml").is_file())
    if not patches:
        sys.exit(f"ERROR: no patches under {PATCHES}.")
    if args.target:
        if args.target not in builder.TARGETS:
            sys.exit(f"ERROR: --target names no such target: {args.target}")
        patches = [d for d in patches if compiles_sources(d)
                   and args.target in builder.required_targets(d)]
        if not patches:
            print(f"  No patch needs a {args.target} module.")
            return 0
    patches = build_order(patches)
    args.out_dir.mkdir(parents=True, exist_ok=True)

    jobs = args.jobs if args.jobs > 0 else (os.cpu_count() or 4)
    print(f"  {len(patches)} patch(es) into {args.out_dir}, up to {jobs} job(s).")

    first, rest = patches[0], patches[1:]
    failed = [] if report(*build(first, args.out_dir, args.target)) else [first.name]
    with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
        for patch_dir, result in pool.map(
                lambda d: build(d, args.out_dir, args.target), rest):
            if not report(patch_dir, result):
                failed.append(patch_dir.name)

    for patch_dir in patches:
        copy_additional(patch_dir, args.out_dir)

    # With --target the other modules were never asked for, so only that one can
    # be missing. Without it, what the host could not reach is worth reporting.
    wanted = {builder.TARGETS[args.target].module} if args.target else set()
    missing = {d.name: m for d in patches
               for m in [missing_modules(d, args.out_dir, builder)] if m}
    if args.target:
        owed = {name: [m for m in modules if m in wanted]
                for name, modules in missing.items()}
        owed = {name: modules for name, modules in owed.items() if modules}
        incomplete = {}
    else:
        incomplete, owed = missing, {}

    print()
    print(f"  Packaged {len(patches) - len(failed)}/{len(patches)}.")
    if failed:
        print(f"  Failed: {', '.join(sorted(failed))}")
    if incomplete:
        label = "Incomplete" if not args.strict else "ERROR: incomplete"
        print(f"  {label}, this host has no toolchain for them:")
        for name in sorted(incomplete):
            print(f"    {name}: {', '.join(incomplete[name])}")
        if args.strict:
            print("  Build those modules on a host that can and drop them in the "
                  "patch's binaries/ directory.")
    if owed:
        print("  Missing modules this host was expected to produce:")
        for name in sorted(owed):
            print(f"    {name}: {', '.join(owed[name])}")
    return 1 if failed or owed or (args.strict and incomplete) else 0


if __name__ == "__main__":
    sys.exit(main())
