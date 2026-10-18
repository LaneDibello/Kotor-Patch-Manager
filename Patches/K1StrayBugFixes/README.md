# Stray Bug Fixes Patch

A standalone collection of pure vanilla engine bug fixes for Star Wars: Knights of the Old Republic (KotOR 1) on macOS (Aspyr 64-bit AMD64 binary).

These fixes resolve fundamental engine bugs present in the vanilla game that benefit **all players**, regardless of whether they play in standard 4:3, widescreen, or high-resolution display modes.

---

## Included Fixes

### 1. K1. Word-Wrap Infinite Loop Hang ("Inventory Crash")
- **Target**: `CAurGUIStringInternal::WrapStrings` (`0x1001bc644`)
- **Hooks**:
  - Replace hook at `0x1001bc7ec`
  - Simple hook at `0x1001bc71e`
  - Simple hook at `0x1001bc738`
- **Issue**: When a text line with no breakable spaces cannot fit even two characters, the engine backs up one character and restarts. The vanilla progress guard compared `r8` (cursor) against `[rsp+0x18]` (start of the entire string) instead of `r15` (start of the current line). Because cursor > string start, the engine believed progress was occurring, looping indefinitely and allocating line objects until memory was exhausted.
- **Fix**: Tests progress against line start `r15`. When no progress occurs, takes the remainder of the line whole and routes to the engine's line-end path at `0x1001bca69`, degrading unfittable text to a single overflowing line rather than hanging. Bypasses narrow label blanking guards at `0x1001bc71e` and `0x1001bc738`.

### 2. K2. Listbox Row Height Inflation on Repopulate
- **Target**: `CSWGuiListBox::OrganizeControls` (`0x1004a82b4`)
- **Hooks**:
  - Simple hook at `0x1004a8927` (`89 5d cc -> 90 90 90`)
  - Simple hook at `0x1004a8939` (`89 45 cc -> 90 90 90`)
- **Issue**: The visible-row loop distributes leftover listbox height across rows by making each row taller and writing the enlarged height back into `[rbp-0x34]`. Reopening or refilling a list recomputes item height from this inflated value, compounding row heights each time.
- **Fix**: NOPs both writes to `[rbp-0x34]`, keeping row heights stable across repopulations while preserving row spacing advancement (`ebx`).

### 3. K3. Leading Newline Trim in GUI Text
- **Target**: `CSWGuiTextParams::SetText` (`0x1004a3714`)
- **Hook**: Detour hook at `0x1004a3726` calling `KMRP_TrimLeadingNewlines` in `stray_bug_fixes.cpp`
- **Issue**: Item and quest descriptions are constructed by prefixing each property line with `\n`. Descriptions starting with properties open with a newline, rendering an empty top line (~16px in vanilla, magnified with enlarged fonts).
- **Fix**: Intercepts right after `CExoString` assignment and trims leading newlines in-place before the string reaches the text object, keeping the line-breaker and renderer in agreement.

### 4. K5. Single Line Taller Than Box Vanishing
- **Target**: `CAurGUIStringInternal::Draw` (`0x1001bcb04`)
- **Hooks**:
  - Replace hooks at `0x1001bcbef`, `0x1001bcc22` (bottom-aligned)
  - Replace hooks at `0x1001bcc72`, `0x1001bcca9` (centered)
- **Issue**: When text exceeds its bounding box height, `Draw` drops lines from the top until the rest fits. A single line taller than its box is dropped entirely and nothing is drawn, causing stack counts and labels to disappear.
- **Fix**: Guards against dropping the final remaining line. Text that partly fits behaves as before; text that cannot fit even one line is drawn overflowing instead of vanishing.

### 5. K6. Wrapped Text Measurement Under-Estimation
- **Target**: `CAurGUIStringInternal::WrapStrings` (`0x1001bca20`)
- **Hook**: Simple hook at `0x1001bca20` (`f3 0f 58 05 78 22 3b 00 -> f3 0f 58 05 9c b3 37 00`)
- **Issue**: The line-breaker truncates every glyph advance to whole pixels after adding `0.25f` (`0x10056eca0`), under-measuring line widths by ~0.25px per character relative to `Draw`. Long wrapped lines often exceeded their container box and collided with scrollbars.
- **Fix**: Repoints the displacement to the engine's existing `0.5f` constant (`0x100537dc4`), restoring unbiased half-up rounding.

