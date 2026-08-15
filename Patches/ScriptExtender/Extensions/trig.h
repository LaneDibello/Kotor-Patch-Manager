#pragma once
#include "Common.h"
#include "GameAPI/CVirtualMachine.h"
#define _USE_MATH_DEFINES
#include <math.h>

const int secIndex = 794;
const int cscIndex = 795;
const int cotIndex = 796;
VirtualMachineReturnTypes __stdcall ExecuteCommandTrig(DWORD routine, int paramCount);

const int RadToDegIndex = 797;
VirtualMachineReturnTypes __stdcall ExecuteCommandRadToDeg(DWORD routine, int paramCount);

const int DegToRadIndex = 798;
VirtualMachineReturnTypes __stdcall ExecuteCommandDegToRad(DWORD routine, int paramCount);