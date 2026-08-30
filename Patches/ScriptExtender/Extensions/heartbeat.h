#pragma once
#include "Common.h"
#include "GameAPI/CVirtualMachine.h"
#include "GameAPI/CServerExoApp.h"
#include "GameAPI/CGameObject.h"


const int ForceHeartbeatIndex = 803;
VirtualMachineReturnTypes __stdcall ExecuteForceHeartbeat(DWORD routine, int paramCount);

/*
Something like this for the timestamp logic:
where (D, M) is the result of CWorldTimer::GetWorldTime
```
offset = 7000                       // > 6000, gives headroom
if (M > offset):
    field43_0x348 = D
    field44_0x34c = M - offset      // non-zero, in the past
else:                               // borrow a day
    field43_0x348 = D - 1
    field44_0x34c = M - offset + day_length_milliseconds   // large, non-zero
```
*/