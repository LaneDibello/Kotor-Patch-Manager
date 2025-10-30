#pragma once
#include <windows.h>
#include <cstdio>
#include <cstdarg>
#pragma pack(push, 4)

extern void** VIRTUAL_MACHINE_PTR;
extern DWORD OBJECT_DEFAULT;

typedef struct {
	void* vtable;
	void* client;
	void* server;
	// Note: there are additional properties being excluded
} CAppManager;

extern CAppManager** APP_MANAGER_PTR;

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

	CExoString(char* src) {
		typedef CExoString* (__thiscall* constructor)(CExoString* thisPtr, char* source);
		const DWORD ADDRESS = 0x005e5a90;
		constructor constructCExoStringFromCStr = (constructor)ADDRESS;

		constructCExoStringFromCStr(this, src);
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

inline void* getServerExoApp() {
	CAppManager* appManager = *APP_MANAGER_PTR;
	return appManager->server;
}

inline void* getClientExoApp() {
	CAppManager* appManager = *APP_MANAGER_PTR;
	return appManager->client;
}

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