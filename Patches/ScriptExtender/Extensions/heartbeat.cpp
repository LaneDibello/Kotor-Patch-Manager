#include "heartbeat.h"

VirtualMachineReturnTypes __stdcall ExecuteForceHeartbeat(DWORD routine, int paramCount) {
	CVirtualMachine* vm = CVirtualMachine::GetInstance();
	if (!vm) return COMMAND_PARAM_ERROR;

	DWORD objectId;
	if (!vm->StackPopObject(&objectId)) {
		delete vm;
		return COMMAND_PARAM_ERROR;
	}

	CServerExoApp* server = CServerExoApp::GetInstance();

	CGameObject* object = server->GetGameObject(objectId);

	// Split handling based on game object type
	switch (object->GetObjectTypeEnum()) {
	case AREA:
		// CSWSArea not yet implemented
		break;
	case AREAOFEFFECT:
		CSWSAreaOfEffectObject* aoe = object->AsSWSAreaOfEffectObject();
		if (!aoe) break;

		delete aoe;
		break;
	case CREATURE:
		CSWSCreature* creature = object->AsSWSCreature();
		if (!creature) break;

		delete creature;
		break;
	case DOOR:
		CSWSDoor* door = object->AsSWSDoor();
		if (!door) break;

		delete door;
		break;
	case ENCOUNTER:
		CSWSEncounter* encounter = object->AsSWSEncounter();
		if (!encounter) break;
		encounter->SetHeartbeatDay(0);
		encounter->SetHeartbeatTime(1);
		delete encounter;
		break;
	case MODULE:
		// CSWSModule not yet implemented
		break;
	case PLACEABLE:
		CSWSPlaceable* placeable = object->AsCSWSPlaceable();
		if (!placeable) break;
		
		delete placeable;
		break;
	case TRIGGER:
		CSWSTrigger* trigger = object->AsSWSTrigger();
		if (!trigger) break;

		delete trigger;
		break;
	default:
		//do nothing
		break;
	}


	// Use the `As` object type methods to get it in the proper form
	// Set the timer or timestamp to the appropriate value to cause a heartbeat to occur
}