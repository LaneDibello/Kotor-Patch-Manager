#pragma once
#include "../Common.h"
#include "CSWGuiControl.h"

class CSWGuiLabel : public CSWGuiControl {
public:
	explicit CSWGuiLabel(void* objectPtr);
	~CSWGuiLabel();

	void InitializeFunctions() override;
	void InitializeOffsets() override;

protected:
	static bool functionsInitialized;
	static bool offsetsInitialized;

};
