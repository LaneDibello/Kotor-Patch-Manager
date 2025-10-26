#pragma once
#include <windows.h>
#include <cstdio>
#pragma pack(push, 4)

struct CExoString {
	char* c_string;
	DWORD length;

	CExoString() {
		typedef CExoString* (__thiscall* constructor)(CExoString* thisPtr);
		const DWORD ADDRESS = 0x005b3190;
		constructor constructCExoString = (constructor)ADDRESS;

		constructCExoString(this);
	}

	CExoString(char* src, int length) {
		typedef CExoString* (__thiscall* constructor)(CExoString* thisPtr, char* source, int length);
		const DWORD ADDRESS = 0x005e5b70;
		constructor constructCExoStringFromCStr = (constructor)ADDRESS;

		constructCExoStringFromCStr(this, src, length);
	}

	~CExoString() {
		typedef CExoString* (__thiscall* destructor)(CExoString* thisPtr);
		const DWORD ADDRESS = 0x005e5c20;
		destructor destructCExoString = (destructor)ADDRESS;

		destructCExoString(this);
	}
};

struct Vector {
	float x;
	float y;
	float z;
};


#pragma pack(pop)