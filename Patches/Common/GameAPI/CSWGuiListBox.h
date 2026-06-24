#pragma once
#include "../Common.h"
#include "CSWGuiNavigable.h"

class CSWGuiListBox : public CSWGuiNavigable {
public:
	explicit CSWGuiListBox(void* objectPtr);
	~CSWGuiListBox();

	void InitializeFunctions() override;
	void InitializeOffsets() override;

protected:
	static bool functionsInitialized;
	static bool offsetsInitialized;

};
