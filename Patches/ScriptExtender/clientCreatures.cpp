#include "clientCreatures.h"

int __stdcall ExecuteCommandIsRunning(DWORD routine, int paramCount)
{
	int outcome = 0;

	DWORD creature;
	if (!virtualMachineStackPopObject(*VIRTUAL_MACHINE_PTR, &creature))
		return -2001;

	void* serverCreature = serverExoAppGetCreatureByGameObjectID(getServerExoApp(), creature);

	if (serverCreature) {
		void* clientCreature = sWSCreatureGetClientCreature(serverCreature);
		if (clientCreature) {
			outcome = getObjectProperty<int>(clientCreature, 0x3e0);
		}
	}

	if (!virtualMachineStackPushInteger(*VIRTUAL_MACHINE_PTR, outcome))
		return -2000;

	return 0;
}

int __stdcall ExecuteCommandIsStealthed(DWORD routine, int paramCount) {
	int outcome = 0;

	DWORD creature;
	if (!virtualMachineStackPopObject(*VIRTUAL_MACHINE_PTR, &creature))
		return -2001;

	void* serverCreature = serverExoAppGetCreatureByGameObjectID(getServerExoApp(), creature);

	if (serverCreature) {
		void* clientCreature = sWSCreatureGetClientCreature(serverCreature);
		if (clientCreature) {
			outcome = getObjectProperty<int>(clientCreature, 0x194);
		}
	}

	if (!virtualMachineStackPushInteger(*VIRTUAL_MACHINE_PTR, outcome))
		return -2000;

	return 0;
}