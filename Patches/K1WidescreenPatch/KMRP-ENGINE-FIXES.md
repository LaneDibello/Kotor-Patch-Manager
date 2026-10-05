# KMRP engine fixes in the widescreen patch

Engine bugs that KMRP (KOTOR Modern Restoration Patch) fixes in the Windows `swkotor.exe`,
ported to the Aspyr macOS build and added to this patch. Most are vanilla bugs that enlarged
text or a larger HUD make visible; the rest are needed for the widescreen layout itself.

- **Target:** `KOTOR_Exe` 1.4.0 (176481), x86_64, SHA-256 `C1FCB8D3…6D71`.
- **Code:** byte and replace hooks in `kotor1-steam-aspyr-macos.hooks.toml` (section *KMRP
  ENGINE FIXES*, plus detours at the end of the file); C++ in `kmrp_engine_fixes.cpp`; small
  changes inside `mac_widescreen.cpp` (see *Changes to the widescreen code*).
- **Build:** from this folder, `python3 ../create-patch.py` compiles `macos_x86_64.dylib` and
  packs the `.kpatch`, as for every KPM patch.
- Contributed by RaymanGT (KMRP).

Every original-bytes entry was checked against the binary, and KPM's own KPatchCore validates
the patch (57 hooks: 41 simple, 6 replace, 10 detour; no overlaps). All fixes were tested in
game on a 14" MacBook Pro (1512x982 points, 3024x1964 pixels) through KotorPatcher, at
`FontScale` 1 and 2, and at 3024x1964 with `FontScale` 3.
The patch also fixes the widescreen code's double drawing of every window, which made the
main-menu animation run too fast and at half the frame rate (see *Changes to the widescreen
code*). It adds one setting, `UseGuiFileLayouts`, off by default (see *Menus already laid
out for the resolution*).
*Corrected 2026-09-29:* earlier drafts of this change also added `NativeResolution` and
`FullWidthMenus`. Both are gone: a resolution at the display's pixel size is asked for with the
existing `ForceWidth` and `ForceHeight`, which K9 makes work, and full-width menus come with
`UseGuiFileLayouts`.

## Summary

| | Fix | Where | Visible without it |
| --- | --- | --- | --- |
| K1 | Word-wrap forward progress | `WrapStrings` `0x1001bc7ec` + two guards | game hangs (infinite loop) on unbreakable text in a narrow box |
| K2 | List rows stop growing | `CSWGuiListBox::OrganizeControls` `0x1004a8927`, `0x1004a8939` | list rows get taller every time a list is refilled |
| K3 | Leading newline trimmed | `CSWGuiTextParams::SetText` `0x1004a3726` | item and quest descriptions start with an empty line |
| K4 | Video mode follows the widescreen target | `ReadAndSetVideoMode` `0x10026ed44` | fullscreen frame cropped to the INI size (1024x768 by default) |
| K5 | A line taller than its box is kept | `CAurGUIStringInternal::Draw`, 4 sites | stack counts vanish once the font outgrows the badge |
| K6 | Wrapped lines measured with a 0.5px margin | `WrapStrings` `0x1001bca20` | long lines can run into the scrollbar |
| K7 | Dialogue letterbox sized from the height | 6 sites via one constant, 2 immediates, `0x100244d7d` | bars negative past 21:9; enlarged replies scroll in a half-empty bar |
| K8 | Minimap keeps the vanilla zoom | minimap draw, 3 detours | enlarged radar shows 1.6x more area, everything smaller |
| K9 | Retina display modes | Aspyr's mode list `0x10001de6c` | a resolution above the point size (e.g. 3024x1964) renders into 1024x768, cropped |

## K1. Word-wrap forward progress

**Symptom.** A string with no space to break on, in a box too narrow for even two of its
characters, hangs the game: memory grows until the process is killed. Stack counts are bare
numbers in a narrow label, so enlarged fonts hit it. KMRP's "inventory crash" on Windows.

**Cause.** `CAurGUIStringInternal::WrapStrings` (`0x1001bc644`) backs its scan cursor up one
character when a line overflows, then checks it made progress. The check compares against the
start of the whole **string** (`[this+0x18]`) instead of the current **line** (`r15`), so once
the first line is done the check always passes, the line restarts where it began, and an empty
line entry is appended on every pass.

