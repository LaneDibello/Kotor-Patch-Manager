#include "consoleCommand.h"

VirtualMachineReturnTypes __stdcall ExecuteCommandRunConsoleCommand(DWORD routine, int paramCount) {
	CVirtualMachine* vm = CVirtualMachine::GetInstance();
	if (!vm) return COMMAND_PARAM_ERROR;

	CExoString command;
	if (!vm->StackPopString(&command)) {
		delete vm;
		return COMMAND_PARAM_ERROR;
	}

	typedef char*(__cdecl* FuncInterpFn)(char* consoleCommand);
	FuncInterpFn funcInterp = 
		reinterpret_cast<FuncInterpFn>(
			GameVersion::GetFunctionAddress("Global", "FuncInterp")
			);

	debugLog("[ScriptExtender] Running Console Command %s", command.GetCStr());

	char* result = funcInterp(command.GetCStr());

	CExoString returnVal(result);
	if (!vm->StackPushString(&returnVal)) {
		delete vm;
		return COMMAND_RETURN_ERROR;
	}

	return SUCCESS;
}