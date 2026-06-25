#pragma once
#include "../Common.h"
#include "CSWGuiControl.h"

class CSWGuiProgressBar : public CSWGuiControl {
public:
	explicit CSWGuiProgressBar(void* objectPtr);
	~CSWGuiProgressBar();

	void InitializeFunctions() override;
	void InitializeOffsets() override;

protected:
	static bool functionsInitialized;
	static bool offsetsInitialized;

};
