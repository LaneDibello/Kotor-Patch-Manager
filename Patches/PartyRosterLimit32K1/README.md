# Party Roster Limit 32 (KotOR I)

KotOR I ships with nine party slots, 0 to 8, one for each companion. This patch raises the roster to 32, so mods can add companions in slots 9 to 31 without taking a vanilla companion's place.

- **The party selection screen** shows the larger roster in pages of 16, with buttons to turn the page.
- **Saves** keep every companion in a new slot the same way the game keeps its own, as `AVAILNPC<slot>` creature files inside the save.
- **Scripts** can use slot numbers 9 to 31 in every function that takes a party slot (`AddAvailableNPCByTemplate`, `AddAvailableNPCByObject`, `AddPartyMember`, `IsNPCPartyMember`, `IsAvailableCreature`, `SetNPCSelectability`, `RemoveAvailableNPC` and the rest).

---

## Installing

1. Apply **Party Roster Limit 32 (K1)** in Kotor Patch Manager.
2. Put `partyselection.gui` (in "PartyRosterLimit32K1 additional files") in the game's `Override` folder. The patch needs it: the vanilla screen has controls for nine companions only. The K1 attendant in "The KPM Mods" installs it for you.

Without the `.gui` file the party selection screen can't show the new slots.

---

## Supported Game Versions

| Platform / Release | Executable SHA-256 | Hooks File |
| :--- | :--- | :--- |
| **Windows GOG 1.03** | `9C10E0450A6EECA417E036E3CDE7474FED1F0A92AAB018446D156944DEA91435` | `kotor1.hooks.toml` |
| **Windows Steam 1.03** | `34E6D971C034222A417995D8E1E8FDD9F8781795C9C289BD86C499A439F34C88` | `kotor1.hooks.toml` |
| **Windows Editable 1.03** | `761F9466F456A83909036BAEBB5C43167D722387BE66E54617BA20A8C49E9886` | `kotor1.hooks.toml` |

---

## For modders: adding a companion in slots 9 to 31

A slot past 8 works like a vanilla one: add your companion to it, then use that number wherever the game wants a party slot. The question is which number. If every companion mod picks its own fixed slot, two of them will sooner or later pick the same one, and the second overwrites the first one's companion. And a player who hasn't applied this patch has no slots past 8 at all.

**Party Slots** is a small script include that settles both, in "PartyRosterLimit32K1 additional files/party-slots". A mod asks for a slot instead of choosing one: it gets the first slot from 9 up that no companion holds and no other mod has reserved, keeps it for the rest of the game, and gets a clear "no" on a game without this patch so it can fall back to what it did before.

| File | What it is |
| :--- | :--- |
| `ps_slots.nss` | The include. Copy it beside your scripts and `#include "ps_slots"`. |
| `ps_example.nss` | A recruit script that uses it, with a fallback. |
| `changes_globalcat.ini` | The HoloPatcher (TSLPatcher) lines for the globals it needs. |
| `LICENSE.txt` | MIT. Ship it with your mod as you like. |

### Three steps

1. **Pick a global name for your companion's slot,** for example `MYMOD_PARTY_SLOT`, and add it to `globalcat.2da` as a Number, together with the four shared globals `PS_SLOTS_A` to `PS_SLOTS_D`. `changes_globalcat.ini` has the lines. The shared rows use `ExclusiveColumn`, so they're added only if missing, and any number of mods can ship them.
2. **Add your companion through Party Slots:**

```c
#include "ps_slots"

void main()
{
    int nSlot = PS_AddByTemplate("MYMOD_PARTY_SLOT", "my_companion");
    if (nSlot == -1)
    {
        // no roster patch, or every slot is taken: your mod's own fallback
        return;
    }
    SetNPCSelectability(nSlot, TRUE);
}
```

3. **Use the slot wherever the game wants a party slot number.** Later scripts get it back with `PS_SlotOf("MYMOD_PARTY_SLOT")`.

### The functions

