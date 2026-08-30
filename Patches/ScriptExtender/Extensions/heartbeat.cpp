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
	// Use the `As` object type methods to get it in the proper form
	// Set the timer or timestamp to the appropriate value to cause a heartbeat to occur
}