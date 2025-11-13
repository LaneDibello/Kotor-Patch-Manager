#pragma once
#include <windows.h>
#include <cstdio>
#include <cstdarg>
#include "VirtualFunctionCall.h"
#pragma pack(push, 4)

extern DWORD OBJECT_DEFAULT;

struct Vector {
	float x;
	float y;
	float z;
};

struct CScriptLocation {
	Vector postion;
	Vector orientation;
};

enum VirtualMachineEngineStructureTypes : int {
	UNKNOWN = -1,
	EFFECT = 0,
	SCRIPT_EVENT = 1,
	LOCATION = 2,
	SCRIPT_TALENT = 3,
};
#pragma pack(pop)

inline void debugLog(const char* format, ...) {
	char buffer[512];
	va_list args;
	va_start(args, format);
	vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, format, args);
	va_end(args);
	OutputDebugStringA(buffer);
}

template <class retType>
inline retType getObjectProperty(void* object, int offset) {
	return *((retType*)((char*)object + offset));
}

template <class propType>
inline void setObjectProperty(void* object, int offset, propType value) {
	*((propType*)((char*)object + offset)) = value;
}