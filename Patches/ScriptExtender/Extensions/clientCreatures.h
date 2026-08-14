#pragma once
#include "Common.h"
#include "VirtualFunctionCall.h"
#include "GameAPI/CVirtualMachine.h"
#include "GameAPI/CServerExoApp.h"
#include "GameAPI/CSWSCreature.h"
#include "GameAPI/CSWCCreature.h"


const int IsRunningIndex = 787;
int __stdcall ExecuteCommandIsRunning(DWORD routine, int paramCount);

const int IsStealthedIndex = 788;
int __stdcall ExecuteCommandIsStealthed(DWORD routine, int paramCount);