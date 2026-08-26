#pragma once
#include "../Common.h"
#include "CSWGuiNavigable.h"

class CSWGuiBorder;
class CSWGuiImage;

class CSWGuiSlider : public CSWGuiNavigable {
public:
	explicit CSWGuiSlider(void* objectPtr);
	~CSWGuiSlider();

	// Accessors. Returned wrapper is heap allocated; caller owns it.
	CSWGuiBorder* GetBorder1();
	CSWGuiBorder* GetBorder2();
	CSWGuiImage* GetImage();
	int GetMaxValue();
	int GetCurValue();

	void InitializeFunctions() override;
	void InitializeOffsets() override;

protected:
	static bool functionsInitialized;
	static bool offsetsInitialized;

	static int offsetMaxValue;
	static int offsetCurValue;
	static int offsetBorder1;
	static int offsetBorder2;
	static int offsetImage;
};
