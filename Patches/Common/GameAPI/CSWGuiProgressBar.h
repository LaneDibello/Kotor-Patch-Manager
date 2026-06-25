#pragma once
#include "../Common.h"
#include "CSWGuiControl.h"

class CSWGuiBorder;

class CSWGuiProgressBar : public CSWGuiControl {
public:
	explicit CSWGuiProgressBar(void* objectPtr);
	CSWGuiProgressBar();
	~CSWGuiProgressBar();

	// Accessors. Returned wrappers are heap allocated; caller owns them.
	int GetMax();
	CSWGuiBorder* GetBorder1();
	CSWGuiBorder* GetBorder2();

	// Functions
	void SetCurValue(int value);
	void SetMaxValue(int value);
	void SetStartFromLeft(UINT startFromLeft);

	void InitializeFunctions() override;
	void InitializeOffsets() override;

protected:
	typedef void* (__thiscall* ConstructorFn)(void* thisPtr);
	typedef void* (__thiscall* DestructorFn)(void* thisPtr);
	typedef void (__thiscall* SetCurValueFn)(void* thisPtr, int value);
	typedef void (__thiscall* SetMaxValueFn)(void* thisPtr, int value);
	typedef void (__thiscall* SetStartFromLeftFn)(void* thisPtr, UINT startFromLeft);

	static ConstructorFn constructor;
	static DestructorFn  destructor;
	static SetCurValueFn      setCurValue;
	static SetMaxValueFn      setMaxValue;
	static SetStartFromLeftFn setStartFromLeft;
	static int classSize;

	static bool functionsInitialized;
	static bool offsetsInitialized;

	static int offsetMax;
	static int offsetBorder1;
	static int offsetBorder2;
};