**Fix.** Replace hook at `0x1001bc7ec` (18 bytes). Progress is tested against the line start.
With no progress, the rest of the line is taken whole and handed to the engine's own
end-of-line path at `0x1001bca69`, which stores it and moves on. Unfittable text becomes one
overflowing line.

Two guards before the loop blanked strings of one or two characters when the box was narrower
than one or two `o` glyphs (`0x1001bc71e`: `jmp return` becomes `jmp loop`; `0x1001bc738`:
`jl return` is NOPed). They existed because such a box would otherwise hang. With the fix they
only hide text, so they go too. Never apply the guard edits without the replace hook.

**Verified.** In-process test: the engine's `WrapStrings` on `"a 12345"` at widths 20 and 30
did not return within 3 s without the fix (watchdog fired). With it, it returned 2 lines, and
normal hyphenation was unchanged.

```asm
    ; replaces 0x1001bc7ec..0x1001bc7fd; KPM jumps back to 0x1001bc7fe (the progress path)
    ; r15 = start of the current line, r8 = scan cursor, r13b = current char
    dec   r8                            ; original: back the cursor up one character
    mov   r12, qword ptr [rbp - 0x50]   ; original: r12 = this
    cmp   r8, r15                       ; FIX: progress against the LINE start
    ja    Lprogress                     ; normal hard break
    mov   r8, r15                       ; no progress: take the rest of the line whole
Lscan:
    movzx eax, byte ptr [r8]
    test  al, al
    je    Leol
    cmp   al, 0x0a
    je    Leol
    inc   r8
    jmp   Lscan
Leol:
    mov   r13b, al
    jmp   qword ptr [rip + Leol_target] ; engine's end-of-line path
Leol_target:
    .quad 0x1001bca69
Lprogress:
```

## K2. List rows stop growing

**Symptom.** Lists that are refilled in place (Powers and Feats on Windows, where it went 42 →
56 → 126px) get taller rows every time.

**Cause.** `CSWGuiListBox::OrganizeControls` (`0x1004a82b4`) spreads the box's leftover height
over the visible rows by making each row taller, and writes that height into the reused row
controls. The next pass takes the maximum row height as the list's item height (`+0x368`),
reads the inflated value back, and inflates it again.

**Fix.** The two stores of the inflated height into `[rbp-0x34]` are NOPed (`0x1004a8927`,
`0x1004a8939`); the rect keeps the item height stored before the loop. Row *positions* still
spread exactly as before (they advance by `ebx`, untouched), so lists lay out the same and simply
stop growing.

**Verified.** No change in any list at `FontScale` 1 or 2. With this patch's row heights the
growth did not reproduce on the Mac before the fix either; it is ported because the defect is in
the shared code path.

## K3. Leading newline trimmed

**Symptom.** Item and quest descriptions whose text opens with a property block begin with an
empty line (~16px at 800x600, very visible once text is enlarged).

**Cause.** The description builder prefixes `"\n"` to each property line, so the string itself
starts with a newline.

**Fix.** A detour in `CSWGuiTextParams::SetText` (`0x1004a3714`), the setter every GUI text
control goes through, right after its `CExoString` assignment (`0x1004a3726`), removes leading
newlines in place (`KMRP_TrimLeadingNewlines`). Trimming at set time keeps the line-breaker and the
renderer, two separate passes over the same string, in agreement about where lines start.

**Verified.** Screenshots with and without the hook: the blank first line is gone.

## K4. Video mode follows the widescreen target

**Symptom.** In fullscreen with the default INI, the 1512x982 layout is drawn into a fixed
1024x768 CGL surface and only its bottom-left corner is shown. (Unpatched Aspyr shows the same
crop on this Mac: its 1024x768 frame sits in the corner of a display-sized surface.)

**Cause.** The layout is built for `g_targetWidth` x `g_targetHeight` (the display's point
size, or `ForceWidth`/`ForceHeight`), but the engine opens its video mode from the INI
`Width`/`Height`, and in fullscreen that sizes a fixed surface (`kCGLCPSurfaceBackingSize`).

**Fix.** All paths of Aspyr's `ReadAndSetVideoMode` (`0x10026e864`: INI value, largest valid
mode, 800x600 fallback) join at `0x10026ed44` with `r12` → width and `r15` → height. A detour
there hands the engine the target resolution (`KMRP_UseTargetVideoMode`). K7's runtime part
runs from here too, because the resolution is final at that point.

