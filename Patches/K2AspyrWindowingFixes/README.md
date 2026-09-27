# K2 Aspyr Windowing Fixes

Fullscreen and windowed-mode fixes for Aspyr's native Linux KOTOR II.

## Why

Aspyr did not port the engine's platform code, they emulated Win32 under it: a D3D-over-OpenGL
layer, a Win32 API shim, and a statically linked SDL 2.0.3. Every fix here has the same shape.
The engine asks the shim for something reasonable, the shim answers with something else, and the
fix supplies the answer the engine expected.

Several of these only show up away from a 16:9 display, which is the case the port was exercised
against, so an ultrawide or a 16:10 panel meets all of them at once.

## What it fixes

**Fullscreen stretched the picture.** The renderer presents by blitting its render target onto the
window with `glBlitFramebuffer`, and in fullscreen it used the desktop size as the destination with
no aspect-ratio check. A 2560x1440 mode on a 5120x1440 desktop came out twice as wide as it should.
The destination is now the largest centred rect of the source's aspect ratio that fits, and the
cursor mapping follows it, through a letterbox offset the shim already had and never set.
See `FullscreenAspectFix.cpp`.

**The resolution list offered no choice about that.** The list was written to show a `(stretched)`
row beside a plain one, but the shim enumerates each mode once and reports `DMDFO_DEFAULT`, so the
second row never existed. Modes that cannot fill the screen now get both rows, and picking one is
remembered. See `StretchedResolutionRows.cpp`.

**Windows appeared at the wrong size, then resized.** Every window is created with `CW_USEDEFAULT`,
which the shim answers with the desktop resolution rather than something sensible, and always
decorated because `dwStyle` never reaches SDL. Since the renderer is torn down and rebuilt several
times during startup, that flash repeated. Windows now arrive at the size and border they are going
to keep. See `WindowGeometryFix.cpp`.

**Alt-tab did not minimise.** The engine asks for it, with `ShowWindow(hwnd, SW_MINIMIZE)`, but the
shim's `ShowWindow` only ever raised a window and did not act on that command at all.
See `MinimiseFix.cpp`.

**Alt+Enter did nothing.** The engine's fullscreen toggle is complete and was unreachable:
nothing calls it, because the shim's SDL translation never produces a `WM_SYSKEYDOWN` for a Win32
system key combination. The keystroke is now taken from the shim's event pump, the engine's own
toggle decides which way to go, and the result is written back to the ini so it survives a restart.
See `AltEnterFix.cpp`.

**Startup rebuilt the renderer for a mode already in use.** Applying a video mode legitimately
rebuilds the window, because a Win32 pixel format cannot be changed once set. Startup did it three
times with identical arguments, and one of those was an unconditional duplicate. The other two are
load bearing and are left alone. See `DuplicateModeApplyFix.cpp`.

## Settings

Nothing has to be configured. Two existing keys in `swkotor2.ini` under `[Graphics Options]` are
honoured that the stock game writes but never reads:

- `Stretched=1` keeps the old stretched image instead of the aspect-correct one. Picking a
  `(stretched)` row in the resolution list sets it.
- `FullScreen` is now written when Alt+Enter changes it, so the toggle persists.

## Layout

`WindowingShim.h` and `WindowingShim.cpp` hold what the fixes share: reading and writing the game's
ini through its own `CExoIni`, and the two questions they ask of it. Each fix is otherwise
self-contained in its own file, with the reasoning at the top.
