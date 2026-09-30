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

---

## Packaging & Dependencies

- **Patch ID**: `k1-stray-bug-fixes-patch`
- **Supported Binary**: Aspyr macOS 64-bit AMD64 (`C1FCB8D37C702849882A17751C63EE0AF7C2B9CBBC3B31B98A5F0EDBC27C6D71`)
- **Integration**: The Widescreen Patch declares `requires = ["k1-stray-bug-fixes-patch"]` in its `manifest.toml`. KotOR Patch Manager (KPM) automatically verifies and installs this patch prior to installing the Widescreen Patch.
```
