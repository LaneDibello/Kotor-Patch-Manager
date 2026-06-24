#pragma once
#include "../Common.h"
#include "CSWGuiObject.h"

class CSWGuiImage : public CSWGuiObject {
public:
	explicit CSWGuiImage(void* objectPtr);
	~CSWGuiImage();

	void InitializeFunctions() override;
	void InitializeOffsets() override;

protected:
	static bool functionsInitialized;
	static bool offsetsInitialized;

};
