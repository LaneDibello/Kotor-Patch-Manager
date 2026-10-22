#!/usr/bin/env python3
"""Merge and diff the address databases (AddressDatabases/*.db) row by row.

The databases are SQLite files, so git sees two branches that each added a few
functions as two unrelated binaries and stops on a conflict nobody can resolve
in a text editor. Their contents merge cleanly almost every time, though: rows
are looked up by name, and most changes add rows.

So this compares the three versions git hands a merge driver (the common
ancestor, ours, theirs) table by table, matching rows by their natural key
rather than by id:

    functions        class_name, function_name
    global_pointers  pointer_name
    offsets          class_name, member_name
    classes          class_name
    game_version     sha256_hash
    schema_version   version

The integer ids are left out of every comparison. They are autoincrement
values, so two branches that each add a row hand out the same id to different
rows, and nothing reads them: every lookup in GameVersion.cpp and the manager is
by name. Nor does a schema_version row's applied_date: two branches that ran the
same migration on different days agree.

For each key, a change made on only one side is taken (an added row is inserted,
a changed one updated, a removed one deleted), and a change made the same way on
both sides is taken once. A row changed differently on the two sides is a
conflict only where the same field was changed differently: a row whose address
one side changed and whose notes the other did takes both. For a real conflict
ours is kept, every such row is listed with all three values, and the exit
status is 1, so git marks the file conflicted.

If one side has run a schema migration the other has not, the merge first
brings ours up to theirs: their new tables, columns and indexes are added (every
migration so far has been additive), and rows from the side that is behind read
the new columns as NULL, as the migration leaves them. Anything a merge cannot
do by adding is refused, and ours is left as it was.

Setup, once per clone (git will not run a driver a repository names by itself):

    git config merge.addressdb.name "KotOR address database merge"
    git config merge.addressdb.driver "python tools/merge-address-db.py %O %A %B %P"
    git config diff.addressdb.textconv "python tools/merge-address-db.py --dump"

.gitattributes routes *.db to both. Without the config, a .db still merges as a
binary, as before.

    merge-address-db.py <ancestor> <ours> <theirs> [<path>]   merge into <ours>
    merge-address-db.py --dump <database>                    one line per row, sorted
"""

from __future__ import annotations

import sqlite3
import sys
from pathlib import Path

KEYS = {
    "functions": ("class_name", "function_name"),
    "global_pointers": ("pointer_name",),
    "offsets": ("class_name", "member_name"),
    "classes": ("class_name",),
    "game_version": ("sha256_hash",),
    "schema_version": ("version",),
}

# Columns that don't count as a change: when a migration ran is not a difference between two
# branches that both ran it.
IGNORED = {"schema_version": {"applied_date"}}

# Columns shown in hex by --dump: addresses and offsets read the way Ghidra shows them.
HEX = {"address", "offset", "size", "vtable"}


def connect(path: str, write: bool = False) -> sqlite3.Connection:
    mode = "rw" if write else "ro"
    # autocommit: the merge runs in a transaction of its own, so it can roll back its schema
    # changes too (the module's implicit transactions leave DDL outside them)
    return sqlite3.connect(f"file:{Path(path).resolve().as_posix()}?mode={mode}", uri=True,
                           isolation_level=None)


def tables(db: sqlite3.Connection) -> dict[str, list[str]]:
    """Every table's columns in order, without the id."""
    names = [r[0] for r in db.execute(
        "SELECT name FROM sqlite_master WHERE type = 'table' AND name NOT LIKE 'sqlite_%' ORDER BY name")]
    return {n: [c[1] for c in db.execute(f'PRAGMA table_info("{n}")') if c[1] != "id"] for n in names}


def catch_up(ours: sqlite3.Connection, theirs: sqlite3.Connection) -> list[str]:
    """Bring ours up to theirs' schema where theirs is ahead: its new tables, columns and indexes.

    Every migration so far has been additive (classes; game_version.platform; classes.vtable), so
    this is all one side's migration takes. Anything else raises, and nothing is merged."""
    done = []
    mine = tables(ours)
    for name, sql in theirs.execute("SELECT name, sql FROM sqlite_master WHERE type = 'table' "
                                    "AND name NOT LIKE 'sqlite_%' ORDER BY name"):
        if name not in mine:
            ours.execute(sql)
            done.append(f"table {name}")
            continue
        for _cid, col, ctype, notnull, default, _pk in theirs.execute(f'PRAGMA table_info("{name}")'):
            if col in mine[name] or col == "id":
                continue
            clause = f'ALTER TABLE "{name}" ADD COLUMN "{col}" {ctype}'
            if default is not None:
                clause += f" DEFAULT {default}" + (" NOT NULL" if notnull else "")
            elif notnull:
                raise ValueError(f"{name}.{col} is NOT NULL with no default, which no added column can be")
            ours.execute(clause)
            done.append(f"column {name}.{col}")
    have = {r[0] for r in ours.execute("SELECT name FROM sqlite_master WHERE type = 'index'")}
    for name, sql in theirs.execute("SELECT name, sql FROM sqlite_master WHERE type = 'index' AND sql IS NOT NULL"):
        if name not in have:
            ours.execute(sql)
            done.append(f"index {name}")
    return done


def rows(db: sqlite3.Connection, table: str, columns: list[str]) -> dict[tuple, tuple]:
    """key -> the row's values in the order of columns (all but the id), for a table merged by
    KEYS. A column this database doesn't have yet (its side hasn't run that migration) reads as
    NULL, as the migration leaves it; a table it doesn't have, as no rows."""
    own = tables(db).get(table)
    if own is None:
        return {}
    key = KEYS[table]
    picked = ", ".join(f'"{c}"' if c in own else "NULL" for c in columns)
    out = {}
    for values in db.execute(f'SELECT {picked} FROM "{table}"'):
        row = dict(zip(columns, values))
        out[tuple(row[k] for k in key)] = values
    return out


