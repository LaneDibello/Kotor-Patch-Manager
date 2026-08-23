#pragma once
#include "Common.h"
#include "GameAPI/CVirtualMachine.h"
#include "GameAPI/CExoIni.h"
#include "GameAPI/CExoString.h"

const int ReadIniEntryIndex = 801;
VirtualMachineReturnTypes __stdcall ExecuteCommandReadIniEntry(DWORD routine, int paramCount);

const int WriteIniEntryIndex = 802;
VirtualMachineReturnTypes __stdcall ExecuteCommandWriteIniEntry(DWORD routine, int paramCount);

