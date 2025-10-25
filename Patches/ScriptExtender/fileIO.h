#pragma once
#include <windows.h>
#include <stdio.h>
#include "VirtualMachine.h"

const int OpenFileIndex = 773;
void __stdcall ExecuteCommandOpenFile(DWORD routine, int paramCount);

const int CloseFileIndex = 774;
void __stdcall ExecuteCommandCloseFile(DWORD routine, int paramCount);

const int ReadFileIndex = 775;
void __stdcall ExecuteCommandReadTextFile(DWORD routine, int paramCount);

const int WriteFileIndex = 775;
void __stdcall ExecuteCommandWriteTextFile(DWORD routine, int paramCount);