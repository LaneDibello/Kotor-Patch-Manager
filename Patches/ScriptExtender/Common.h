#pragma once
#include <windows.h>
#include <cstdio>
#include <cstdarg>
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

// Debug logging helper - automatically formats and outputs to debugger
inline void DebugLog(const char* format, ...) {
	char buffer[512];
	va_list args;
	va_start(args, format);
	vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, format, args);
	va_end(args);
	OutputDebugStringA(buffer);
}