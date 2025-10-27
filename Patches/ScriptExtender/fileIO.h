#pragma once
#include "Kotor1Functions.h"

const int OpenFileIndex = 773;
int __stdcall ExecuteCommandOpenFile(DWORD routine, int paramCount);

const int CloseFileIndex = 774;
int __stdcall ExecuteCommandCloseFile(DWORD routine, int paramCount);

const int ReadFileIndex = 775;
int __stdcall ExecuteCommandReadTextFile(DWORD routine, int paramCount);

const int WriteFileIndex = 776;
int __stdcall ExecuteCommandWriteTextFile(DWORD routine, int paramCount);

const int PeakCharFileIndex = 777;
int __stdcall ExecuteCommandPeakCharFile(DWORD routine, int paramCount);

const int SeekFileIndex = 778;
int __stdcall ExecuteCommandSeekFile(DWORD routine, int paramCount);

const int TellFileIndex = 779;
int __stdcall ExecuteCommandTellFile(DWORD routine, int paramCount);