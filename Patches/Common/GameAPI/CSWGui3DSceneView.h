#pragma once
#include "../Common.h"
#include "CSWGuiControl.h"

class CSWGuiScene;

class CSWGui3DSceneView : public CSWGuiControl {
public:
	explicit CSWGui3DSceneView(void* objectPtr);
	// Allocates and constructs a new CSWGui3DSceneView, freeing it on destruction.
	CSWGui3DSceneView();
	~CSWGui3DSceneView();

	// Accessors
	CSWGuiScene* GetScene();

	void InitializeFunctions() override;
	void InitializeOffsets() override;

protected:
	// CSWGui3DSceneView has no dedicated constructor in the binary: the compiler
	// inlined it to a direct call of the CSWGuiControl constructor (only the class
	// size differs). The default constructor below reuses CSWGuiControl::constructor.
	// It does, however, have its own destructor (which tears down the embedded scene).
	typedef void* (__thiscall* DestructorFn)(void* thisPtr);
	static DestructorFn destructor;

	static bool functionsInitialized;
	static bool offsetsInitialized;

	static int offsetScene;
	static int classSize;
};
