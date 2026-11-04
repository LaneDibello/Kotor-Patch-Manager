// ps_example.nss: an example recruit script using Party Slots (ps_slots.nss).
// The companion goes into the first free slot from 9 up; without the roster patch
// (or with every slot taken) the mod falls back to a vanilla slot of its choosing.
#include "ps_slots"

void main()
{
    int nSlot = PS_AddByTemplate("MYMOD_PARTY_SLOT", "my_companion");
    if (nSlot == -1)
    {
        // the fallback: here, sharing T3-M4's slot 7 the way mods did before the roster patch
        nSlot = 7;
        if (!AddAvailableNPCByTemplate(nSlot, "my_companion")) return;
    }
    SetNPCSelectability(nSlot, TRUE);
    // straight into the party, as many recruit scenes do
    AddPartyMember(nSlot, GetObjectByTag("my_companion"));
}
