#pragma once
#include "Common.h"
#include "GameAPI/CVirtualMachine.h"
#include "GameAPI/CServerExoApp.h"
#include "GameAPI/CClientExoApp.h"

const int GetPlayerLastTargetObjectIndex = 800;
VirtualMachineReturnTypes __stdcall ExecuteCommandetGetPlayerLastTarget(DWORD routine, int paramCount);