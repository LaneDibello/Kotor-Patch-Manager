#include "globalModifiers.h"

VirtualMachineReturnTypes __stdcall ExecuteCommandAdjustGlobalNumber(DWORD routine, int paramCount) {
	CVirtualMachine* vm = CVirtualMachine::GetInstance();
	if (!vm) return COMMAND_PARAM_ERROR;

    CExoString indentifier;
    if (!vm->StackPopString(&indentifier)) {
        delete vm;
        return COMMAND_PARAM_ERROR;
    }

    int amount;
    if (!vm->StackPopInteger(&amount)) {
        delete vm;
        return COMMAND_PARAM_ERROR;
    }

    CServerExoApp* server = CServerExoApp::GetInstance();
    void* globalVars = server->GetGlobalVariableTable();

    typedef void (__thiscall* SetValueNumberFn)(void* thisPtr, void* id, BYTE value);
    typedef void(__thiscall* GetValueNumberFn)(void* thisPtr, void* id, int * outNumber);

    SetValueNumberFn setValueNumber = reinterpret_cast<SetValueNumberFn>(
        GameVersion::GetFunctionAddress("CSWGlobalVariableTable", "SetValueNumber")
        );
    GetValueNumberFn getValueNumber = reinterpret_cast<GetValueNumberFn>(
        GameVersion::GetFunctionAddress("CSWGlobalVariableTable", "GetValueNumber")
        );

    int prev;
    getValueNumber(globalVars, indentifier.GetPtr(), &prev);

    if (routine == DecrementGlobalNumberIndex) {
        amount *= -1;
    }

    int value = prev + amount;

    if (value > 127 || value < -128) {
        delete server;
        delete vm;
        return COMMAND_RETURN_ERROR;
    }

    setValueNumber(globalVars, indentifier.GetPtr(), value);

    delete server;
    delete vm;

    return SUCCESS;
}