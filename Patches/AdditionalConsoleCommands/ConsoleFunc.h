#pragma once
#include "Common.h"
#include "GameAPI/GameVersion.h"
#include "GameAPI/CVirtualMachine.h"
#include "GameAPI/CServerExoApp.h"
#include "GameAPI/CClientExoApp.h"
#include "GameAPI/CSWSCreature.h"
#include "GameAPI/CSWSObject.h"
#include "GameAPI/CExoString.h"
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
			void* address = GameVersion::GetFunctionAddress("ConsoleFunc", "IntConstructor");
			constructor constructConsoleFunc = (constructor)address;
			constructConsoleFunc(this, name, function);
			return;
		}
		case STRING_PARAM:
		{
			void* address = GameVersion::GetFunctionAddress("ConsoleFunc", "StringConstructor");
				constructor constructConsoleFunc = (constructor)address;
			constructConsoleFunc(this, name, function);
			return;
		}
		default:
		case NO_PARAMS:
		{
			void* address = GameVersion::GetFunctionAddress("ConsoleFunc", "NoParamConstructor");
				constructor constructConsoleFunc = (constructor)address;
			constructConsoleFunc(this, name, function);
			return;
		}
		}
	}

	~ConsoleFunc() {
		typedef ConsoleFunc* (__thiscall* destructor)(ConsoleFunc* thisPtr);
		void* address = GameVersion::GetFunctionAddress("ConsoleFunc", "Destructor");
		destructor destructConsoleFunc = (destructor)address;
		destructConsoleFunc(this);
	}
};

#pragma pack(pop)