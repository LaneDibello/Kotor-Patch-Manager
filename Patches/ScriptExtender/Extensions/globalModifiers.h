#pragma once
#include "Common.h"
#include "VirtualFunctionCall.h"
#include "GameAPI/CVirtualMachine.h"
#include "GameAPI/CServerExoApp.h"

const int IncrementGlobalNumberIndex = 792;
const int DecrementGlobalNumberIndex = 793;
VirtualMachineReturnTypes __stdcall ExecuteCommandAdjustGlobalNumber(DWORD routine, int paramCount);
