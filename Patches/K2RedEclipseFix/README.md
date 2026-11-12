# K2RedEclipseFix

Fixes the empty Ebon Hawk after the Red Eclipse cutscene on Nar Shaddaa, and the lost party restore in 004EBO.

Supported build: Steam (Aspyr), `kotor2_steam_aspyr`. Seen under Proton on Linux.

## Cause

A third-party Ebon Hawk mod causes this. AxConsortium's *TSL Ebon Hawk K1 Camera & Unlocked Map* (DeadlyStream file 2864) installs `Override/003ebo.are`, the Ebon Hawk area file, to change the Hawk's camera and map.

Three modules name their area `003ebo`: the Hawk itself (003EBO), the Hawk after the Red Eclipse cutscene (012EBO), and the Hawk fight (004EBO). The game loads an Override file before a module's own files, so the Override area also takes over 012EBO and 004EBO. Those two modules then run the Hawk's scripts instead of their own:

- 012EBO runs `k_003ebo_enter` instead of `k_012_enter`. `k_012_enter` spawns the companions and starts the Ratrin conversation. Without it the Hawk is empty and the party is stuck.
- 004EBO runs `k_ebo_user` instead of `k_004_heart`. `k_004_heart` restores the party and returns to 301NAR.

Without the third-party Override file, the modules use their own areas and nothing here changes.

## Fix

One detour at `CVirtualMachine::RunScript` (Steam `0x006FD8D6`, 6 bytes after the function entry `0x006FD8D0`). It runs for every script:

- When the script is `k_003ebo_enter` in module `012ebo`, it runs `k_012_enter` instead.
- When the script is `k_ebo_user` in module `004ebo`, it runs `k_004_heart` instead.

The hook writes the replacement name into the script argument at `[EBP+8]`, which `RunScript` reads next. All other scripts pass through unchanged. 003EBO is never changed, so the Hawk keeps its Override area and the camera and map changes still apply.

The active module name comes from the game object chain: `0x00A1B4A4` → `+4` → `+4` → `+0x18` → module name at `+0x38`.

Hook site bytes: `89 4D F8 83 7D 08 00` (`MOV [EBP-8],ECX`, `CMP DWORD [EBP+8],0`).
Hook parameter: `ebp` (frame pointer).

## Configuration

None. Uninstall the patch to turn it off. It writes no log.

## Testing

- Steam build under Proton, with the third-party Override installed: played the Red Eclipse landing-pad cutscene on Nar Shaddaa, went to the Ebon Hawk and returned without a softlock. The log shows the `012ebo` swap firing.
- The tested build had the same hook and redirect with logging added. This package has the logging removed.
- 004EBO branch (`k_ebo_user` to `k_004_heart`): not yet tested in game.

## Known limits

- The third-party mod's heartbeat (`a_showmap`) still runs in the shadowed 012EBO and 004EBO areas. It only shows the map, so it is left alone.
- An earlier one-off freeze during the dock cutscene was seen once and not reproduced. It is not addressed here.
