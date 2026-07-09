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

	// The class's own vtable address (from the classes table). Written to
	// offset 0 after construction because the inlined constructor set the
	// CSWGuiControl vtable, not the CSWGui3DSceneView one. May be null when the
	// DB doesn't record it, in which case we leave the control vtable in place.
	static void* vtable;
};
