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

### 9. First-Person Camera Toggle `NULL` Party Leader Safety (<kbd>Caps Lock</kbd> Crash)
- **Target**: `CClientGui::HandleKeyEvent` (`0x1002fa0a1`)
- **Hook**: Replace at `0x1002faf87`
- **Issue**: Pressing <kbd>Caps Lock</kbd> to enter First-Person Look Mode (`0x1002faf82`) calls `CSWPartyTable::GetPartyLeader()` (`0x1002a70ae`) and immediately dereferences `0x8(%rax)` at `0x1002faf87` without a `NULL` check. If the client party table is empty (for example, during scripted sequences or after `UT_ClearAllPartyMembers` when no party leader is active), `GetPartyLeader()` returns `NULL` and pressing <kbd>Caps Lock</kbd> causes an immediate `EXC_BAD_ACCESS` crash.
- **Fix**: Guards against `GetPartyLeader() == NULL` at `0x1002faf87` by jumping cleanly to the function epilogue (`0x1002fadee`) instead of dereferencing `0x8(%rax)`.

### 10. Leading Newline Trim in GUI Text
- **Target**: `CSWGuiTextParams::SetText` (`0x1004a3714`)
- **Hook**: Detour at `0x1004a3726` calling `KMRP_TrimLeadingNewlines`
- **Issue**: Item and quest descriptions prefix each property line with `\n`. Descriptions that start with property blocks open with a newline, rendering an unnecessary empty top line (~16px in vanilla, worse with scaled fonts).
- **Fix**: Trims leading newlines in-place right after `CExoString` assignment before the string reaches the text layout object.

---

## Summary of All 19 Hooks

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
| 18 | `0x1002faf87` | `CClientGui::HandleKeyEvent` | `replace` | 9. Caps Lock Crash | Guards First-Person Look Mode (<kbd>Caps Lock</kbd>) against `GetPartyLeader() == NULL`. |
| 19 | `0x1004a3726` | `CSWGuiTextParams::SetText` | `detour` | 10. Leading Newlines | Calls `KMRP_TrimLeadingNewlines` to strip leading `\n` from item/quest descriptions. |