**Verified.** Fullscreen with the default INI renders natively at 1512x982.

## K5. A line taller than its box is kept

**Symptom.** Stack counts disappear as soon as the font outgrows the count badge (`FontScale`,
HD font textures, font mods).

**Cause.** When a text's lines do not fit its box, `CAurGUIStringInternal::Draw`
(`0x1001bcb04`) drops lines from the top until the rest fits (bottom-aligned `0x20` and centred
`0x10` alignment). A single line taller than its box is therefore dropped entirely.

**Fix.** Never drop the last line. Four replace hooks, one entry and one loop site per
alignment. Text that partly fits behaves exactly as before; text that cannot fit even one line is
drawn overflowing instead of vanishing. `[r13+0x50]` = line count, `r12` = lines dropped so far.

| site | original | role |
| --- | --- | --- |
| `0x1001bcbef` | `ucomiss xmm1,xmm2; jbe +0x36` | bottom-aligned entry |
| `0x1001bcc22` | `inc r12; ucomiss xmm1,xmm2; ja -0x2f` | bottom-aligned drop loop |
| `0x1001bcc72` | `ucomiss xmm2,xmm1; jbe +0x3a` | centred entry |
| `0x1001bcca9` | `inc r12; ucomiss xmm2,xmm1; ja -0x2f` | centred drop loop |

```asm
; entry (bottom: xmm1,xmm2 -> keep 0x1001bcc2a; centred: xmm2,xmm1 -> keep 0x1001bccb1)
    ucomiss xmm1, xmm2
    jbe   Lkeep                 ; fits: as before
    cmp   dword ptr [r13 + 0x50], 1
    jle   Lkeep                 ; FIX: a single line is kept
    jmp   Lend                  ; several lines: the engine's drop loop (KPM jumps back)
Lkeep:
    jmp   qword ptr [rip + Lt]
Lt: .quad 0x1001bcc2a
Lend:

; loop (bottom -> 0x1001bcbfb; centred -> 0x1001bcc82)
    inc   r12
    ucomiss xmm1, xmm2
    jbe   Lend                  ; fits now: stop dropping
    mov   ecx, dword ptr [r13 + 0x50]
    dec   ecx
    cmp   r12d, ecx
    jge   Lend                  ; FIX: only the last line is left
    jmp   qword ptr [rip + Lt]  ; drop another
Lt: .quad 0x1001bcbfb
Lend:
```

**Verified.** Test build with 2x glyphs in an unenlarged badge: stack counts visible with K5,
gone without it. No difference at normal settings.

## K6. Wrapped lines measured with a 0.5px margin

**Cause.** The line-breaker truncates every glyph advance to whole pixels after adding 0.25,
so it under-measures a line by ~0.25px per character on average, while `Draw` advances by the
exact float widths. Long lines can end up wider than the box they were wrapped for.

**Fix.** KMRP's Windows fix uses a 0.5px margin (unbiased rounding). Here the one accumulation
in the loop (`addss xmm0, [rip+disp]` at `0x1001bca20`) is pointed at the engine's existing
`0.5f` (`0x100537dc4`) instead of `0.25f` (`0x10056eca0`). The 0.25 constant itself is shared by
42 other instructions and stays.

**Verified.** No regression in any description pane at `FontScale` 1 or 2.

## K7. Dialogue letterbox sized from the height

**Cause (bars).** Vanilla keeps a 21:9 band clear: each bar is `(H - W / (7/3)) / 2`, with the
7/3 a float at `0x100570a14` (stored as `2.333333f`, one ulp under 7/3). Derived from the
width, the bars shrink on wide screens and go **negative** once the screen is wider than 21:9
(3440x1440: -17px, 2560x1080: -8px), taking the subtitle and the replies with them. The
constant is read by exactly six sites:

| site | what it sizes |
| --- | --- |
| `CSWGuiDialogLetterbox::SetTop` `0x100244174` | top bar |
| `CSWGuiDialogLetterbox::SetBottom` `0x100244218` | bottom bar |
| `CSWGuiDialogTop` constructor `0x100244580` | NPC line label height |
| `CSWGuiDialogTop::SetReply` `0x10024483e` | NPC line label height (grows for long lines) |
| `CSWGuiDialogCinematic` constructor `0x10024497c` | reply panel top |
| `CSWGuiDialogCinematic::Reset` `0x100244ff0` | reply panel top |

