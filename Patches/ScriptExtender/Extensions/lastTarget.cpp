#include "lastTarget.h"

VirtualMachineReturnTypes __stdcall ExecuteCommandetGetPlayerLastTarget(DWORD routine, int paramCount) {
	CVirtualMachine* vm = CVirtualMachine::GetInstance();
	if (!vm) return COMMAND_PARAM_ERROR;

	CServerExoApp* server = CServerExoApp::GetInstance();
	CClientExoApp* client = CClientExoApp::GetInstance();

	DWORD clientTarget = client->GetLastTarget();
	DWORD serverTarget = server->ClientToServerObjectId(clientTarget);

	if (!vm->StackPushObject(serverTarget)) {
		delete server;
		delete client;
		delete vm;
		return COMMAND_RETURN_ERROR;
	}

	delete server;
	delete client;
	delete vm;
	return SUCCESS;
}