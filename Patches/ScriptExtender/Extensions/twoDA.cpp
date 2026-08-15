#include "twoDA.h"
#include "Common.h"
#include "GameAPI/CExoString.h"
#include "GameAPI/C2DA.h"

VirtualMachineReturnTypes __stdcall ExecuteCommandGet2DAString(DWORD routine, int paramCount)
{
    debugLog("[ScriptExtender] Running Get2DAString\n");

    CVirtualMachine* vm = CVirtualMachine::GetInstance();
    if (!vm) return COMMAND_PARAM_ERROR;

    CExoString s2daName;
    if (!vm->StackPopString(&s2daName)) {
        delete vm;
        return COMMAND_PARAM_ERROR;
    }

    int row;
    if (!vm->StackPopInteger(&row)) {
        delete vm;
        return COMMAND_PARAM_ERROR;
    }

    CExoString column;
    if (!vm->StackPopString(&column)) {
        delete vm;
        return COMMAND_PARAM_ERROR;
    }

    char* nameStr = s2daName.GetCStr();
    if (!nameStr) {
        CExoString emptyResult("");
        if (!vm->StackPushString(&emptyResult)) {
            delete vm;
            return COMMAND_RETURN_ERROR;
        }
        delete vm;
        return SUCCESS;
    }

    C2DA twoDA(nameStr);
    if (!twoDA.GetPtr()) {
        CExoString emptyResult("");
        if (!vm->StackPushString(&emptyResult)) {
            delete vm;
            return COMMAND_RETURN_ERROR;
        }
        delete vm;
        return SUCCESS;
    }

    CExoString result;
    bool success = twoDA.GetCExoStringEntry(row, &column, &result);

    if (!success) {
        CExoString emptyResult("");
        if (!vm->StackPushString(&emptyResult)) {
            delete vm;
            return COMMAND_RETURN_ERROR;
        }
        delete vm;
        return SUCCESS;
    }

    if (!vm->StackPushString(&result)) {
        delete vm;
        return COMMAND_RETURN_ERROR;
    }

    delete vm;
    return SUCCESS;
}

VirtualMachineReturnTypes __stdcall ExecuteCommandGet2DAInt(DWORD routine, int paramCount)
{
    debugLog("[ScriptExtender] Running Get2DAInt\n");

    CVirtualMachine* vm = CVirtualMachine::GetInstance();
    if (!vm) return COMMAND_PARAM_ERROR;

    CExoString s2daName;
    if (!vm->StackPopString(&s2daName)) {
        delete vm;
        return COMMAND_PARAM_ERROR;
    }

    int row;
    if (!vm->StackPopInteger(&row)) {
        delete vm;
        return COMMAND_PARAM_ERROR;
    }

    CExoString column;
    if (!vm->StackPopString(&column)) {
        delete vm;
        return COMMAND_PARAM_ERROR;
    }

    char* nameStr = s2daName.GetCStr();
    if (!nameStr) {
        if (!vm->StackPushInteger(0)) {
            delete vm;
            return COMMAND_RETURN_ERROR;
        }
        delete vm;
        return SUCCESS;
    }

    C2DA twoDA(nameStr);
    if (!twoDA.GetPtr()) {
        if (!vm->StackPushInteger(0)) {
            delete vm;
            return COMMAND_RETURN_ERROR;
        }
        delete vm;
        return SUCCESS;
    }

    int result = 0;
    bool success = twoDA.GetINTEntry(row, &column, &result);

    if (!success) {
        if (!vm->StackPushInteger(0)) {
            delete vm;
            return COMMAND_RETURN_ERROR;
        }
        delete vm;
        return SUCCESS;
    }

    if (!vm->StackPushInteger(result)) {
        delete vm;
        return COMMAND_RETURN_ERROR;
    }

    delete vm;
    return SUCCESS;
}

VirtualMachineReturnTypes __stdcall ExecuteCommandGet2DAFloat(DWORD routine, int paramCount)
{
    debugLog("[ScriptExtender] Running Get2DAFloat\n");

    CVirtualMachine* vm = CVirtualMachine::GetInstance();
    if (!vm) return COMMAND_PARAM_ERROR;

    CExoString s2daName;
    if (!vm->StackPopString(&s2daName)) {
        delete vm;
        return COMMAND_PARAM_ERROR;
    }

    int row;
    if (!vm->StackPopInteger(&row)) {
        delete vm;
        return COMMAND_PARAM_ERROR;
    }

    CExoString column;
    if (!vm->StackPopString(&column)) {
        delete vm;
        return COMMAND_PARAM_ERROR;
    }

    char* nameStr = s2daName.GetCStr();
    if (!nameStr) {
        if (!vm->StackPushFloat(0.0f)) {
            delete vm;
            return COMMAND_RETURN_ERROR;
        }
        delete vm;
        return SUCCESS;
    }

    C2DA twoDA(nameStr);
    if (!twoDA.GetPtr()) {
        if (!vm->StackPushFloat(0.0f)) {
            delete vm;
            return COMMAND_RETURN_ERROR;
        }
        delete vm;
        return SUCCESS;
    }

    float result = 0.0f;
    bool success = twoDA.GetFLOATEntry(row, &column, &result);

    if (!success) {
        if (!vm->StackPushFloat(0.0f)) {
            delete vm;
            return COMMAND_RETURN_ERROR;
        }
        delete vm;
        return SUCCESS;
    }

    if (!vm->StackPushFloat(result)) {
        delete vm;
        return COMMAND_RETURN_ERROR;
    }

    delete vm;
    return SUCCESS;
}
