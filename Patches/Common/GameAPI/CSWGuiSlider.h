#pragma once
#include "../Common.h"
#include "CSWGuiNavigable.h"

class CSWGuiSlider : public CSWGuiNavigable {
public:
	explicit CSWGuiSlider(void* objectPtr);
	~CSWGuiSlider();

	void InitializeFunctions() override;
	void InitializeOffsets() override;

protected:
	static bool functionsInitialized;
	static bool offsetsInitialized;

};
