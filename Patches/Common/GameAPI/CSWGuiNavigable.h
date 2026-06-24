#pragma once
#include "../Common.h"
#include "CSWGuiControl.h"

class CSWGuiNavigable : public CSWGuiControl {
public:
	explicit CSWGuiNavigable(void* objectPtr);
	~CSWGuiNavigable();

	void InitializeFunctions() override;
	void InitializeOffsets() override;

protected:
	static bool functionsInitialized;
	static bool offsetsInitialized;

};
