# Stray Bug Fixes Patch

A collection of pure vanilla engine bug fixes for Star Wars: Knights of the Old Republic (KotOR 1) on macOS (Aspyr 64-bit AMD64 binary).

These fixes resolve fundamental engine bugs present in the vanilla game that benefit **all players**, regardless of whether they play in standard 4:3, widescreen, or high-resolution display modes.

---

## Included Fixes

### 1. Word-Wrap Infinite Loop Hang ("Inventory Crash")
- **Target**: `CAurGUIStringInternal::WrapStrings` (`0x1001bc644`)
- **Hooks**: Replace at `0x1001bc7ec`; Simple at `0x1001bc71e`, `0x1001bc738`
- **Issue**: When a long word with no breakable spaces cannot fit even two characters in its container box, the engine attempts to back up one character and restart line-breaking. The vanilla progress guard compared the cursor (`%r8`) against the start of the entire string (`[rsp+0x18]`) instead of the start of the current line (`%r15`). Because `cursor > string_start`, the engine falsely believed progress was occurring, looping indefinitely and allocating line objects until memory exhausted and the game crashed.
- **Fix**: Tests progress against the current line start (`%r15`). When no progress occurs, accepts the remainder of the line whole and routes to the engine's line-end path (`0x1001bca69`), degrading unfittable text to a single overflowing line rather than hanging. Also bypasses narrow label blanking guards (`0x1001bc71e`, `0x1001bc738`).

### 2. Listbox Row Height Inflation on Repopulate
- **Target**: `CSWGuiListBox::OrganizeControls` (`0x1004a82b4`)
- **Hooks**: Simple at `0x1004a8927`, `0x1004a8939`
- **Issue**: The visible-row loop distributes leftover listbox height across rows by expanding row heights and writing the enlarged height back into `[rbp-0x34]`. Reopening or refilling a list recomputes item height from this inflated value, compounding row heights each time until rows become massive.
- **Fix**: NOPs both writes to `[rbp-0x34]`, keeping row heights stable across repopulations while preserving row spacing advancement (`%ebx`).

### 3. Single Line Taller Than Box Vanishing
- **Target**: `CAurGUIStringInternal::Draw` (`0x1001bcb04`)
- **Hooks**: Replace at `0x1001bcbef`, `0x1001bcc22` (bottom-aligned); Replace at `0x1001bcc72`, `0x1001bcca9` (centered)
- **Issue**: When text exceeds its bounding box height, `Draw` drops lines from the top until the rest fits. A single line taller than its box (such as inventory stack count numbers or short labels) was dropped entirely, rendering nothing.
- **Fix**: Guards against dropping the final remaining line. Text that partly fits behaves normally; a single line taller than its box is drawn overflowing instead of disappearing.

### 4. Wrapped Text Measurement Under-Estimation
- **Target**: `CAurGUIStringInternal::WrapStrings` (`0x1001bca20`)
- **Hook**: Simple at `0x1001bca20`
- **Issue**: The line-breaker truncated every glyph advance to whole pixels after adding `0.25f` (`0x10056eca0`), under-measuring line widths by ~0.25px per character relative to `Draw`. Long wrapped lines often exceeded their container box and collided with scrollbars.
- **Fix**: Repoints the displacement to the engine's existing `0.5f` constant (`0x100537dc4`), restoring unbiased half-up rounding.

### 5. Texture Bucket Array Bounds & Maximum Safety
- **Targets**: Texture bucket insertion (`0x1001d0663`), `GetMaxTextureID` (`0x1001fa2bb`)
- **Hooks**: Replace at `0x1001d0663`, `0x1001fa2bf`
- **Issue**: The engine indexes three 5000-entry internal arrays using raw OpenGL texture IDs without bounds checking. IDs $\ge 5000$ index past the end of the arrays, causing memory corruption. Additionally, the maximum texture ID global could be set without range validation, causing the bucket clearing loop to scan out of bounds.
- **Fix**: The insertion hook accepts IDs 0..4999 and skips directly to continuation (`0x1001d067b`) for IDs $\ge 5000$. The maximum hook caps the global value at 4999, preventing out-of-bounds indexing in both insertion and clearing loops.

