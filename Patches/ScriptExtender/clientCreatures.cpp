#include "clientCreatures.h"

int __stdcall ExecuteCommandIsRunning(DWORD routine, int paramCount)
{
	debugLog("[PATCH] Running IsRunning");

	int outcome = 0;

	CVirtualMachine* vm = CVirtualMachine::GetInstance();
	if (!vm) return -2001;

	DWORD creature;
	if (!vm->StackPopObject(&creature)) {
		delete vm;
		return -2001;
	}

	CServerExoApp* server = CServerExoApp::GetInstance();
	if (!server) {
		delete vm;
		return -2001;
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
		return -2000;
	}

	delete server;
	delete vm;
	return 0;
}

int __stdcall ExecuteCommandIsStealthed(DWORD routine, int paramCount)
{
	debugLog("[PATCH] Running IsStealthed");

	int outcome = 0;

	CVirtualMachine* vm = CVirtualMachine::GetInstance();
	if (!vm) return -2001;

	DWORD creature;
	if (!vm->StackPopObject(&creature)) {
		delete vm;
		return -2001;
	}

	CServerExoApp* server = CServerExoApp::GetInstance();
	if (!server) {
		delete vm;
		return -2001;
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
		return -2000;
	}

	delete server;
	delete vm;
	return 0;
}