# Ghidra scripts

## ExportFunctionsCsv.java

Exports named functions from Ghidra to the CSV that `SqliteTools import-ghidra` reads.

Functions are named flat as `Class__Function` in Ghidra's Global namespace, so the
script splits each name on the last `__` into `class_name` / `function_name`. Free
functions (no `__`) get class `Global`, matching the address-DB convention.

### Usage

Run from Ghidra's Script Manager (add this directory via the Bundle Manager first),
pick an output path, then import:

```bash
dotnet run --project tools/SqliteTools -- import-ghidra \
    --csv functions.csv --database AddressDatabases/<db>.db --mode append
```

Output columns: `class_name,function_name,address,calling_convention,param_size_bytes,notes`.

Note: names are Itanium-spelled (`~CResGFF`, `operator==`). A few differ from the
GameAPI aliases the DB uses (`HasSpell` vs `HasSpellPrereq`) and need re-aliasing
after import; most are identical.