**Cause (replies).** The reply panel under the bottom bar is a hard-coded 100px tall
(constructor `0x100244aa6` and `Reset` `0x10024505b`) whatever the bar's height, and
`LB_REPLIES` keeps the 98px its layout gives it. Measured at 1512x982, `FontScale` 2: panel
`{48,815,1416,100}`, list 98px, bar 167px. Two of three replies were visible, with a scrollbar
and 67px of empty bar.

**Fix.** KMRP's Windows formula, `bar = H / 6` (from J0-o's "Scaled Letterbox"). On the Mac
that needs one float: with `C = 1.5 * W / H`, `W / C = 2H/3`, so all six sites follow at once.
`KMRP_UseTargetVideoMode` (K4) calls `SizeDialogueLetterbox`, which:

1. writes `1.5 * W / H` over the constant;
2. rewrites both reply-panel immediates to `bar - 36` (the bottom safe margin; never below the
   vanilla 100). `Reset` computes its "restore the message label" delta from the same immediate
   (`sub r15d, [rbx+0x14]`), so that bookkeeping stays exact;
3. only writes where the vanilla bytes are still present.

A replace hook in `CSWGuiDialogCinematic::SetExtent` (`0x100244d7d`) then stretches
`LB_REPLIES` to the panel, never shrinking it:

```asm
    ; replaces 0x100244d7d (7 bytes); KPM jumps back to 0x100244d84, which passes the rect
    ; to CSWGuiListBox::SetExtent. rbx = panel, r14 -> LB_REPLIES rect (panel-local)
    mov   eax, dword ptr [rbx + 0x10]   ; original: panel width
    mov   dword ptr [r14 + 8], eax      ; original: list width = panel width
    mov   eax, dword ptr [rbx + 0x14]   ; panel height
    sub   eax, dword ptr [r14 + 4]      ; minus the list's top inside the panel
    cmp   eax, dword ptr [r14 + 0xc]
    jle   Lkeep                         ; never shorter than the layout's own height
    mov   dword ptr [r14 + 0xc], eax
Lkeep:
```

**Verified.** At 1512x982: bar 164px (was 167), panel `{48,818,1416,128}`, `LB_REPLIES` 128px.
At `FontScale` 2 all three replies of Bastila's conversation fit with no scrollbar. At
`FontScale` 1 the only change is the bar being 3px thinner. The ultrawide case is established
by the arithmetic (every site computes `(H - floor(H/1.5)) / 2` whatever the width), not in
game: `ForceWidth`/`ForceHeight` cannot produce a non-native surface on a Mac (see *Notes*).

**Rejected.** Porting the Windows fix site by site (nine inline patches and two trampolines).
One float write does the same on this build.

## K8. Minimap keeps the vanilla zoom

**Cause.** Every HUD template authors the radar (`LBL_MAPVIEW`) at 120x120 and the map texture
(`LBL_MAP`) at 512x512 (read from `mipc28x6`, `mipc210x7`, `mipc212x9`), and the minimap is
drawn in radar pixels. This patch enlarges the radar to `130 * HudScale` (195px at 1512x982)
while the map stays 512px, so the minimap showed 1.6x more area with everything smaller.
Measured against unpatched vanilla at the same spot. KMRP fixes the same thing on Windows
(`.kmz` for the map, `.kfg` for the fog). With `UseGuiFileLayouts` the radar's size comes from
the HUD file instead (270px at 3440x1440 in KMRP's), and K8 works the same way.

The draw (`0x100237848`) opens a GL viewport of the radar's size (`AurGUISetupViewport`, which
also pushes `{x, y, w, h}` on the viewport stack at `0x1005f4b40`), centres `LBL_MAP` on the
player (`left = radar/2 - x * scale`), draws it, runs the fog pass (`0x100238684`), then draws
the arrow. Quads are normalised to the viewport by dividing by the stack entry's `w`/`h`
(`0x1001be62e`, `0x1001be6b6`); the fog pass divides by the radar size at `hud+0x7b10/0x7b14`
instead, over its own 440x256 basis.

**Fix.** Replay the vanilla 120px radar inside the enlarged viewport. Three detours:

