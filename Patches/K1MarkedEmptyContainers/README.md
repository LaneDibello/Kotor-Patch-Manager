# K1 Marked Empty Containers

Appends `" (empty)"` to the floating hover name of any placeable container or lootable corpse (`"Remains"`) that currently holds zero items, allowing you to tell an empty container from a full one at a glance without opening it.

- **Hover-label only:** Modifies only the temporary `CExoString` passed to `CSWGuiTargetActionMenu::SetNameLabel`. The container's real name, the loot window header, and game scripts are completely untouched.
- **Live updates:** Shows `" (empty)"` on first sight for initially empty containers and updates immediately the moment you take the last item out of a container.

---

## Supported Game Versions

| Platform / Release | Executable SHA-256 | Hooks File |
| :--- | :--- | :--- |
| **Windows GOG 1.03** | `9C10E0450A6EECA417E036E3CDE7474FED1F0A92AAB018446D156944DEA91435` | `kotor1.hooks.toml` |
| **Windows Editable 1.03** | `761F9466F456A83909036BAEBB5C43167D722387BE66E54617BA20A8C49E9886` | `kotor1.hooks.toml` |
| **Windows Steam 1.03** | `34E6D971C034222A417995D8E1E8FDD9F8781795C9C289BD86C499A439F34C88` | `kotor1.hooks.toml` |
| **macOS Steam Aspyr (64-bit)** | `C1FCB8D37C702849882A17751C63EE0AF7C2B9CBBC3B31B98A5F0EDBC27C6D71` | `kotor1-steam-aspyr-macos.hooks.toml` |

---

## How It Works

When the player hovers over or targets an object in the world, `CSWGuiTargetActionMenu::UpdateNameLabel` resolves the client object's server twin (`CSWSObject*`), assembles its display name into a stack-local temporary `CExoString`, and calls `CSWGuiTargetActionMenu::SetNameLabel` to display it.

This patch installs a `detour` hook on the 7-byte argument-setup instructions immediately preceding `call SetNameLabel` and passes two pointers into `MarkEmptyContainer` (`MarkedEmptyContainers.cpp`):
1. `exoName`: The address of the temporary `CExoString` holding the assembled hover label.
2. `serverObject`: The `CSWSObject*` register (`ebx` on Windows, `r14` on macOS) holding the hovered server object.

Inside `MarkEmptyContainer`:
1. If `exoName` is null or empty (which also covers the case where nothing is hovered), it returns early.
2. It checks whether `serverObject`'s vtable pointer matches `CSWSPlaceable`.
3. It checks whether `CSWSPlaceable::m_bHasInventory` is non-zero (distinguishing lootable containers from non-container placeables like control panels or terminals).
4. It calls `CSWSPlaceable::GetItemCount(serverObject, 0)` to count all items inside the container (including plot items).
5. If the item count is `0`, it formats `"<name> (empty)"` into a stack buffer and assigns it back to the temporary `CExoString` via `CExoString::operator=(char const*)`.
6. Execution returns to the trampoline, which runs the stolen 7-byte argument setup and proceeds into `CSWGuiTargetActionMenu::SetNameLabel`.

### Engine Addresses & Offsets

| Symbol / Offset | Windows 1.03 (32-bit x86) | macOS Aspyr (64-bit x86\_64) |
| :--- | :--- | :--- |
| **`CSWGuiTargetActionMenu::UpdateNameLabel`** | `0x00685CB0` | `0x100230CB4` |
| **Detour Hook Site (before `call SetNameLabel`)** | `0x00685E62` (`7` bytes) | `0x100230E87` (`7` bytes) |
| **`CSWGuiTargetActionMenu::SetNameLabel`** | `0x00685AF0` | `0x1002309C6` |
| **`CSWSPlaceable` vtable** | `0x007494D0` | `0x1005B2390` |
| **`CSWSPlaceable::m_bHasInventory`** | `+0x324` | `+0x490` |
| **`CSWSPlaceable::GetItemCount(int)`** | `0x00587710` | `0x100416E98` |
| **`CExoString::operator=(char const*)`** | `0x005E5140` | `0x10034CED4` |

---

