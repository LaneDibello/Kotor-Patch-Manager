# K2ActivationFix

Fixes an intermittent hang when loading into KOTOR II, where the load screen stays up forever.

Supported build: Steam (Aspyr), `kotor2_steam_aspyr`. Seen under Proton on Linux.

## Cause

At the end of a movie (observed after the intro movie) the movie thread posts a synthetic
`WM_ACTIVATEAPP(wParam 0, lParam 0)` to the main window:

```
0x0040D692  push 0
0x0040D694  push 0
0x0040D696  push 0x1C                  ; WM_ACTIVATEAPP
0x0040D698  mov  eax, [0x00A1B484]     ; main window
0x0040D69D  push eax
0x0040D69E  call dword ptr [0x0098646C] ; PostMessageA
```

Under Wine the movie window only saw activation move to the game's own main window, so this "the
game was deactivated" is false. The window procedure marks the engine inactive and minimizes it.
When the window stays in front, no `WM_ACTIVATEAPP(TRUE)` ever arrives, so the main loop never
hides the load screen and the game stays stuck there. The process is alive, the UI thread still
pumps messages, and the load never finishes.

## Fix

One detour at `0x0040D69E`, run just before that `CALL [PostMessageA]`. It receives a pointer to the
four values the game pushed (`hwnd`, `msg`, `wParam`, `lParam`). When the message is
`WM_ACTIVATEAPP(0, 0)` and the foreground window belongs to this process, it rewrites the call to
`PostMessageA(NULL, WM_NULL, 0, 0)`, which does nothing, and the stolen `CALL` then runs as usual.
Anything else passes through unchanged, so a real alt-tab during the movie still works. The window
procedure and Alt+Enter handling are not changed.

Hook parameter: `esp+0`, the address of the pushed argument block.

## Configuration

None. Uninstall the patch to turn it off. It writes no log.