| site | cut | does |
| --- | --- | --- |
| `0x100237974` | `mov r13,[rbp-0x40]; mov rdi,r13` | place `LBL_MAP` for a 120px radar (its size stays 512) |
| `0x100237a2f` | `lea r14,[rbx+0x7970]` | viewport open: stack entry and radar size read 120 |
| `0x100237a4f` | `mov rdi,r14; movss xmm0,[rbp-0x2c]` | after the fog, before the arrow: restore both |

Map and fog then normalise against 120 and land enlarged by `radar / 120`. The arrow keeps its
own geometry. With a 120px radar nothing changes.

**Verified.** Side by side at the Manaan spawn point: vanilla, this patch without K8, and with
K8. With K8 the map and the fog grid match vanilla's framing.

**Rejected.** Enlarging `LBL_MAP`'s extent instead. The label maps its texture by extent, so
the map tiled rather than zoomed: a different part of the area showed under the arrow.

## K9. Retina display modes

**Cause.** A resolution is only usable if it is in Aspyr's display-mode list: the engine looks
the requested size up there (`0x100204d5a`) and falls back to the default mode when it is
missing. The list is built once at startup (`0x10001ddee`) from SDL's modes, which are in
points. That function was written to give every mode a twin scaled by the display's backing
factor, flagged as a HiDPI mode, but the factor is the constant 1.0 (`movabs rax, 1.0` at
`0x10001de62`), so the twins are never added. On a Retina display the pixel resolution
(3024x1964 on a 14" MacBook Pro) was therefore not a valid mode. With
`ForceWidth=3024 ForceHeight=1964`, K4 asked for it, the lookup failed, and the engine laid out
3024x1964 but rendered into a 1024x768 surface: cropped, the bottom-left corner only.

**Fix.** A detour on the store of that constant (`0x10001de6c`, `mov [rbp-0xa0], rax`) returns
the display's real pixel/point ratio in `rax` (`exclude_from_restore = ["rax"]`), and the cut
stores it in Aspyr's variable. It does so only when the target is larger than the display's
point size; otherwise it returns 1.0 and the list stays vanilla. Everything downstream is
Aspyr's own code:

- the surface is sized from the mode, so it is 3024x1964;
- the mouse arrives from SDL in window points, and the port converts it to render pixels with
  the render/window ratio it already keeps for every mode (`0x100025802`: `(x - offset) *
  scale`). The cursor therefore tracks at 2x without a mouse fix of its own. The same
  function has the reverse direction (`x / scale + offset`, render to window). Vanilla already
  depends on this conversion at 1024x768 in a 1512x982 window, a ratio of 0.68. Not traced: a
  cursor warp at 3024x1964; no warp came up in testing.

The resolution is asked for with the existing `ForceWidth=3024 ForceHeight=1964`.
`InitTargetResolution` also records the display's size in points and in pixels
(`CGDisplayModeGetPixelWidth/Height`), which the detour compares the target against.

**Verified.** At 3024x1964, fullscreen and windowed, with the target at the pixel size:

- surface, viewport and screenshots are all 3024x1964;
- a click at Load Game's position on screen opens Load Game;
- HUD, inventory, map, options and a conversation are laid out as at 1512x982.

With the target at the point size the frame is 1512x982 with no fixed surface, as before.
The in-game checks ran with the target set by a `NativeResolution` key since removed;
`ForceWidth`/`ForceHeight` reach the same test (target above the point size). Re-run with
`ForceWidth=3024 ForceHeight=1964` after the removal (2026-09-29, fullscreen, with
`UseGuiFileLayouts=1` and KMRP's 3024x1964 set): surface 3024x1964, backing scale 2.00, and
the clicks through the main menu, Load Game and the Abilities tabs landed.

Frame times on an M5, in game at a Manaan dock:

| resolution | Anti Aliasing=6 | Anti Aliasing=2 | Anti Aliasing=0 |
| --- | --- | --- | --- |
| 1512x982 | 7.5 ms | | |
| 3024x1964 | 32.5 ms | 8.0 ms | 8.2 ms |

Native with 6x AA costs about 4x; at 2x or none it is as fast as 1512x982 was with 6x.

## Menus already laid out for the resolution

`UseGuiFileLayouts=1` in `[Graphics Options]` is for `.gui` sets already laid out at the
screen's resolution in Override: KOTOR High Resolution Menus, and KMRP, which builds on it and
derives sets for the Mac displays. Off (the default) nothing changes. On, the patch still
unlocks the resolution (the engine's UI size, viewport, GUI canvas and tooltip bounds, the
video mode, K9) and keeps every engine fix, but lays nothing out itself:

| | Off (default) | On |
| --- | --- | --- |
| Recentring constants (20 sites) | `-(H * 4/3)`, `-H`: a centred 4:3 canvas | `-W`, `-H`: the full screen, as the High Resolution Menus executable patch sets them |
| HUD file (`CSWGuiMainInterface` constructor, 5 `lea` sites) | `mipc212x9` at every height | `mipc28x6`, or `mipc210x7` at 3440x1440, as KMRP's Windows executable loads |
| Menu, popup and HUD scaling (`Hook_WindowDraw`, `Hook_MainInterfaceDraw`) | runs | returns after `updateEngineGlobals` |
| Class-selection screen | patch's own loop (`0x100337897` bypassed) | the vanilla loop (bytes restored) |
| Stack-count badge in store and workbench rows (`0x1002bfbbf`, `0x10021c0e2`) | 14px left of the icon's right edge | at the right edge, as in vanilla (bytes restored) |
| Font scaling (`scaleLoadedTextureMetadata`) | runs | off: fonts come at their size from their TXI metrics |

The five `lea` sites are the only references to the HUD file names in the binary (scanned for
every RIP-relative `lea` to them). What the switch leaves undone is the business of whatever
supplies the `.gui` files: KMRP, for instance, pairs it with a Mac patch of its own that writes
the per-resolution sizes its Windows installer writes (list rows, popups, the area map).

**Tested in game** with KMRP's sets, 2026-09-29: at 3024x1964 fullscreen (with K9) and
1512x982 both fullscreen and windowed, and at 1352x878 windowed (a size KMRP blends at
install): main menu, Load Game, HUD and minimap, inventory, abilities, messages, journal,
area map (with clicks on its notes), options, a confirmation popup and a conversation; every
click landed. With the setting off, none of this code runs: the patch lays out the menus
itself, as it always has, with the engine fixes above.

## Entry points for a patch that brings its own menus

Added 2026-10-04, so that KMRP for macOS can be a patch of its own that `requires` this one
instead of carrying a copy of it. Nothing changes for the patch used alone.

| Entry point | What it does |
| --- | --- |
| `K1Widescreen_UseGuiFileLayouts(1)` | the same as `UseGuiFileLayouts=1` in `swkotor.ini`, asked for from code. To be called from the other patch's constructor: it has to come before the engine first sets its video mode. Returns 1 when the mode is, or will be, the one asked for |
| `K1Widescreen_SetTargetResolution(w, h)` | the resolution the layout is for, changed while the game runs. Everything in this patch that depends on the size is applied again: the HUD template, the recentring constants, the list, map, popup and checkbox sizes, the dialogue letterbox (K7) |
| `K1Widescreen_GetTargetResolution(&w, &h)` | the resolution in force |

KotorPatcher loads each patch's module privately, so the other patch finds these in this
patch's own module (`dlopen` of `patches/k1widescreenpatch.dylib` beside its own, with
`RTLD_NOLOAD`, then `dlsym`); `dlsym(RTLD_DEFAULT, ...)` does not find them.

What had to change for them:

- **The layout mode is written at the first video mode, not at load.** `DylibInit` only reads
  the resolution now. What it used to write (the HUD template, the class-selection loop, the
  stack badge, the recentring and list constants, the `.gui` mode's stubs) is written by
  `ApplyLayoutMode`, from the video-mode detour (K4, `KMRP_UseTargetVideoMode`), which the
  engine reaches before any panel exists. KotOR Patch Manager loads a patch that requires this
  one after it, so a mode written at load could not be chosen by that patch, and the patch's
  own layout cannot be taken back once written (its list constants keep no record of what they
  replaced).
- **K7's letterbox can be sized again.** It remembered only that it had run; it now replaces
  what it last wrote.
- **K9 gives the display's pixel/point ratio always**, not only for a start above the point
  size. Aspyr's mode list is built once, so a game started at 1512x982 had no 3024x1964 to
  switch to. On a display with as many points as pixels the ratio is 1.0 and the list is
  vanilla, as before.
- **The 11 engine-fix hooks this patch shared with the Stray Bug Fixes patch are declared
  there only.** With both built from master, KotOR Patch Manager refused them together
  ("Hook conflicts detected: Address 0x1001BC7EC used by multiple patches", 2026-10-04), and
  this patch requires that one.

Measured 2026-10-04 on a 14" MacBook Pro (1512x982 points, 3024x1964 pixels), Steam build,
fullscreen:

| Check | Result |
| --- | --- |
| This patch with the Stray Bug Fixes, its own layout, the Options screen: before these changes against after | 211 of 1,484,784 pixels differ by more than 8 of 255, at most by 10 |
| With a KMRP patch on top that requires this one and calls `K1Widescreen_UseGuiFileLayouts(1)`, no `UseGuiFileLayouts` in `swkotor.ini` | KMRP's menus at 1512x982 |
| Options, Graphics, Screen Resolution, 1512x982 to 3024x1964, that KMRP calling `K1Widescreen_SetTargetResolution` | viewport and surface 3024x1964; the Graphics screen, Options and the main menu laid out for it |

Not tested: `K1Widescreen_SetTargetResolution` with the patch's own layout (no `.gui` sets),
where the fonts are scaled once when they load; a switch while a game is loaded.

## Changes to the widescreen code

Besides the setting above, five changes inside `mac_widescreen.cpp`, each commented in place:

- **Display size in points and pixels** (for K9). `InitTargetResolution` also records the
  display's size in both, and is no longer `static`, because K9 needs the target before
  anything else asks for it. It reads the INI before detecting the display instead of after.

- **Every window drawn once per frame, not twice.** This fixes the "too-fast animation on
  the menu screen".
  - `Hook_WindowDraw` and `Hook_MainInterfaceDraw` ended by calling the original draw through
    a hand-built trampoline (`Original_WindowDraw`, `Original_MainInterfaceDraw`). That is
    right for a hook that replaces a function, but these are KPM DETOURs
    (`skipOriginalBytes = false`): once the hook returns, KPM's wrapper runs the stolen
    prologue and continues into the original anyway. So every GUI window and the HUD were
    drawn twice per frame, and every draw advanced its animations by `delta`.
  - Measured at the main menu, fullscreen at 1512x982: with the patch, the scene rendered at
    51 fps against 120 fps without it. It moved in double steps, and 10 of 19 screenshots
    0.1 s apart were identical to the one before, which reads as about 10 fps.
  - Both calls are removed; the draw now runs once, after the hook's adjustments, as before.
    Measured after the change: 99–103 fps, 3 of 19 identical.
  - Checked in play: HUD, inventory, abilities, messages, journal, map, options, a
    conversation, and the target nameplate both on-screen (above the NPC) and off-screen
    (pinned to the edge with its arrow).
  - *Rejected first:* skipping redundant runtime code writes (`refreshPatchedListConstants`
    rewrites about 45 code sites per panel per frame, which is costly under Rosetta). It
    changed neither measurement, so it was not kept.

- **Stack badge grows with the font** (`StackBadgeHeight`). The quantity badge was
  `18 * scale` whatever the font, so with `FontScale` > 1 the digits outgrew it and every stack
  count disappeared (measured at `FontScale` 2). The badge now also grows by the font scale; at
  1x nothing changes. Same defect KMRP fixes on Windows by sizing the stack label with the font.
- **Font scaling cache keyed on the height too.** The engine frees font textures and allocates
  new ones at recycled addresses. Keyed by address alone, a freshly parsed font at a reused
  address counted as "already scaled" and stayed at 1x. Which fonts were hit varied with heap
  layout from launch to launch. An entry now matches only if the font still holds the height
  it was left with; a fresh parse carries its unscaled TXI height.
- **No writes past the letterbox object.** The dialogue block in `Hook_WindowDraw` treated
  `window + 0x20c0` as `LB_REPLIES` for four vtables. For `0x1005a6b98`, which is
  `CSWGuiDialogLetterbox` (a ~0xc8-byte object, constructor `0x100243fc0`), the two writes landed
  about 9KB past its end on every dialogue frame. Only `CSWGuiDialog` (`0x1005a6a70`) and its
  subclasses `Cinematic` (`0x1005a6c88`) and `Computer` (`0x1005a6db0`) have that list.

Hooks were also reordered: all byte and replace hooks first, all detours last. KotorPatcher
applies hooks in file order and stops at the first `original_bytes` mismatch. The first detour
loads the dylib, whose constructor rewrites byte hooks with resolution-dependent values, so a
byte hook listed after a detour fails verification. In the shipped order only 14 of 40 hooks
were applied.

## Every KMRP Windows fix, checked against this build

Source: KMRP's `reverse-engineering/binary-inventory.md`, which lists every byte the Windows
patch changes (68 code and data runs, 11 added sections, 12 runs written at install). The
right column is for this patch's own layout (`UseGuiFileLayouts` off). Rows marked † exist on
Windows to fit KMRP's own `.gui` sets; with `UseGuiFileLayouts` on, KMRP's Mac patch carries
them, not this one.

| Windows fix | Mac |
| --- | --- |
| Resolution unlock, per-resolution constants, resource selector (`0x403D6C`, `0x40AA65`, `0x5F0C65`, `0x68C4E3`, `0x68C4F4`, …) | covered by the widescreen patch (runtime layout, HUD template pinning) and K4 |
| Font scale hooks (`.kfs`: `TextOutA`, `Draw`) | covered: `scaleLoadedTextureMetadata` (with the cache fix above) |
| Text-list row height grows with the font (`0x417992`, `CSWGuiButton::Initialize`) † | not needed: rows scale with the layout (H/480) and outgrow the automatic font scale (H/1080) at every size |
| Dialogue letterbox (`.klb`, 9 sites) | **K7** |
| Word-wrap progress (`.kwl` `0x45A5E0`) and short-string guards (`0x45A3B7`, `0x45A3DC`) | **K1** |
| Wrap margin | **K6** |
| Stack-count label with the font (`.ksc` `0x6B5336`) | badge geometry covered by the widescreen patch, growth by `StackBadgeHeight`, vanishing by **K5** |
| Listbox row growth (`0x41B507`, `0x41B52C`) | **K2** |
| Horizontal-only listbox padding (v11), gutter follows the scrollbar (`.kgs` v12), fit test † | not needed: these make KMRP's own `.gui` gutters work. This patch uses vanilla `PADDING` as row spacing on purpose; Feedback Options circles clear the scrollbar |
| Leading newline (`.ktn` `0x415E0D`) | **K3** |
| Minimap zoom and fog grid (`.kmz`, `.kfg`, v14) | **K8** |
| Minimap constructor wrap (`0x62B39C`) | not needed: undoes KMRP's own full-map constructor defaults |
| Message popup caps and icon rect (v15) † | not needed: the Mac popup starts narrower than the 440 cap, so auto-fit runs (message grew 20 → 116px at `FontScale` 2); the icon size is KMRP's own `confirm.gui` layout |
| Area map markers, fog grid, hit test, surface (v18–v20, `.kui`) † | covered by the widescreen patch |
| Map-note corrections (`.kmn`, v21) | a content mod, not an engine fix; shipped by KMRP separately |
| Large Address Aware (v22) | not applicable: the Mac build is 64-bit |
| Movie display modes and aspect (v23, v24, `.kmv`) | not applicable: Aspyr's Bink 2 player pillarboxes and switches no mode (checked in game) |
| Texture residency, driver compatibility, DPI and NVIDIA settings, controller layer | not applicable: Windows or Direct3D specific; the Aspyr port has native controller support |

## Notes for the widescreen patch (found while testing, not changed)

- **`ForceWidth`/`ForceHeight` must be a real display mode.** At 1512x630 fullscreen the
  surface stayed 1512x982 with the frame in its bottom 630px; windowed, the window opened at
  1024x768. With K9 the display's pixel size counts as a mode too.
- **Windowed mode** renders the display-sized frame into a content area 61px shorter (title and
  menu bar), so it is squashed about 6% vertically. Clicks still land correctly.
- **`FontScale` well above the automatic value** (tested at 2 on a 982-point display) overflows
  fixed-height boxes, identically with and without these fixes: 8-digit credits show only their
  last two digits, long journal quest names and Feedback options wrap in one-line rows, and
  message-box buttons stay 22px tall while their labels double. At the automatic scale none of
  this occurs.
