#include "creatureStats.h"

VirtualMachineReturnTypes __stdcall ExecuteCommandGetFeatAcquired(DWORD routine, int paramCount)
{
	debugLog("[PATCH] Running GetFeatAcquired");

	int outcome = 0;

	CVirtualMachine* vm = CVirtualMachine::GetInstance();
	if (!vm) return COMMAND_PARAM_ERROR;

	int feat;
	if (!vm->StackPopInteger(&feat)) {
		delete vm;
		return COMMAND_PARAM_ERROR;
	}

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

	void* objectArrayPtr = server->GetObjectArray();
	if (!objectArrayPtr) {
		delete server;
		delete vm;
		return COMMAND_PARAM_ERROR;
	}

	CGameObjectArray objectArray(objectArrayPtr);
	void* creatureObject = objectArray.GetGameObject(creature);

	// Game object vtable[12] = AsSWSCreature
	void* serverCreature = callVirtualFunction<void*>(creatureObject, 12);
	if (!serverCreature)
	{
		vm->StackPushInteger(outcome);
		delete server;
		delete vm;
		return SUCCESS;
	}

	CSWSCreature serverCreatureWrapper(serverCreature);
	CSWSCreatureStats* stats = serverCreatureWrapper.GetCreatureStats();
	if (!stats) {
		vm->StackPushInteger(outcome);
		delete server;
		delete vm;
		return SUCCESS;
	}

	outcome = stats->HasFeat((WORD)feat);

	if (!vm->StackPushInteger(outcome)) {
		delete stats;
		delete server;
		delete vm;
		return COMMAND_RETURN_ERROR;
	}

	delete stats;
	delete server;
	delete vm;
	return SUCCESS;
}

