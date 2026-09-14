#!/usr/bin/env python3
"""Check every patch's manifest.toml and hooks.toml before they reach a release.

A .kpatch is only as good as its metadata, and the ways it goes wrong are quiet
ones. The manager picks a patch by comparing the executable's SHA-256 against the
values in supported_versions, so a mistyped hash there does not announce itself.
It makes the manager tell the user their game is unsupported, which is a lie
neither of them can see through.

Hashes are checked against the address databases rather than against the other
manifests. A database is where a build's identity is actually recorded. Manifests
agreeing with each other only proves they were copied from each other.

Problems come in two grades. An error would misbehave once installed and fails
the run. A warning is drift from the documented conventions that the manager
itself tolerates, and is reported without failing.

Exit status is 0 when no patch has an error, 1 otherwise.
"""

from __future__ import annotations

import contextlib
import re
import sqlite3
import sys
from pathlib import Path

try:
    import tomllib
except ModuleNotFoundError:  # tomllib landed in 3.11; create-patch.py still runs on 3.9.
    sys.exit("validate-patches.py needs Python 3.11 or newer")

ROOT = Path(__file__).resolve().parent.parent
PATCHES = ROOT / "Patches"
DATABASES = ROOT / "AddressDatabases"

SHA256 = re.compile(r"[0-9A-Fa-f]{64}")
PATCH_ID = re.compile(r"[A-Za-z0-9_-]+")
MAJOR_MINOR_PATCH = re.compile(r"[0-9]+\.[0-9]+\.[0-9]+")

MANIFEST_FIELDS = ("id", "name", "version", "author", "description")
HOOK_TYPES = ("simple", "replace", "detour", "static")

# DETOUR and REPLACE overwrite the hook site with a relative JMP: one opcode byte
# and a four byte displacement. Fewer stolen bytes than that cannot hold one.
JMP_LENGTH = 5


def known_versions() -> dict[str, str]:
    """Map every SHA-256 recorded in the address databases to a readable name."""
    known: dict[str, str] = {}
    for db in sorted(DATABASES.glob("*.db")):
        # sqlite3's own context manager commits a transaction, it does not close
        # the connection, so the handle has to be closed separately.
        with contextlib.closing(
            sqlite3.connect(f"file:{db}?mode=ro", uri=True)
        ) as con:
            rows = con.execute(
                "SELECT sha256_hash, game_name, version_string FROM game_version"
            )
            for sha, game, version in rows:
                known[sha.upper()] = f"{game} {version}"
    return known


def byte_values(raw: object) -> list[int] | None:
    """Normalise a TOML byte array to ints, or None if it is not one.

    Both integers and hex strings are accepted, because the runtime's own
    ParseByteArray takes either and patches are free to use whichever reads
    better at the hook site.
    """
    if not isinstance(raw, list):
        return None
    out: list[int] = []
    for item in raw:
        if isinstance(item, int):
            out.append(item)
        elif isinstance(item, str):
            try:
                out.append(int(item, 16))
            except ValueError:
                return None
        else:
            return None
    return out


def check_manifest(
    path: Path, known: dict[str, str]
) -> tuple[dict, list[str], list[str]]:
    """Return the [patch] table, its errors, and its warnings."""
    errors: list[str] = []
    warnings: list[str] = []
    try:
        manifest = tomllib.loads(path.read_text(encoding="utf-8"))
    except (tomllib.TOMLDecodeError, UnicodeDecodeError) as exc:
        return {}, [f"manifest.toml does not parse: {exc}"], warnings

    patch = manifest.get("patch")
    if not isinstance(patch, dict):
        return {}, ["manifest.toml has no [patch] table"], warnings

    for field in MANIFEST_FIELDS:
        if not str(patch.get(field, "")).strip():
            errors.append(f"[patch] is missing {field}")

    patch_id = str(patch.get("id", ""))
    if patch_id and not PATCH_ID.fullmatch(patch_id):
        errors.append(f"id {patch_id!r} has characters outside [A-Za-z0-9_-]")

    # The manager carries the version as a string and only ever displays it, so a
    # version outside the documented format still installs.
    version = str(patch.get("version", ""))
    if version and not MAJOR_MINOR_PATCH.fullmatch(version):
        warnings.append(f"version {version!r} is not major.minor.patch")

    supported = patch.get("supported_versions")
    if not isinstance(supported, dict) or not supported:
        errors.append("[patch.supported_versions] is missing or empty")
        return patch, errors, warnings

    for key, value in supported.items():
        sha = str(value)
        if not SHA256.fullmatch(sha):
            errors.append(
                f"supported_versions.{key} is not a SHA-256 "
                f"({len(sha)} characters): {sha}"
            )
        elif sha.upper() not in known:
            errors.append(
                f"supported_versions.{key} matches no address database: {sha}"
            )
    return patch, errors, warnings


