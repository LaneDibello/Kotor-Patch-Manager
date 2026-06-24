#pragma once
#include "../Common.h"
#include "CSWGuiObject.h"

class CSWGuiPanel : public CSWGuiObject {
public:
	explicit CSWGuiPanel(void* objectPtr);
	~CSWGuiPanel();

	void InitializeFunctions() override;
	void InitializeOffsets() override;

protected:
	static bool functionsInitialized;
	static bool offsetsInitialized;

};