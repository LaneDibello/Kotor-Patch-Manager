#pragma once
#include "Common.h"
#include "VirtualFunctionCall.h"
#include "GameAPI/CVirtualMachine.h"
#include "GameAPI/CServerExoApp.h"

const int IncrementGlobalNumberIndex = 792;
int __stdcall ExecuteCommandIncrementGlobalNumber(DWORD routine, int paramCount);

const int DecrementGlobalNumberIndex = 793;
int __stdcall ExecuteCommandDecrementGlobalNumber(DWORD routine, int paramCount);