def combine(table: str, columns: list[str], bv, ov, tv):
    """A row both sides changed, field by field: each field takes whichever side changed it.
    None if the same field was changed differently on the two sides."""
    skip = IGNORED.get(table, set())
    bv = bv if bv is not None else (None,) * len(columns)
    out = []
    for c, b, o, t in zip(columns, bv, ov, tv):
        if o == t or t == b or c in skip:
            out.append(o)
        elif o == b:
            out.append(t)
        else:
            return None
    return tuple(out)


def compared(table: str, columns: list[str], values):
    """The values a merge compares: all but IGNORED columns (None for no row)."""
    if values is None:
        return None
    skip = IGNORED.get(table, set())
    return tuple(v for c, v in zip(columns, values) if c not in skip)


def show(table: str, columns: list[str], values) -> str:
    if values is None:
        return "(no row)"
    parts = []
    for c, v in zip(columns, values):
        if v is None or c in KEYS[table]:
            continue
        parts.append(f"{c}={v:#x}" if c in HEX and isinstance(v, int) else f"{c}={v!r}")
    return " ".join(parts) or "(row)"


def merge(ancestor: str, ours: str, theirs: str, path: str) -> int:
    base_db, ours_db, theirs_db = connect(ancestor), connect(ours, write=True), connect(theirs)
    unknown = sorted((set(tables(ours_db)) | set(tables(theirs_db))) - set(KEYS))
    if unknown:
        print(f"{path}: no merge key for table(s) {', '.join(unknown)}; add them to KEYS in "
              f"tools/merge-address-db.py. Nothing was merged.", file=sys.stderr)
        return 1

    conflicts, changes = [], {"added": 0, "changed": 0, "removed": 0, "combined": 0}
    ours_db.execute("BEGIN")
    try:
        migrated = catch_up(ours_db, theirs_db)
        layout = tables(ours_db)
        for table, columns in layout.items():
            o = rows(ours_db, table, columns)
            t = rows(theirs_db, table, columns)
            b = rows(base_db, table, columns)
            key = KEYS[table]
            where = " AND ".join(f'"{k}" = ?' for k in key)
            for k in sorted(set(o) | set(t) | set(b), key=repr):
                bv, ov, tv = b.get(k), o.get(k), t.get(k)
                same = lambda x, y: compared(table, columns, x) == compared(table, columns, y)
                if same(ov, tv) or same(tv, bv):
                    continue                    # same on both sides, or only ours changed it
                if same(ov, bv):
                    new = tv                    # only theirs changed it
                elif ov is not None and tv is not None:
                    new = combine(table, columns, bv, ov, tv)
                    if new is None:
                        conflicts.append((table, columns, k, bv, ov, tv))
                        continue
                else:                           # one side removed it, the other changed it
                    conflicts.append((table, columns, k, bv, ov, tv))
                    continue
                kind = "combined" if new is not tv else None
                if new is None:                 # theirs removed it
                    ours_db.execute(f'DELETE FROM "{table}" WHERE {where}', k)
                    changes[kind or "removed"] += 1
                elif ov is None:                # theirs added it
                    names = ", ".join(f'"{c}"' for c in columns)
                    marks = ", ".join("?" for _ in columns)
                    ours_db.execute(f'INSERT INTO "{table}" ({names}) VALUES ({marks})', new)
                    changes[kind or "added"] += 1
                else:
                    sets = ", ".join(f'"{c}" = ?' for c in columns)
                    ours_db.execute(f'UPDATE "{table}" SET {sets} WHERE {where}', (*new, *k))
                    changes[kind or "changed"] += 1
        ours_db.execute("COMMIT")
    except (sqlite3.Error, ValueError) as e:
        ours_db.execute("ROLLBACK")
        print(f"{path}: could not merge ({e}). Nothing was merged; ours is as it was.", file=sys.stderr)
        return 1
    ours_db.execute("VACUUM")
    ours_db.close()

    if migrated:
        print(f"{path}: brought ours up to theirs' schema: {', '.join(migrated)}", file=sys.stderr)
    summary = ", ".join(f"{n} {what}" for what, n in changes.items() if n) or "nothing to take"
    print(f"{path}: took theirs: {summary}", file=sys.stderr)
    if not conflicts:
        return 0
    print(f"{path}: {len(conflicts)} row(s) changed differently on both sides; ours is kept for each:",
          file=sys.stderr)
    for table, columns, k, bv, ov, tv in conflicts:
        name = "::".join(str(x) for x in k)
        print(f"  {table} {name}\n"
              f"    ancestor  {show(table, columns, bv)}\n"
              f"    ours      {show(table, columns, ov)}\n"
              f"    theirs    {show(table, columns, tv)}", file=sys.stderr)
    return 1


def dump(path: str) -> int:
    """The database as text, one line per row in key order, for git diff (textconv)."""
    db = connect(path)
    for table, columns in tables(db).items():
        key = KEYS.get(table)
        if key is None:
            print(f"[{table}] (no merge key)")
            continue
        for k, values in sorted(rows(db, table, columns).items(), key=lambda kv: repr(kv[0])):
            name = "::".join(str(x) for x in k)
            print(f"{table} {name} {show(table, columns, values)}")
    return 0


def main(argv: list[str]) -> int:
    if len(argv) == 2 and argv[0] == "--dump":
        return dump(argv[1])
    if len(argv) in (3, 4):
        return merge(*argv[:3], argv[3] if len(argv) == 4 else argv[1])
    print(__doc__, file=sys.stderr)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