### 6. K7. Scripts Menu Enter Key Bug
- **Target**:
  - **macOS**: Enter-key button callback for script list rows (`0x1002d3e34`)
  - **Windows (v1.03 PE32)**: `CSWGuiScriptSelect::OnScriptSelected` (`0x006E9E70`)
- **Hooks**:
  - **macOS**: Simple hook at `0x1002d3e34` (`55 -> c3`, 1 byte `retq`)
  - **Windows**: Simple hook at `0x006E9E70` (`56 8b f1 -> c2 04 00`, 3 bytes `ret $4`)
- **Issue**: Opening the AI Scripts selection menu from the Character Sheet displays Tutorial Box 10 ("Combat Scripts"). Pressing Enter to dismiss the tutorial immediately closed both the popup and the Scripts menu, whereas clicking "OK" with the mouse kept the menu open. Each script row control in the list registered an Enter (`0x27`) button callback (`0x1002d3e34` on macOS, `0x006E9E70` on Windows) that hijacked Enter keypresses intended for the tutorial popup, popping the modal panel and setting the `0x200` closing flag on `CSWGuiScriptSelect`.
- **Fix**: Replaces the function's entry with an immediate return (`retq` on macOS, `ret $4` on Windows). This neutralizes the premature row callback, allowing Enter to dismiss the tutorial popup cleanly on the first press. Once dismissed, native Enter handling in `CSWGuiScriptSelect::HandleInputEvent` and the Select button confirm scripts normally.

### 7. Texture Bucket Array Bounds & Maximum Safety
- **Targets**:
  - `Texture bucket insertion` (`0x1001d0663`)
  - `GetMaxTextureID` (`0x1001fa2bb`)
- **Hooks**:
  - Replace hook at `0x1001d0663`
  - Replace hook at `0x1001fa2bf`
- **Issue**: The engine indexes three 5000-entry internal arrays using raw OpenGL texture IDs without bounds checking. IDs $\ge 5000$ index past the end of the arrays, causing memory corruption. Additionally, in the all-textures builder path, the engine sets the maximum texture ID without a range check, which the bucket-clearing loop then uses.
- **Fix**: The insertion hook accepts unsigned IDs 0..4999 and bypasses insertion directly to the native shadow path continuation (`0x1001d067b`) for IDs $\ge 5000$. The maximum hook reads global `0x100635ba8` and caps its value at 4999 (preserving `RFLAGS`), eliminating out-of-bounds indexing across both insertion and clearing loops.

### 8. Grass Buffer Cleanup Double-Free Safety
- **Targets**:
  - Grass destructor cleanup loop (`0x1001e00cd`)
  - `DestroyGrassPolys` cleanup loop (`0x1001e1627`)
- **Hooks**:
  - Replace hook at `0x1001e00cd`
  - Replace hook at `0x1001e1627`
- **Issue**: In the grass rendering system, `+0x38` is the primary buffer and `+0x40` is a temporary alias pointer. When an allocation fails or early cleanup occurs, both cleanup routines attempt to delete the temporary pointer and then the primary pointer even when both point to the same memory allocation, triggering a heap double-free crash.
- **Fix**: Compares the temporary pointer in `[rbx+0x40]` against the primary pointer in `[rbx+0x38]`. If they point to the same allocation, `%rdi` is zeroed so the native `je` skips the redundant `free` call. When pointers differ, the temporary allocation is freed normally.

---

## Packaging & Dependencies

- **Patch ID**: `k1-stray-bug-fixes-patch`
- **Supported Binary**: Aspyr macOS 64-bit AMD64 (`C1FCB8D37C702849882A17751C63EE0AF7C2B9CBBC3B31B98A5F0EDBC27C6D71`)
- **Integration**: The Widescreen Patch declares `requires = ["k1-stray-bug-fixes-patch"]` in its `manifest.toml`. KotOR Patch Manager (KPM) automatically verifies and installs this patch prior to installing the Widescreen Patch.
```
