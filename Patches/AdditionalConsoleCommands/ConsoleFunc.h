#pragma once
#include "Common.h"
#pragma pack(push, 4)

enum funcTypes {
	NO_PARAMS,
	INT_PARAM,
	STRING_PARAM,
};

struct ConsoleFunc {
	char name[80];
	void* funcholder;

	ConsoleFunc(char* name, void* function, funcTypes type) {
		typedef ConsoleFunc* (__thiscall* constructor)(ConsoleFunc* thisPtr, char* name, void* function);

		debugLog("Constructing ConsoleFunc %s (at %p), with function at %p, with type %i at %p", name, name, function, type, this);

		switch (type) {
		case INT_PARAM:
		{
			const DWORD ADDRESS = 0x0044c620;
			constructor constructConsoleFunc = (constructor)ADDRESS;
			constructConsoleFunc(this, name, function);
			return;
		}
		case STRING_PARAM:
		{
			const DWORD ADDRESS = 0x0044c5c0;
			constructor constructConsoleFunc = (constructor)ADDRESS;
			constructConsoleFunc(this, name, function);
			return;
		}
		default:
		case NO_PARAMS:
		{
			const DWORD ADDRESS = 0x0044c560;
			constructor constructConsoleFunc = (constructor)ADDRESS;
			constructConsoleFunc(this, name, function);
			return;
		}
		}
	}

	~ConsoleFunc() {
		typedef ConsoleFunc* (__thiscall* destructor)(ConsoleFunc* thisPtr);
		const DWORD ADDRESS = 0x0044bcf0;
		destructor destructConsoleFunc = (destructor)ADDRESS;
		destructConsoleFunc(this);
	}
};

#pragma pack(pop)