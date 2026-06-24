#pragma once
#include "../Common.h"
#include "CSWGuiObject.h"

class CSWGuiControl : public CSWGuiObject {
public:
	explicit CSWGuiControl(void* objectPtr);
	~CSWGuiControl();

	void InitializeFunctions() override;
	void InitializeOffsets() override;

protected:
	static bool functionsInitialized;
	static bool offsetsInitialized;

};
