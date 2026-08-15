#include "trig.h"

VirtualMachineReturnTypes __stdcall ExecuteCommandTrig(DWORD routine, int paramCount) {
	if (paramCount != 1) {
		debugLog("[ScriptExtender] Wrong number of Params for ExecuteCommandTrig, expected 1, got %d", paramCount);
		return COMMAND_PARAM_ERROR;
	}

	CVirtualMachine* vm = CVirtualMachine::GetInstance();
	if (!vm) return COMMAND_PARAM_ERROR;

	float input;
	if (!vm->StackPopFloat(&input)) {
		delete vm;
		return COMMAND_PARAM_ERROR;
	}

	float output;
	switch (routine) {
	case secIndex:
		output = (float)(1 / cos((double)input));
		break;
	case cscIndex:
		output = (float)(1 / sin((double)input));
		break;
	case cotIndex:
		output = (float)(cos((double)input) / sin((double)input));
		break;
	default:
		delete vm;
		return COMMAND_NOT_FOUND;
	}

	if (!vm->StackPushFloat(output)) {
		delete vm;
		return COMMAND_RETURN_ERROR;
	}

	delete vm;
	return SUCCESS;
}

VirtualMachineReturnTypes __stdcall ExecuteCommandRadToDeg(DWORD routine, int paramCount) {
	if (paramCount != 1) {
		debugLog("[ScriptExtender] Wrong number of Params for ExecuteCommandRadToDeg, expected 1, got %d", paramCount);
		return COMMAND_PARAM_ERROR;
	}

	CVirtualMachine* vm = CVirtualMachine::GetInstance();
	if (!vm) return COMMAND_PARAM_ERROR;

	float radians;
	if (!vm->StackPopFloat(&radians)) {
		delete vm;
		return COMMAND_PARAM_ERROR;
	}

	float degrees = radians * (180 / M_PI);

	if (!vm->StackPushFloat(degrees)) {
		delete vm;
		return COMMAND_RETURN_ERROR;
	}

	delete vm;
	return SUCCESS;
}

VirtualMachineReturnTypes __stdcall ExecuteCommandDegToRad(DWORD routine, int paramCount) {
	if (paramCount != 1) {
		debugLog("[ScriptExtender] Wrong number of Params for ExecuteCommandDegToRad, expected 1, got %d", paramCount);
		return COMMAND_PARAM_ERROR;
	}

	CVirtualMachine* vm = CVirtualMachine::GetInstance();
	if (!vm) return COMMAND_PARAM_ERROR;

	float degrees;
	if (!vm->StackPopFloat(&degrees)) {
		delete vm;
		return COMMAND_PARAM_ERROR;
	}

	float radians = degrees * (M_PI / 180);

	if (!vm->StackPushFloat(radians)) {
		delete vm;
		return COMMAND_RETURN_ERROR;
	}

	delete vm;
	return SUCCESS;
}