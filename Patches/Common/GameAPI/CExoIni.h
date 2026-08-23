#pragma once
#include "../Common.h"
#include "GameAPIObject.h"
#include "GameVersion.h"

class CExoString;

class CExoIni : public GameAPIObject {
public:
	explicit CExoIni(void* objectPtr);
	CExoInit();
	~CExoInit();

	int ReadIniEntry(CExoString* outValue, CExoString* filename, CExoString* category, CExoString* key);
	int WriteIniEntry(CExoString* value, CExoString* filename, CExoString* category, CExoString* key);

	void InitializeFunctions() override;
	void InitializeOffsets() override;

private:
	typedef void* (__thiscall* ConstructorFn)(void* thisPtr);
	typedef void* (__thiscall* DestructorFn)(void* thisPtr);
	typedef int(__thiscall* ReadIniEntryFn)(void* thisPtr, void* value, void* filename, void* category, void* key);
	typedef int(__thiscall* WriteIniEntryFn)(void* thisPtr, void* value, void* filename, void* category, void* key);

	static ConstructorFn constructor;
	static DestructorFn  destructor;
	static ReadIniEntryFn readIniEntry;
	static WriteIniEntryFn writeIniEntry;

	static bool functionsInitialized;
	static bool offsetsInitialized;

	static int classSize;
};