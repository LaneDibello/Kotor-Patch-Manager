#pragma once
#include <windows.h>
#include <cstdio>
#pragma pack(push, 4)

struct CExoString {
	char* c_string;
	DWORD length;
};

struct Vector {
	float x;
	float y;
	float z;
};


typedef CExoString*(__thiscall* CExoStringFromCstr)(CExoString* thisPtr, char* source, int length);
typedef CExoString* (__thiscall* DestructCExoString)(CExoString* thisPtr);

const DWORD CEXOSTRING_CONSTRUCTOR_FROM_CSTR = 0x005e5b70;
const DWORD CEXOSTRING_DESTRUCTOR = 0x005e5c20;

CExoStringFromCstr constructCExoStringFromCStr = (CExoStringFromCstr)CEXOSTRING_CONSTRUCTOR_FROM_CSTR;
DestructCExoString destructCExoString = (DestructCExoString)CEXOSTRING_DESTRUCTOR;

#pragma pack(pop)