#pragma once
#include "../Common.h"
#include "CSWGuiNavigable.h"

class CSWGuiBorder;
class CSWGuiBorderParams;
class CSWGuiImage;
class CResRef;
struct CSWGuiExtent;

class CSWGuiSlider : public CSWGuiNavigable {
public:
	explicit CSWGuiSlider(void* objectPtr);
	~CSWGuiSlider();

	// Accessors. Returned wrapper is heap allocated; caller owns it.
	CSWGuiBorder* GetBorder();
	CSWGuiBorder* GetBorderHilight();
	CSWGuiImage* GetImage();
	int GetMaxValue();
	int GetCurValue();

	// Functions
	void Initialize(CSWGuiExtent* extent, CSWGuiBorderParams* borderParams,
	                CSWGuiBorderParams* borderHilightParams, CResRef* imageResRef);
	void SetExtent(CSWGuiExtent* extent);

	void InitializeFunctions() override;
	void InitializeOffsets() override;

protected:
	typedef void (__thiscall* InitializeFn)(void* thisPtr, void* extent, void* borderParams,
	                                        void* borderHilightParams, void* imageResRef);
	typedef void (__thiscall* SetExtentFn)(void* thisPtr, void* extent);

	static InitializeFn initialize;
	static SetExtentFn setExtent;

	static bool functionsInitialized;
	static bool offsetsInitialized;

	static int offsetMaxValue;
	static int offsetCurValue;
	static int offsetBorder;
	static int offsetBorderHilight;
	static int offsetImage;
};
