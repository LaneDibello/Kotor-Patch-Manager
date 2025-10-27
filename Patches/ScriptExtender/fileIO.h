#pragma once
#include "Kotor1Functions.h"

const int OpenFileIndex = 773;
void __stdcall ExecuteCommandOpenFile(DWORD routine, int paramCount);

const int CloseFileIndex = 774;
void __stdcall ExecuteCommandCloseFile(DWORD routine, int paramCount);

const int ReadFileIndex = 775;
void __stdcall ExecuteCommandReadTextFile(DWORD routine, int paramCount);

const int WriteFileIndex = 776;
void __stdcall ExecuteCommandWriteTextFile(DWORD routine, int paramCount);

const int PeakCharFileIndex = 777;
void __stdcall ExecuteCommandPeakCharFile(DWORD routine, int paramCount);

const int SeekFileIndex = 778;
void __stdcall ExecuteCommandSeekFile(DWORD routine, int paramCount);

const int TellFileIndex = 779;
void __stdcall ExecuteCommandTellFile(DWORD routine, int paramCount);