#pragma once
#include "Common.h"
#include "Kotor1Functions.h"
#include "VirtualFunctionCall.h"


const int IsRunningIndex = 787;
int __stdcall ExecuteCommandIsRunning(DWORD routine, int paramCount);

const int IsStealthedIndex = 788;
int __stdcall ExecuteCommandIsStealthed(DWORD routine, int paramCount);