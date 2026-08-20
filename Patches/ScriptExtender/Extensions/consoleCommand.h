#pragma once
#include "Common.h"
#include "GameAPI/CVirtualMachine.h"

const int RunConsoleCommandIndex = 799;
VirtualMachineReturnTypes __stdcall ExecuteCommandRunConsoleCommand(DWORD routine, int paramCount);