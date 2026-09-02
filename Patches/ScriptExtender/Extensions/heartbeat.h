#pragma once
#include "Common.h"
#include "GameAPI/CVirtualMachine.h"
#include "GameAPI/CServerExoApp.h"
#include "GameAPI/CGameObject.h"
#include "GameAPI/CSWSAreaOfEffectObject.h"
#include "GameAPI/CSWSCreature.h"
#include "GameAPI/CSWSDoor.h"
#include "GameAPI/CSWSEncounter.h"
#include "GameAPI/CSWSPlaceable.h"
#include "GameAPI/CSWSTrigger.h"

const int ForceHeartbeatIndex = 803;
VirtualMachineReturnTypes __stdcall ExecuteCommandForceHeartbeat(DWORD routine, int paramCount);