### 6. Grass Buffer Cleanup Double-Free Safety
- **Targets**: Grass destructor (`0x1001e00cd`), `DestroyGrassPolys` (`0x1001e1627`)
- **Hooks**: Replace at `0x1001e00cd`, `0x1001e1627`
- **Issue**: In the grass rendering system, `+0x38` is the primary buffer and `+0x40` is a temporary alias pointer. When an allocation fails or early cleanup occurs, both cleanup routines attempt to delete the temporary pointer and then the primary pointer even when both point to the same allocation, triggering a heap double-free crash.
- **Fix**: Compares the temporary pointer in `[rbx+0x40]` against the primary pointer in `[rbx+0x38]`. If identical, zeroes `%rdi` so the native branch skips the redundant `free()` call.

### 7. Scripts Menu Enter Key Bug & Double-Click Selection
- **Target**: Script list row activation/double-click callback (`0x1002d3e34`)
- **Hook**: Replace at `0x1002d3e34`
- **Issue**: Opening the AI Scripts menu displays Tutorial Box 10 ("Combat Scripts"). Each script row registers a callback for event `0x27` (shared by the <kbd>Enter</kbd> key and mouse double-clicks). Pressing <kbd>Enter</kbd> to dismiss the tutorial box triggered this callback, prematurely closing the entire Scripts menu. Conversely, naively disabling the callback broke row double-clicking.
- **Fix**: Verifies call stack origin (`(%rsp) == 0x1004a9221` and `8(%rbp) == 0x1004aa8b1`). When called by a genuine mouse double-click (`CSWGuiListBox::OnMouseClick`), it executes normal script selection and auto-close. When called by keyboard <kbd>Enter</kbd>, it returns immediately (`retq`), allowing <kbd>Enter</kbd> to dismiss the tutorial popup cleanly without closing the menu.

### 8. Module Key Table Destruction Use-After-Free Safety
- **Targets**: `CExoKeyTable::DestroyTable` (`0x10035be6e`), `CExoKeyTable::RebuildTable` (`0x10035c300`)
- **Hooks**: Simple at `0x10035bf7a`, Replace at `0x10035c434`
- **Issue**: When unloading a module capsule (`.mod`/`.rim`/`.sav`), `DestroyTable` iterates over entries before freeing `m_pEntries`. If a resource is still in demand (`m_nDemands > 0`), the engine failed to clear `m_pRes->m_pKeyEntry` (`+0x20`) and `m_pRes->m_nID` (`+0x0c`), leaving a dangling pointer into the freed table. Subsequent resource release wrote zeroes to the freed block, corrupting malloc checksums and causing `SIGABRT` crashes during module transitions.
- **Fix**: Clears `m_pRes->m_pKeyEntry` to `NULL` and `m_pRes->m_nID` to `0xFFFFFFFF` in both `DestroyTable` and `RebuildTable`.

