#include "clientCreatures.h"

VirtualMachineReturnTypes __stdcall ExecuteCommandIsRunning(DWORD routine, int paramCount)
{
	debugLog("[PATCH] Running IsRunning");

	int outcome = 0;

	CVirtualMachine* vm = CVirtualMachine::GetInstance();
	if (!vm) return COMMAND_PARAM_ERROR;

	DWORD creature;
	if (!vm->StackPopObject(&creature)) {
		delete vm;
		return COMMAND_PARAM_ERROR;
	}

	CServerExoApp* server = CServerExoApp::GetInstance();
	if (!server) {
		delete vm;
		return COMMAND_PARAM_ERROR;
	}

	CSWSCreature* serverCreature = server->GetCreatureByGameObjectID(creature);
	if (serverCreature) {
		CSWCCreature* clientCreature = serverCreature->GetClientCreature();
		if (clientCreature) {
			outcome = (int)clientCreature->GetRunning();
			delete clientCreature;
		}
		delete serverCreature;
	}

	if (!vm->StackPushInteger(outcome)) {
		delete server;
		delete vm;
		return COMMAND_RETURN_ERROR;
	}

	delete server;
	delete vm;
	return SUCCESS;
}

VirtualMachineReturnTypes __stdcall ExecuteCommandIsStealthed(DWORD routine, int paramCount)
{
	debugLog("[PATCH] Running IsStealthed");

	int outcome = 0;

	CVirtualMachine* vm = CVirtualMachine::GetInstance();
	if (!vm) return COMMAND_PARAM_ERROR;

	DWORD creature;
	if (!vm->StackPopObject(&creature)) {
		delete vm;
		return COMMAND_PARAM_ERROR;
	}

	CServerExoApp* server = CServerExoApp::GetInstance();
	if (!server) {
		delete vm;
		return COMMAND_PARAM_ERROR;
	}

	CSWSCreature* serverCreature = server->GetCreatureByGameObjectID(creature);
	if (serverCreature) {
		CSWCCreature* clientCreature = serverCreature->GetClientCreature();
		if (clientCreature) {
			outcome = (int)clientCreature->GetStealth();
			delete clientCreature;
		}
		delete serverCreature;
	}

	if (!vm->StackPushInteger(outcome)) {
		delete server;
		delete vm;
		return COMMAND_RETURN_ERROR;
	}

	delete server;
	delete vm;
	return SUCCESS;
}