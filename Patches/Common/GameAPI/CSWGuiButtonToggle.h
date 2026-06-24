#pragma once
#include "../Common.h"
#include "CSWGuiButton.h"

class CSWGuiButtonToggle : public CSWGuiButton {
public:
	explicit CSWGuiButtonToggle(void* objectPtr);
	~CSWGuiButtonToggle();

	void InitializeFunctions() override;
	void InitializeOffsets() override;

protected:
	static bool functionsInitialized;
	static bool offsetsInitialized;

};
