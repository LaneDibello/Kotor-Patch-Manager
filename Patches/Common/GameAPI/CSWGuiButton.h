#pragma once
#include "../Common.h"
#include "CSWGuiNavigable.h"

class CSWGuiButton : public CSWGuiNavigable {
public:
	explicit CSWGuiButton(void* objectPtr);
	~CSWGuiButton();

	void InitializeFunctions() override;
	void InitializeOffsets() override;

protected:
	static bool functionsInitialized;
	static bool offsetsInitialized;

};
