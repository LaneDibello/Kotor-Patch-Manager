#pragma once
#include "../Common.h"
#include "CSWGuiObject.h"

class CSWGuiText : public CSWGuiObject {
public:
	explicit CSWGuiText(void* objectPtr);
	~CSWGuiText();

	void InitializeFunctions() override;
	void InitializeOffsets() override;

protected:
	static bool functionsInitialized;
	static bool offsetsInitialized;

};
