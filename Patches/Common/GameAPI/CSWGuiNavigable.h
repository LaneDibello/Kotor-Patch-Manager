#pragma once
#include "../Common.h"
#include "CSWGuiControl.h"

class CSWGuiNavigable : public CSWGuiControl {
public:
	explicit CSWGuiNavigable(void* objectPtr);
	CSWGuiNavigable();
	~CSWGuiNavigable();

	void InitializeFunctions() override;
	void InitializeOffsets() override;

protected:
	typedef void* (__thiscall* ConstructorFn)(void* thisPtr);
	typedef void* (__thiscall* DestructorFn)(void* thisPtr);

	static ConstructorFn constructor;
	static DestructorFn  destructor;
	static int classSize;

	static bool functionsInitialized;
	static bool offsetsInitialized;

};
