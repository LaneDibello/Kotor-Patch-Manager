#include "ini.h"

VirtualMachineReturnTypes __stdcall ExecuteCommandReadIniEntry(DWORD routine, int paramCount) {
	CVirtualMachine* vm = CVirtualMachine::GetInstance();
	if (!vm) return COMMAND_PARAM_ERROR;

	CExoString filename;
	if (!vm->StackPopString(&filename)) {
		delete vm;
		return COMMAND_PARAM_ERROR;
	}

	CExoString category;
	if (!vm->StackPopString(&category)) {
		delete vm;
		return COMMAND_PARAM_ERROR;
	}

	CExoString key;
	if (!vm->StackPopString(&key)) {
		delete vm;
		return COMMAND_PARAM_ERROR;
	}

	CExoIni ini;
	CExoString outValue("");

	int result = ini.ReadIniEntry(&outValue, &filename, &category, &key);

	if (result == 0) {
		debugLog("[ScriptExtender] Failed to read INI entry:[%s] %s in file: %s", category.GetCStr(), key.GetCStr(), filename.GetCStr());
	}

	if (!vm->StackPushString(&outValue)) {
		delete vm;
		return COMMAND_RETURN_ERROR;
	}

	delete vm;
	return SUCCESS;
}

VirtualMachineReturnTypes __stdcall ExecuteCommandWriteIniEntry(DWORD routine, int paramCount) {
	CVirtualMachine* vm = CVirtualMachine::GetInstance();
	if (!vm) return COMMAND_PARAM_ERROR;

	CExoString value;
	if (!vm->StackPopString(&value)) {
		delete vm;
		return COMMAND_PARAM_ERROR;
	}
	
	CExoString filename;
	if (!vm->StackPopString(&filename)) {
		delete vm;
		return COMMAND_PARAM_ERROR;
	}

	CExoString category;
	if (!vm->StackPopString(&category)) {
		delete vm;
		return COMMAND_PARAM_ERROR;
	}

	CExoString key;
	if (!vm->StackPopString(&key)) {
		delete vm;
		return COMMAND_PARAM_ERROR;
	}

	CExoIni ini;

	int result = ini.WriteIniEntry(&value, &filename, &category, &key);

	if (result == 0) {
		debugLog("[ScriptExtender] Failed to write INI entry:[%s] %s=%s to file: %s", category.GetCStr(), key.GetCStr(), value.GetCStr(), filename.GetCStr());
	}

	delete vm;
	return SUCCESS;
}