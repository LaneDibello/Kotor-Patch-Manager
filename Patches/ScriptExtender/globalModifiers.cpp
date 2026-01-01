#include "globalModifiers.h"


int __stdcall ExecuteCommandIncrementGlobalNumber(DWORD routine, int paramCount) {
	CVirtualMachine* vm = CVirtualMachine::GetInstance();
	if (!vm) return -2001;

    CExoString indentifier;
    if (!vm->StackPopString(&indentifier)) {
        delete vm;
        return -2001;
    }

    int amount;
    if (!vm->StackPopInteger(&amount)) {
        delete vm;
        return -2001;
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

    int value = prev + amount;

    if (value > 127) {
        delete vm;
        return -2000;
    }

    setValueNumber(globalVars, indentifier.GetPtr(), value);

    return 0;
}


int __stdcall ExecuteCommandDecrementGlobalNumber(DWORD routine, int paramCount) {
    return 0;
}