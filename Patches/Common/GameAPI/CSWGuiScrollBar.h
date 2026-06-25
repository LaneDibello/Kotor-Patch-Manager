#pragma once
#include "../Common.h"
#include "CSWGuiControl.h"

class CSWGuiBorder;
class CSWGuiImage;

class CSWGuiScrollBar : public CSWGuiControl {
public:
	explicit CSWGuiScrollBar(void* objectPtr);
	CSWGuiScrollBar();
	~CSWGuiScrollBar();

	// Accessors. Returned wrappers are heap allocated; caller owns them.
	int GetMaxValue();
	int GetCurrentValue();
	int GetVisibleItemCount();
	CSWGuiBorder* GetBorder();
	CSWGuiImage* GetImage1();
	CSWGuiImage* GetImage2();

	// Functions
	int HitCheckScrollbar(int mouseX, int mouseY);
	void setCurValue(int value);
	void setMaxValue(int value, BYTE _unknown);
	void setVisibleValue(int value);

	void InitializeFunctions() override;
	void InitializeOffsets() override;

protected:
	typedef void* (__thiscall* ConstructorFn)(void* thisPtr);
	typedef void* (__thiscall* DestructorFn)(void* thisPtr);
	typedef int  (__thiscall* HitCheckScrollbarFn)(void* thisPtr, int mouseX, int mouseY);
	typedef void (__thiscall* SetCurValueFn)(void* thisPtr, int value);
	typedef void (__thiscall* SetMaxValueFn)(void* thisPtr, int value, BYTE _unknown);
	typedef void (__thiscall* SetVisibleValueFn)(void* thisPtr, int value);

	static ConstructorFn constructor;
	static DestructorFn  destructor;
	static HitCheckScrollbarFn hitCheckScrollbar;
	static SetCurValueFn       setCurValueFn;
	static SetMaxValueFn       setMaxValueFn;
	static SetVisibleValueFn   setVisibleValueFn;
	static int classSize;

	static bool functionsInitialized;
	static bool offsetsInitialized;

	static int offsetMaxValue;
	static int offsetCurrentValue;
	static int offsetVisibleItemCount;
	static int offsetBorder;
	static int offsetImage1;
	static int offsetImage2;
};