### 9. `SwitchPlayerCharacter` Client Party Table & First-Person Camera Safety
- **Targets**: `CSWSPartyTable::SwitchPlayerCharacter` (`0x1003e1b34`), `CClientGui::HandleKeyEvent` (`0x1002fa0a1`)
- **Hooks**: Replace at `0x1003e1f67`, `0x1003e202d`, `0x1002faf87`
- **Issue**: When `SwitchPlayerCharacter(nNPC)` is called for an NPC whose creature object already exists in the area (for example, when a companion was in the active party on module entry and the module's `OnEnter` script calls `RemovePartyMember` followed by `SwitchPlayerCharacter`, as in *Brotherhood of Shadow: Solomon's Revenge*'s `bos_ebovis` module), `SwitchPlayerCharacter` reuses the existing server `CSWSCreature` (`0x10(%r12,%rbx,8)`) instead of calling `SpawnAvailableNPC`, then calls `CSWPartyTable::ClearPartyTable()` (`0x1002a7220`) to clear the client party table (`m_nNumPartyMembers = 0`), and calls `CSWSCreature::SetPC(1, 1)` (`0x100490adc`) on the server creature. Unlike `AddNPC` (`0x1003df52f`) and `LoadPartyTable` (`0x1003df6ba`), however, `SwitchPlayerCharacter` forgets to call `CSWCCreature::SetPartyMember(1)` (`0x1002a18f4`) on the already-existing client `CSWCCreature`, and subsequent client `GameObjUpdate_AddCreature` ticks (`0x10030bd2a`) skip `SetPartyMember(1)` because the client creature already exists. This leaves the client party table permanently empty (`m_nNumPartyMembers == 0`), causing:
  1. `CClientGui::Update` (`0x1002684ea`) to see `GetPartyMember(0) == NULL` once any opening dialog camera ends and skip `CClientGui::UpdateActiveRoom` (`0x100269b34`) on every frame, freezing `Scene->m_pActiveRoom` at the starting room and culling non-adjacent rooms to the solid `glClearColor` backdrop.
  2. The HUD to have no party leader (`GetPartyLeader() == NULL`).
  3. Pressing <kbd>Caps Lock</kbd> to enter First-Person Look Mode (`0x1002faf82`) to call `CSWPartyTable::GetPartyLeader()` and dereference `0x8(%rax)` at `0x1002faf87` without a `NULL` check, causing an immediate `EXC_BAD_ACCESS` crash.
  Additionally, if `SwitchPlayerCharacter(nNPC)` is called while `nNPC` is still in the server active party list, the cleanup loop at `0x1003e2012` calls `DestroyAvailableNPC(nNPC, 1)` (`0x1003e1660`) on every active party member including `nNPC` itself, destroying the newly switched player character.
- **Fix**: Hook `0x1003e1f67` looks up the switched PC's client `CSWCCreature` right after `SetPC(1, 1)` and, if it already exists, resets `m_bPartyMember` (`+0x508`) to `0` and calls `CSWCCreature::SetPartyMember(1)` (`0x1002a18f4`) to register it in slot 0 of the client `CSWPartyTable`. Hook `0x1003e202d` skips `DestroyAvailableNPC` when the loop index matches the target NPC (`-0x44(%rbp)`). Hook `0x1002faf87` guards against `GetPartyLeader() == NULL` on <kbd>Caps Lock</kbd> by jumping cleanly to the function epilogue (`0x1002fadee`).

### 10. Leading Newline Trim in GUI Text
- **Target**: `CSWGuiTextParams::SetText` (`0x1004a3714`)
- **Hook**: Detour at `0x1004a3726` calling `KMRP_TrimLeadingNewlines`
- **Issue**: Item and quest descriptions prefix each property line with `\n`. Descriptions that start with property blocks open with a newline, rendering an unnecessary empty top line (~16px in vanilla, worse with scaled fonts).
- **Fix**: Trims leading newlines in-place right after `CExoString` assignment before the string reaches the text layout object.

---

## Summary of All 21 Hooks

| # | Address | Function / Target | Type | Bug Category | What It Does |
|---|---------|-------------------|------|--------------|--------------|
| 1 | `0x1001bc7ec` | `CAurGUIStringInternal::WrapStrings` | `replace` | 1. Word-Wrap Hang | Tests progress against current line start (`%r15`), preventing infinite loop memory exhaustion crash. |
| 2 | `0x1001bc71e` | `CAurGUIStringInternal::WrapStrings` | `simple` | 1. Word-Wrap Hang | Bypasses narrow label blanking check. |
| 3 | `0x1001bc738` | `CAurGUIStringInternal::WrapStrings` | `simple` | 1. Word-Wrap Hang | Bypasses secondary narrow label blanking check. |
| 4 | `0x1004a8927` | `CSWGuiListBox::OrganizeControls` | `simple` | 2. Listbox Row Inflation | NOPs write to `[rbp-0x34]` to prevent compounding row heights across repopulations. |
| 5 | `0x1004a8939` | `CSWGuiListBox::OrganizeControls` | `simple` | 2. Listbox Row Inflation | NOPs secondary write to `[rbp-0x34]`. |
| 6 | `0x1001bcbef` | `CAurGUIStringInternal::Draw` | `replace` | 3. Vanishing Text | Bottom-aligned text: prevents dropping final remaining line taller than container box. |
| 7 | `0x1001bcc22` | `CAurGUIStringInternal::Draw` | `replace` | 3. Vanishing Text | Bottom-aligned text: secondary check preventing dropping last line. |
| 8 | `0x1001bcc72` | `CAurGUIStringInternal::Draw` | `replace` | 3. Vanishing Text | Centered text: prevents dropping final remaining line taller than container box. |
| 9 | `0x1001bcca9` | `CAurGUIStringInternal::Draw` | `replace` | 3. Vanishing Text | Centered text: secondary check preventing dropping last line. |
| 10 | `0x1001bca20` | `CAurGUIStringInternal::WrapStrings` | `simple` | 4. Measurement Error | Repoints rounding offset from `0.25f` to `0.5f` (`0x100537dc4`), preventing text clipping into scrollbars. |
| 11 | `0x1001d0663` | Texture Bucket Insertion | `replace` | 5. Texture Bucket OOB | Bounds-checks OpenGL texture IDs ($< 5000$) before indexing internal arrays. |
| 12 | `0x1001fa2bf` | `GetMaxTextureID` | `replace` | 5. Texture Bucket OOB | Clamps maximum texture ID to 4999 to prevent out-of-bounds scanning during clearing. |
| 13 | `0x1001e00cd` | Grass Destructor Cleanup | `replace` | 6. Grass Double-Free | Compares primary buffer (`+0x38`) with temporary alias (`+0x40`) to skip duplicate `free()`. |
| 14 | `0x1001e1627` | `DestroyGrassPolys` Cleanup | `replace` | 6. Grass Double-Free | Compares primary buffer with temporary alias during polygon destruction. |
| 15 | `0x1002d3e34` | Script List Row Callback | `replace` | 7. Scripts Enter Key | Inspects call stack: allows mouse double-clicks to select scripts, but ignores keyboard <kbd>Enter</kbd>. |
| 16 | `0x10035bf7a` | `CExoKeyTable::DestroyTable` | `simple` | 8. Key Table UAF | Clears dangling `m_pKeyEntry` and `m_nID` on demanded resources during table destruction. |
| 17 | `0x10035c434` | `CExoKeyTable::RebuildTable` | `replace` | 8. Key Table UAF | Clears dangling `m_pKeyEntry` and `m_nID` during key table rebuilds. |
| 18 | `0x1003e1f67` | `CSWSPartyTable::SwitchPlayerCharacter` | `replace` | 9. SwitchPlayerCharacter | Registers already-existing client `CSWCCreature` in `CSWPartyTable` after `ClearPartyTable()`, preventing frozen room culling and missing party leader. |
| 19 | `0x1003e202d` | `CSWSPartyTable::SwitchPlayerCharacter` | `replace` | 9. SwitchPlayerCharacter | Prevents party cleanup loop from calling `DestroyAvailableNPC` on the newly switched PC. |
| 20 | `0x1002faf87` | `CClientGui::HandleKeyEvent` | `replace` | 9. SwitchPlayerCharacter | Guards First-Person Look Mode (<kbd>Caps Lock</kbd>) against `GetPartyLeader() == NULL`. |
| 21 | `0x1004a3726` | `CSWGuiTextParams::SetText` | `detour` | 10. Leading Newlines | Calls `KMRP_TrimLeadingNewlines` to strip leading `\n` from item/quest descriptions. |

---

## Packaging & Dependencies

- **Patch ID**: `k1-stray-bug-fixes-patch`
- **Supported Binary**: Aspyr macOS 64-bit AMD64 (`C1FCB8D37C702849882A17751C63EE0AF7C2B9CBBC3B31B98A5F0EDBC27C6D71`)
- **Integration**: The Widescreen Patch declares `requires = ["k1-stray-bug-fixes-patch"]` in its `manifest.toml`. KotOR Patch Manager (KPM) automatically verifies and installs this patch prior to installing the Widescreen Patch.
