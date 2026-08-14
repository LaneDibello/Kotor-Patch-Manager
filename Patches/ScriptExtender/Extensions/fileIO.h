#pragma once
#include "Common.h"
#include "GameAPI/CVirtualMachine.h"
#include "GameAPI/CExoString.h"

const int OpenFileIndex = 773;
VirtualMachineReturnTypes __stdcall ExecuteCommandOpenFile(DWORD routine, int paramCount);

const int CloseFileIndex = 774;
VirtualMachineReturnTypes __stdcall ExecuteCommandCloseFile(DWORD routine, int paramCount);

const int ReadFileIndex = 775;
VirtualMachineReturnTypes __stdcall ExecuteCommandReadTextFile(DWORD routine, int paramCount);

const int WriteFileIndex = 776;
VirtualMachineReturnTypes __stdcall ExecuteCommandWriteTextFile(DWORD routine, int paramCount);

const int PeakCharFileIndex = 777;
VirtualMachineReturnTypes __stdcall ExecuteCommandPeakCharFile(DWORD routine, int paramCount);

const int SeekFileIndex = 778;
VirtualMachineReturnTypes __stdcall ExecuteCommandSeekFile(DWORD routine, int paramCount);

const int TellFileIndex = 779;
VirtualMachineReturnTypes __stdcall ExecuteCommandTellFile(DWORD routine, int paramCount);