VirtualMachineReturnTypes __stdcall ExecuteCommandGetSpellAcquired(DWORD routine, int paramCount)
{
	debugLog("[PATCH] Running GetSpellAcquired");

	int outcome = 0;

	CVirtualMachine* vm = CVirtualMachine::GetInstance();
	if (!vm) return COMMAND_PARAM_ERROR;

	int spell;
	if (!vm->StackPopInteger(&spell)) {
		delete vm;
		return COMMAND_PARAM_ERROR;
	}

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
		CSWSCreatureStats* stats = serverCreature->GetCreatureStats();

		if (stats) {
			outcome = stats->HasSpell(0, (DWORD)spell, 0);
			delete stats;
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

VirtualMachineReturnTypes __stdcall ExecuteCommandGrantAbility(DWORD routine, int paramCount)
{
	CVirtualMachine* vm = CVirtualMachine::GetInstance();
	if (!vm) return COMMAND_PARAM_ERROR;

	int ability;
	if (!vm->StackPopInteger(&ability)) {
		delete vm;
		return COMMAND_PARAM_ERROR;
	}

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

	if (!serverCreature) {
		delete server;
		delete vm;
		return SUCCESS;
	}

	CSWSCreatureStats* stats = serverCreature->GetCreatureStats();
	if (!stats) {
		delete serverCreature;
		delete server;
		delete vm;
		return SUCCESS;
	}

	if (routine == GrantFeatIndex) {
		stats->AddFeat((WORD)ability);
	}
	else if (routine == GrantSpellIndex)
	{
		// Give to last class for now
		// In the future consider alternate options to guarantee Jedi class
		BYTE classCount = stats->GetClassCount();
		if (classCount > 0) {
			BYTE classIndex = classCount - 1;
			stats->AddKnownSpell(classIndex, (DWORD)ability);
		}
	}

	delete stats;
	delete serverCreature;
	delete server;
	delete vm;
	return SUCCESS;
}

VirtualMachineReturnTypes __stdcall ExecuteCommandAdjustCreatureAttributes(DWORD routine, int paramCount)
{
	CVirtualMachine* vm = CVirtualMachine::GetInstance();
	if (!vm) return COMMAND_PARAM_ERROR;

	DWORD object;
	if (!vm->StackPopObject(&object)) {
		delete vm;
		return COMMAND_PARAM_ERROR;
	}

	int attribute;
	if (!vm->StackPopInteger(&attribute)) {
		delete vm;
		return COMMAND_PARAM_ERROR;
	}

	int amount;
	if (!vm->StackPopInteger(&amount)) {
		delete vm;
		return COMMAND_PARAM_ERROR;
	}

	CServerExoApp* server = CServerExoApp::GetInstance();
	if (!server) {
		delete vm;
		return COMMAND_PARAM_ERROR;
	}

	CSWSCreature* serverCreature = server->GetCreatureByGameObjectID(object);
	if (!serverCreature) {
		delete server;
		delete vm;
		return SUCCESS;
	}

	CSWSCreatureStats* stats = serverCreature->GetCreatureStats();
	if (!stats) {
		delete serverCreature;
		delete server;
		delete vm;
		return SUCCESS;
	}

	BYTE baseCurrent = 0;
	int offset = -1;

	switch ((Attributes)attribute) {
	case STR:
		if (offset >= 0) {
			baseCurrent = stats->GetSTRBase();
			stats->SetSTRBase((BYTE)(amount + (int)baseCurrent));
		}
		break;
	case DEX:
		if (offset >= 0) {
			baseCurrent = stats->GetDEXBase();
			stats->SetDEXBase((BYTE)(amount + (int)baseCurrent));
		}
		break;
	case CON:
		if (offset >= 0) {
			baseCurrent = stats->GetCONBase();
			stats->SetCONBase((BYTE)(amount + (int)baseCurrent), 1);
		}
		break;
	case INTEL:
		if (offset >= 0) {
			baseCurrent = stats->GetINTBase();
			stats->SetINTBase((BYTE)(amount + (int)baseCurrent));
		}
		break;
	case WIS:
		if (offset >= 0) {
			baseCurrent = stats->GetWISBase();
			stats->SetWISBase((BYTE)(amount + (int)baseCurrent));
		}
		break;
	case CHA:
		if (offset >= 0) {
			baseCurrent = stats->GetCHABase();
			stats->SetCHABase((BYTE)(amount + (int)baseCurrent));
		}
		break;
	}

	delete stats;
	delete serverCreature;
	delete server;
	delete vm;
	return SUCCESS;
}

VirtualMachineReturnTypes __stdcall ExecuteCommandAdjustCreatureSkills(DWORD routine, int paramCount)
{
	CVirtualMachine* vm = CVirtualMachine::GetInstance();
	if (!vm) return COMMAND_PARAM_ERROR;

	DWORD object;
	if (!vm->StackPopObject(&object)) {
		delete vm;
		return COMMAND_PARAM_ERROR;
	}

	int skill;
	if (!vm->StackPopInteger(&skill)) {
		delete vm;
		return COMMAND_PARAM_ERROR;
	}

	int amount;
	if (!vm->StackPopInteger(&amount)) {
		delete vm;
		return COMMAND_PARAM_ERROR;
	}

	CServerExoApp* server = CServerExoApp::GetInstance();
	if (!server) {
		delete vm;
		return COMMAND_PARAM_ERROR;
	}

	CSWSCreature* serverCreature = server->GetCreatureByGameObjectID(object);
	if (!serverCreature) {
		delete server;
		delete vm;
		return SUCCESS;
	}

	CSWSCreatureStats* stats = serverCreature->GetCreatureStats();
	if (!stats) {
		delete server;
		delete vm;
		return SUCCESS;
	}

	BYTE baseCurrent = stats->GetSkillRank((BYTE)skill, NULL, 1);
	stats->SetSkillRank((BYTE)skill, (BYTE)((int)baseCurrent + amount));

	delete server;
	delete vm;
	return SUCCESS;
}

VirtualMachineReturnTypes __stdcall ExecuteCommandGetSkillRankBase(DWORD routine, int paramCount)
{
	CVirtualMachine* vm = CVirtualMachine::GetInstance();
	if (!vm) return COMMAND_PARAM_ERROR;

	int skill;
	if (!vm->StackPopInteger(&skill)) {
		delete vm;
		return COMMAND_PARAM_ERROR;
	}

	DWORD object;
	if (!vm->StackPopObject(&object)) {
		delete vm;
		return COMMAND_PARAM_ERROR;
	}

	CServerExoApp* server = CServerExoApp::GetInstance();
	if (!server) {
		delete vm;
		return COMMAND_PARAM_ERROR;
	}

	CSWSCreature* serverCreature = server->GetCreatureByGameObjectID(object);

	if (!serverCreature) {
		vm->StackPushInteger(-1);
		delete server;
		delete vm;
		return SUCCESS;
	}

	CSWSCreatureStats* stats = serverCreature->GetCreatureStats();
	if (!stats) {
		vm->StackPushInteger(-1);
		delete server;
		delete vm;
		return SUCCESS;
	}

	BYTE skillBase = stats->GetSkillRank((BYTE)skill, NULL, 1);

	if (!vm->StackPushInteger(skillBase)) {
		delete server;
		delete vm;
		return COMMAND_RETURN_ERROR;
	}

	delete server;
	delete vm;
	return SUCCESS;
}
