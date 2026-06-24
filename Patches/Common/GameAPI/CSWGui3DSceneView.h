#pragma once
#include "../Common.h"
#include "CSWGuiControl.h"

class CSWGui3DSceneView : public CSWGuiControl {
public:
	explicit CSWGui3DSceneView(void* objectPtr);
	~CSWGui3DSceneView();

	void InitializeFunctions() override;
	void InitializeOffsets() override;

protected:
	static bool functionsInitialized;
	static bool offsetsInitialized;

};
