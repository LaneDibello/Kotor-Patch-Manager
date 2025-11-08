#pragma once
#include "Common.h"

// CSWSObject
// todo RE these params
typedef void(__thiscall* SWSObject_AddActionToFront)(
    void* thisPtr, DWORD param_1, USHORT param_2, DWORD param_3, void* param_4, DWORD param_5,
    void* param_6, DWORD param_7, void* param_8, DWORD param_9, void* param_10, DWORD param_11,
    void* param_12, DWORD param_13, void* param_14, DWORD param_15, void* param_16, DWORD param_17,
    void* param_18, DWORD param_19, void* param_20, DWORD param_21, void* param_22, DWORD param_23,
    void* param_24, DWORD param_25, void* param_26, DWORD param_27, void* param_28);

const DWORD SWSOBJECT_ADD_ACTION_TO_FRONT = 0x004ccb00;

extern SWSObject_AddActionToFront sWSObjectAddActionToFront;
