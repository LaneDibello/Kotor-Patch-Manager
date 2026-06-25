#pragma once
#include "../Common.h"
#include "CSWGuiControl.h"

class CSWGuiProgressBar : public CSWGuiControl {
public:
	explicit CSWGuiProgressBar(void* objectPtr);
	CSWGuiProgressBar();
	~CSWGuiProgressBar();

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
