#pragma once

#include <windows.h>

// Command indices for 2DA functions
const int Get2DAStringIndex = 789;
const int Get2DAIntIndex = 790;
const int Get2DAFloatIndex = 791;

// ExecuteCommand functions for 2DA access
int __stdcall ExecuteCommandGet2DAString(DWORD routine, int paramCount);
int __stdcall ExecuteCommandGet2DAInt(DWORD routine, int paramCount);
int __stdcall ExecuteCommandGet2DAFloat(DWORD routine, int paramCount);
