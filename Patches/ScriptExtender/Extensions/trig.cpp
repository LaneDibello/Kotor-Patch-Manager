#include "trig.h"


static const double kPi = 3.14159265358979323846;
static const double kDegToRad = kPi / 180.0;
static const double kRadToDeg = 180.0 / kPi;


static const double kTrigEpsilon = 1e-6;

VirtualMachineReturnTypes __stdcall ExecuteCommandTrig(DWORD routine, int paramCount) {
	if (paramCount != 1) {
		debugLog("[ScriptExtender] Wrong number of Params for ExecuteCommandTrig, expected 1, got %d", paramCount);
		return COMMAND_PARAM_ERROR;
	}

	CVirtualMachine* vm = CVirtualMachine::GetInstance();
	if (!vm) return COMMAND_PARAM_ERROR;

	float fValue;
	if (!vm->StackPopFloat(&fValue)) {
		delete vm;
		return COMMAND_PARAM_ERROR;
	}

	double radians = (double)fValue * kDegToRad;
	double numerator = 0.0;
	double denominator = 1.0;

	switch (routine) {
	case secIndex:
		numerator = 1.0;
		denominator = cos(radians);
		break;
	case cscIndex:
		numerator = 1.0;
		denominator = sin(radians);
		break;
	case cotIndex:
		numerator = cos(radians);
		denominator = sin(radians);
		break;
	default:
		delete vm;
		return COMMAND_NOT_FOUND;
	}

	float output = 0.0f;
	if (fabs(denominator) <= kTrigEpsilon) {
		debugLog("[ScriptExtender] Trig routine %d is undefined at %f degrees, returning 0.0", routine, fValue);
	}
	else {
		output = (float)(numerator / denominator);
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

	float degrees = (float)((double)radians * kRadToDeg);

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

	float radians = (float)((double)degrees * kDegToRad);

	if (!vm->StackPushFloat(radians)) {
		delete vm;
		return COMMAND_RETURN_ERROR;
	}

	delete vm;
	return SUCCESS;
}
