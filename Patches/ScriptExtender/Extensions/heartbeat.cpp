#include "heartbeat.h"

VirtualMachineReturnTypes __stdcall ExecuteForceHeartbeat(DWORD routine, int paramCount) {
	CVirtualMachine* vm = CVirtualMachine::GetInstance();
	if (!vm) return COMMAND_PARAM_ERROR;

	DWORD object;
	if (!vm->StackPopObject(&object)) {
		delete vm;
		return COMMAND_PARAM_ERROR;
	}


}