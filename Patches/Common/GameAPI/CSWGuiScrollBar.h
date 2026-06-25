#pragma once
#include "../Common.h"
#include "CSWGuiControl.h"

class CSWGuiScrollBar : public CSWGuiControl {
public:
	explicit CSWGuiScrollBar(void* objectPtr);
	CSWGuiScrollBar();
	~CSWGuiScrollBar();

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