def check_hooks(path: Path, supported: set[str]) -> list[str]:
    """Everything wrong with one hooks.toml, relative to its manifest."""
    errors: list[str] = []
    try:
        document = tomllib.loads(path.read_text(encoding="utf-8"))
    except (tomllib.TOMLDecodeError, UnicodeDecodeError) as exc:
        return [f"{path.name} does not parse: {exc}"]

    # A bare hooks.toml carrying no [metadata] applies to every version the
    # manifest supports, so target_versions is only checked when it is present.
    for target in document.get("metadata", {}).get("target_versions", []):
        if str(target).upper() not in supported:
            errors.append(
                f"{path.name} targets a version the manifest does not "
                f"support: {target}"
            )

    # No [[hooks]] is the DLL_ONLY profile, where the patch DLL installs its own
    # hooks from DllMain and there is nothing here for the runtime to apply.
    hooks = document.get("hooks")
    if not isinstance(hooks, list):
        return errors

    for index, hook in enumerate(hooks):
        where = f"{path.name} hook {index}"
        kind = hook.get("type")
        if kind not in HOOK_TYPES:
            errors.append(f"{where} has unknown type {kind!r}")
            continue
        if not isinstance(hook.get("address"), int):
            errors.append(f"{where} has no integer address")

        original = byte_values(hook.get("original_bytes"))
        if original is None:
            errors.append(f"{where} has no readable original_bytes")
            continue
        if any(b < 0 or b > 0xFF for b in original):
            errors.append(f"{where} has an original_bytes value outside 0-255")
        if kind in ("detour", "replace") and len(original) < JMP_LENGTH:
            errors.append(
                f"{where} steals {len(original)} bytes, too few for a "
                f"{JMP_LENGTH}-byte JMP"
            )

        if kind == "detour":
            # The patch DLL carries the code, so a detour has nothing to write.
            if not str(hook.get("function", "")).strip():
                errors.append(f"{where} is a detour with no function")
            continue

        replacement = byte_values(hook.get("replacement_bytes"))
        if replacement is None:
            errors.append(f"{where} has no readable replacement_bytes")
            continue
        if any(b < 0 or b > 0xFF for b in replacement):
            errors.append(f"{where} has a replacement_bytes value outside 0-255")

        # REPLACE is jumped to and may be any length. SIMPLE and STATIC are
        # written over the original in place.
        if kind != "replace" and len(original) != len(replacement):
            errors.append(
                f"{where} replaces {len(original)} bytes with {len(replacement)}"
            )
    return errors


def main() -> int:
    known = known_versions()
    if not known:
        print(f"no game versions found in {DATABASES}", file=sys.stderr)
        return 1

    failed = 0
    warned = 0
    checked = 0
    seen_ids: dict[str, str] = {}

    for directory in sorted(PATCHES.iterdir()):
        manifest_path = directory / "manifest.toml"
        if not manifest_path.is_file():
            continue
        checked += 1

        patch, errors, warnings = check_manifest(manifest_path, known)

        patch_id = str(patch.get("id", ""))
        if patch_id:
            if patch_id in seen_ids:
                errors.append(f"id {patch_id!r} is already used by {seen_ids[patch_id]}")
            else:
                seen_ids[patch_id] = directory.name

        supported = {
            str(v).upper() for v in patch.get("supported_versions", {}).values()
        }
        hook_files = sorted(directory.glob("*hooks.toml"))
        if not hook_files:
            errors.append("no *hooks.toml")
        for hook_file in hook_files:
            errors.extend(check_hooks(hook_file, supported))

        if errors:
            failed += 1
            print(f"FAIL {directory.name}")
            for error in errors:
                print(f"       {error}")
        if warnings:
            warned += 1
            print(f"WARN {directory.name}")
            for warning in warnings:
                print(f"       {warning}")

    print(
        f"\n{checked} patches checked, {failed} with errors, "
        f"{warned} with warnings"
    )
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
