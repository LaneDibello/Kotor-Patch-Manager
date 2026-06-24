#pragma once
#include "../Common.h"
#include "CSWGuiNavigable.h"

class CSWGuiEditBox : public CSWGuiNavigable {
public:
	explicit CSWGuiEditBox(void* objectPtr);
	~CSWGuiEditBox();

	void InitializeFunctions() override;
	void InitializeOffsets() override;

protected:
	static bool functionsInitialized;
	static bool offsetsInitialized;

};
