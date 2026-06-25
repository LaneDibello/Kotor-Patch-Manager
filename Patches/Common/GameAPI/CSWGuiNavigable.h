#pragma once
#include "../Common.h"
#include "CSWGuiControl.h"

// Direction indices used for keyboard/controller navigation. A CSWGuiNavigable
// stores the index of the control that should be navigated to in each of these
// four directions.
enum CSWGuiNavDirection {
	NavUp    = 0,
	NavLeft  = 1,
	NavDown  = 2,
	NavRight = 3
};

class CSWGuiNavigable : public CSWGuiControl {
public:
	explicit CSWGuiNavigable(void* objectPtr);
	CSWGuiNavigable();
	~CSWGuiNavigable();

	// Engine routines: get/set the control index navigated to in a direction.
	int GetMoveToControl(CSWGuiNavDirection direction);
	void SetMoveToControl(CSWGuiNavDirection direction, int controlIndex);

	// Direct read/write of the backing up/left/down/right member fields
	// (bypasses the engine routines above).
	int GetMoveToControlIndex(CSWGuiNavDirection direction);
	void SetMoveToControlIndex(CSWGuiNavDirection direction, int controlIndex);

	void InitializeFunctions() override;
	void InitializeOffsets() override;

protected:
	typedef void* (__thiscall* ConstructorFn)(void* thisPtr);
	typedef void* (__thiscall* DestructorFn)(void* thisPtr);
	typedef int  (__thiscall* GetMoveToControlFn)(void* thisPtr, int direction);
	typedef void (__thiscall* SetMoveToControlFn)(void* thisPtr, int direction, int controlIndex);

	// Returns the backing field offset for a direction, or -1 if unavailable.
	int offsetForDirection(CSWGuiNavDirection direction);

	static ConstructorFn constructor;
	static DestructorFn  destructor;
	static GetMoveToControlFn getMoveToControl;
	static SetMoveToControlFn setMoveToControl;
	static int classSize;

	static bool functionsInitialized;
	static bool offsetsInitialized;

	static int offsetUp;
	static int offsetLeft;
	static int offsetDown;
	static int offsetRight;
};
