#pragma once
#include "../Common.h"
#include "CSWGuiControl.h"

class CSWGuiScrollBar : public CSWGuiControl {
public:
	explicit CSWGuiScrollBar(void* objectPtr);
	~CSWGuiScrollBar();

	void InitializeFunctions() override;
	void InitializeOffsets() override;

protected:
	static bool functionsInitialized;
	static bool offsetsInitialized;

};
