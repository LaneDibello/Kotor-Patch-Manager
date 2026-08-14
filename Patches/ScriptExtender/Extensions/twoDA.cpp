#include "twoDA.h"
#include "Common.h"
#include "GameAPI/CVirtualMachine.h"
#include "GameAPI/CExoString.h"
#include "GameAPI/C2DA.h"

int __stdcall ExecuteCommandGet2DAString(DWORD routine, int paramCount)
{
    debugLog("[PATCH] Running Get2DAString\n");

    CVirtualMachine* vm = CVirtualMachine::GetInstance();
    if (!vm) return -2001;

    CExoString s2daName;
    if (!vm->StackPopString(&s2daName)) {
        delete vm;
        return -2001;
    }

    int row;
    if (!vm->StackPopInteger(&row)) {
        delete vm;
        return -2001;
    }

    CExoString column;
    if (!vm->StackPopString(&column)) {
        delete vm;
        return -2001;
    }

    char* nameStr = s2daName.GetCStr();
    if (!nameStr) {
        CExoString emptyResult("");
        if (!vm->StackPushString(&emptyResult)) {
            delete vm;
            return -2000;
        }
        delete vm;
        return 0;
    }

    C2DA twoDA(nameStr);
    if (!twoDA.GetPtr()) {
        CExoString emptyResult("");
        if (!vm->StackPushString(&emptyResult)) {
            delete vm;
            return -2000;
        }
        delete vm;
        return 0;
    }

    CExoString result;
    bool success = twoDA.GetCExoStringEntry(row, &column, &result);

    if (!success) {
        CExoString emptyResult("");
        if (!vm->StackPushString(&emptyResult)) {
            delete vm;
            return -2000;
        }
        delete vm;
        return 0;
    }

    if (!vm->StackPushString(&result)) {
        delete vm;
        return -2000;
    }

    delete vm;
    return 0;
}

int __stdcall ExecuteCommandGet2DAInt(DWORD routine, int paramCount)
{
    debugLog("[PATCH] Running Get2DAInt\n");

    CVirtualMachine* vm = CVirtualMachine::GetInstance();
    if (!vm) return -2001;

    CExoString s2daName;
    if (!vm->StackPopString(&s2daName)) {
        delete vm;
        return -2001;
    }

    int row;
    if (!vm->StackPopInteger(&row)) {
        delete vm;
        return -2001;
    }

    CExoString column;
    if (!vm->StackPopString(&column)) {
        delete vm;
        return -2001;
    }

    char* nameStr = s2daName.GetCStr();
    if (!nameStr) {
        if (!vm->StackPushInteger(0)) {
            delete vm;
            return -2000;
        }
        delete vm;
        return 0;
    }

    C2DA twoDA(nameStr);
    if (!twoDA.GetPtr()) {
        if (!vm->StackPushInteger(0)) {
            delete vm;
            return -2000;
        }
        delete vm;
        return 0;
    }

    int result = 0;
    bool success = twoDA.GetINTEntry(row, &column, &result);

    if (!success) {
        if (!vm->StackPushInteger(0)) {
            delete vm;
            return -2000;
        }
        delete vm;
        return 0;
    }

    if (!vm->StackPushInteger(result)) {
        delete vm;
        return -2000;
    }

    delete vm;
    return 0;
}

int __stdcall ExecuteCommandGet2DAFloat(DWORD routine, int paramCount)
{
    debugLog("[PATCH] Running Get2DAFloat\n");

    CVirtualMachine* vm = CVirtualMachine::GetInstance();
    if (!vm) return -2001;

    CExoString s2daName;
    if (!vm->StackPopString(&s2daName)) {
        delete vm;
        return -2001;
    }

    int row;
    if (!vm->StackPopInteger(&row)) {
        delete vm;
        return -2001;
    }

    CExoString column;
    if (!vm->StackPopString(&column)) {
        delete vm;
        return -2001;
    }

    char* nameStr = s2daName.GetCStr();
    if (!nameStr) {
        if (!vm->StackPushFloat(0.0f)) {
            delete vm;
            return -2000;
        }
        delete vm;
        return 0;
    }

    C2DA twoDA(nameStr);
    if (!twoDA.GetPtr()) {
        if (!vm->StackPushFloat(0.0f)) {
            delete vm;
            return -2000;
        }
        delete vm;
        return 0;
    }

    float result = 0.0f;
    bool success = twoDA.GetFLOATEntry(row, &column, &result);

    if (!success) {
        if (!vm->StackPushFloat(0.0f)) {
            delete vm;
            return -2000;
        }
        delete vm;
        return 0;
    }

    if (!vm->StackPushFloat(result)) {
        delete vm;
        return -2000;
    }

    delete vm;
    return 0;
}
