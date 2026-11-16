// ps_slots.nss: Party Slots, one shared way for KotOR I companion mods to take a party slot.
// Version 1.0.0 (2026-10-10). MIT License (see LICENSE). https://github.com/jryantomes/kotor-party-slots
//
// With Kotor Patch Manager's Party Roster Limit 32 the game has party slots 9 to 31 as well as the
// nine it ships with. A mod that adds a companion should take the first one nobody else holds, not
// a fixed number: two mods that both pick slot 10 overwrite each other's companion. And without the
// roster patch there are no slots past 8 at all, so a mod needs a way to know and fall back.
//
// Every mod that includes this keeps the same small record, so they never collide:
//   - its own global number (any name, e.g. "MYMOD_PARTY_SLOT") holds the slot its companion has;
//   - four shared global numbers, PS_SLOTS_A to PS_SLOTS_D, mark which slots 9-31 are taken
//     (seven slots each), so a slot stays reserved even while its companion is out of the
//     roster for a while.
// All five go in globalcat.2da (holopatcher/changes_globalcat.ini has the lines; the shared ones are added
// only if missing, so any number of mods can ship them).
//
//   int nSlot = PS_AddByTemplate("MYMOD_PARTY_SLOT", "my_companion");   // the first free slot
//   if (nSlot == -1) { ... no roster patch, or every slot taken: the mod's own fallback ... }
//
// Then use nSlot (or PS_SlotOf("MYMOD_PARTY_SLOT") later) wherever the game wants an NPC_ number:
// AddPartyMember, IsNPCPartyMember, SetNPCSelectability, RemoveAvailableNPC...
//
// Safe on a game without the roster patch: the engine refuses slots past 8 (they read as not
// available, and adding to one fails), so the Add functions return -1 and change nothing.

int PS_FIRST = 9;     // (plain ints: the KotOR I compiler has no "const")
int PS_LAST = 31;

// which shared global marks a slot, and its bit there (seven slots to a global: they are
// signed bytes, and the top bit is left alone)
string PS_MarkGlobal(int nSlot)
{
    int i = (nSlot - PS_FIRST) / 7;
    if (i == 0) return "PS_SLOTS_A";
    if (i == 1) return "PS_SLOTS_B";
    if (i == 2) return "PS_SLOTS_C";
    return "PS_SLOTS_D";
}

int PS_MarkBit(int nSlot)
{
    return 1 << ((nSlot - PS_FIRST) % 7);
}

// TRUE if a mod has claimed this slot (whether or not its companion is in the roster now)
int PS_IsClaimed(int nSlot)
{
    if (nSlot < PS_FIRST || nSlot > PS_LAST) return FALSE;
    return (GetGlobalNumber(PS_MarkGlobal(nSlot)) & PS_MarkBit(nSlot)) != 0;
}

void PS_SetClaimed(int nSlot, int bClaimed)
{
    if (nSlot < PS_FIRST || nSlot > PS_LAST) return;
    string sGlobal = PS_MarkGlobal(nSlot);
    int n = GetGlobalNumber(sGlobal);
    if (bClaimed) n = n | PS_MarkBit(nSlot);
    else n = n & ~PS_MarkBit(nSlot);
    SetGlobalNumber(sGlobal, n);
}

// the slot a mod holds, by its own global; -1 if none
int PS_SlotOf(string sGlobal)
{
    int n = GetGlobalNumber(sGlobal);
    if (n >= PS_FIRST && n <= PS_LAST) return n;
    return -1;
}

// the first slot from 9 that holds no companion and that no mod has claimed; -1 if none
int PS_FindFree()
{
    int n;
    for (n = PS_FIRST; n <= PS_LAST; n++)
    {
        if (!IsAvailableCreature(n) && !PS_IsClaimed(n)) return n;
    }
    return -1;
}

// The mod's slot: the one it already holds, else the first free one, claimed now. A claim is a
// reservation only; it does not check that the roster patch is there. To add a companion, use
// PS_AddByTemplate / PS_AddByObject, which do.
int PS_Claim(string sGlobal)
{
    int n = PS_SlotOf(sGlobal);
    if (n != -1) return n;
    n = PS_FindFree();
    if (n == -1) return -1;
    PS_SetClaimed(n, TRUE);
    SetGlobalNumber(sGlobal, n);
    return n;
}

// Give the slot back (the companion has left the game for good). Removing the companion from
// the roster (RemoveAvailableNPC) is the mod's own step.
void PS_Release(string sGlobal)
{
    int n = PS_SlotOf(sGlobal);
    if (n == -1) return;
    PS_SetClaimed(n, FALSE);
    SetGlobalNumber(sGlobal, 0);
}

// The companion into the mod's slot (claimed if it has none), from its template. Returns the slot,
// or -1 with nothing changed (no roster patch, so the engine refused, or no free slot).
int PS_AddByTemplate(string sGlobal, string sTemplate)
{
    int bHad = PS_SlotOf(sGlobal) != -1;
    int n = PS_Claim(sGlobal);
    if (n == -1) return -1;
    if (IsAvailableCreature(n)) return n;                 // already there
    if (AddAvailableNPCByTemplate(n, sTemplate) && IsAvailableCreature(n)) return n;
    if (!bHad) PS_Release(sGlobal);
    return -1;
}

// ...or an existing creature (keeps its levels, gear and choices). Same answer as above.
int PS_AddByObject(string sGlobal, object oCreature)
{
    int bHad = PS_SlotOf(sGlobal) != -1;
    int n = PS_Claim(sGlobal);
    if (n == -1) return -1;
    if (IsAvailableCreature(n)) return n;
    if (AddAvailableNPCByObject(n, oCreature) && IsAvailableCreature(n)) return n;
    if (!bHad) PS_Release(sGlobal);
    return -1;
}
