#include "Common.h"
#include "Kotor1Functions.h"

extern "C" void __cdecl AddBonusMainHandAttack(void* combatRound) {
	void* playerCreature = getObjectProperty<void*>(combatRound, 0x9b4);
	void* inventory = getObjectProperty<void*>(playerCreature, 0xa2c);
	void* item = sWInventoryGetItemInSlot(inventory, 0x10); // Main hand slot
	void* baseitem = sWItemGetBaseItem(item);
	BYTE weaponWield = getObjectProperty<BYTE>(baseitem, 0x8);

	if (weaponWield == (BYTE)6) {
		int onHandAttacks = getObjectProperty<int>(combatRound, 0x990);
		setObjectProperty<int>(combatRound, 0x990, onHandAttacks + 1);
	}
}