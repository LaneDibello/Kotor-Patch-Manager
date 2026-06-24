#pragma once
#include "../Common.h"
#include "CSWGuiObject.h"

class CSWGuiBorder : public CSWGuiObject {
public:
	explicit CSWGuiBorder(void* objectPtr);
	~CSWGuiBorder();

	void InitializeFunctions() override;
	void InitializeOffsets() override;

protected:
	static bool functionsInitialized;
	static bool offsetsInitialized;

};