| Function | What it does |
| :--- | :--- |
| `PS_AddByTemplate(sGlobal, sTemplate)` | Puts the companion in your slot from a template, claiming the first free slot if you have none yet. Returns the slot, or -1 with nothing changed. |
| `PS_AddByObject(sGlobal, oCreature)` | The same with an existing creature, keeping its levels, gear and choices. |
| `PS_SlotOf(sGlobal)` | Your slot, or -1. |
| `PS_Claim(sGlobal)` | Reserves a slot without adding anyone. A reservation only: it doesn't check for the roster patch. |
| `PS_Release(sGlobal)` | Gives your slot back, for a companion who has left the game for good. |
| `PS_FindFree()` | The first free slot, or -1. |
| `PS_IsClaimed(nSlot)` | Whether any mod has reserved a slot. |
| `PS_SetClaimed(nSlot, bClaimed)` | Marks a slot taken or free by hand (see below). |

### Converting a mod that shares a vanilla slot

A mod that puts its companion in T3-M4's slot 7, for example:

```c
// before
AddAvailableNPCByTemplate(7, "p_mycompanion");

// after: its own slot when the roster patch is there, slot 7 as before when it isn't
int nSlot = PS_AddByTemplate("MYMOD_PARTY_SLOT", "p_mycompanion");
if (nSlot == -1)
{
    nSlot = 7;
    AddAvailableNPCByTemplate(nSlot, "p_mycompanion");
}
```

Every later reference to slot 7 then asks `PS_SlotOf("MYMOD_PARTY_SLOT")` first and uses 7 only if it returns -1. Games already under way keep working: until the recruit script runs again, the companion stays where it was.

A companion that already lives in a fixed slot past 8 on players' games can be recorded the first time your scripts run, so other mods see the slot taken:

```c
if (PS_SlotOf("MYMOD_PARTY_SLOT") == -1 && IsAvailableCreature(10))
{
    SetGlobalNumber("MYMOD_PARTY_SLOT", 10);
    PS_SetClaimed(10, TRUE);
}
```

### How it keeps mods apart

Each mod keeps its slot in its own global. The four shared globals hold one bit per slot from 9 to 31, seven slots to each (KotOR's global numbers are signed bytes, so the top bit is left alone), marking every slot a mod has claimed. A slot stays reserved while its companion is out of the roster for a while, so another mod can't take it in the meantime. A slot that already holds a companion is never handed out, even to a mod that doesn't use Party Slots.

### Without this patch

The game itself refuses slots past 8: they read as not available, and adding a companion to one fails. So `PS_AddByTemplate` and `PS_AddByObject` return -1 and change nothing, and your fallback runs.

### Notes

- The KotOR I script compiler has no `const`, so the include uses plain variables.
- Keep the global names exactly as they are. `PS_SLOTS_A` to `PS_SLOTS_D` and every function name will keep working in later versions.
- Party Slots is also kept as its own repository: [jryantomes/kotor-party-slots](https://github.com/jryantomes/kotor-party-slots).

---

## Known gaps

- **Scripted party clears** in the vanilla game (boarding the Ebon Hawk, the Leviathan capture, the Taris dueling ring, the Manaan arrest, the Star Forge) remove slots 0 to 8 only, so a companion in a higher slot stays in the party where the player should be alone. [#330](https://github.com/LaneDibello/Kotor-Patch-Manager/issues/330)
- **Party selection silhouettes** for the new slots. [#331](https://github.com/LaneDibello/Kotor-Patch-Manager/issues/331)
- **A companion removed from the party** in a slot past 8 can stay standing in the area. [#336](https://github.com/LaneDibello/Kotor-Patch-Manager/issues/336)

---

## Tested (Party Slots)

KotOR I 1.03 with this patch, run unattended by a test harness that loads a save, sets the game state and reads the results back from the game's own saves:

- With one companion already in slot 10, a second mod's companion added by template got slot 9, a third's added by object got 11, and a fourth's claim was given back cleanly. With every slot marked taken, a claim returned -1 and changed nothing.
- After the save was loaded again, every claim was still there, the next free slot was 12, and the second mod got its own slot 9 back instead of a new one.
- The party screen shows companions in slots 9, 10 and 11 with their portraits.
- Without the roster patch nothing changed, and the companion stayed in its vanilla slot.
- Two HoloPatcher mods installed one after the other gave one row of each shared global and one of each mod's own.
