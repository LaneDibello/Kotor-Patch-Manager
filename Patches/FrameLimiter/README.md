# Frame Limiter

Caps the frame rate by sleeping through the spare time in each frame, so a capped game
leaves the CPU alone.

## Why

KOTOR's only frame cap is V-Sync, and V-Sync leaves the waiting to the graphics driver.
Current GL drivers spin inside `SwapBuffers` rather than blocking on the vblank, so the
frame rate is held down while a core stays pinned at 100%.

The game already has a better cap. `WinMain` holds a spin at `0x00404990` that waits
until the frame has taken `1000/target` ms, with a `Sleep` in front of it. Neither has
ever run: the target at `0x007A3C64` and the gate at `0x007A3C58` are globals that no
instruction in the binary writes. Arming them shows the spin, the deadline and the
frame-start bookkeeping all work.

The one broken part is that the `Sleep` takes a constant, and a fixed nap cannot cover a
variable wait. Too small and the spin burns the rest. Too large and a slow frame
oversleeps past its deadline, which the spin can never undo, because it can only ever
wait longer. That is very likely why it shipped switched off.

The console's `framerate` command is a different thing and not a cap: it sets a fixed
simulation timestep and never waits.

## What it does

Arms the game's limiter and, once per frame, writes the nap that frame actually needs:
the time left until the deadline, less one millisecond for their spin to land on. So the
`Sleep` does the bulk of the wait and their spin finishes it exactly.

It also raises the timer resolution to 1ms, without which `Sleep` rounds up to about
15.6ms. The game never does this itself, which would have made their `Sleep(1)` a whole
frame on its own.

The deadline, the spin and the frame-start bookkeeping are all the game's. This only
supplies the number that was wrong.

## Settings

The patch writes `FrameLimiter.ini` beside the game on first run:

```ini
[Frame Limiter]
Limit=60
```

Edit and restart. Values outside 20 to 300 turn the limiter off, so `Limit=0` means off.

Pick the limit for input feel, not just heat. The game samples input once per rendered
frame, so the limit is also the input sampling rate: 16.7ms at 60 against a few
milliseconds uncapped. Matching the display's refresh rate is the usual answer. That is
the cost of any cap rather than anything this one does.

## Supported versions

KOTOR 1 1.0.3: GOG, Steam and CD. All three share one address database, so the globals
and the timer resolve the same way on each. The hook site was read off the GOG build;
the patcher checks those bytes before writing anything, so a build where they differ is
refused rather than corrupted.

The addresses of the game's limiter globals come from the version database, so a new
build needs them recorded there as well as the hook site.
