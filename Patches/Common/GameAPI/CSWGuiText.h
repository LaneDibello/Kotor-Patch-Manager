#pragma once
#include "../Common.h"
#include "CSWGuiObject.h"

class CSWGuiText : public CSWGuiObject {
public:
	explicit CSWGuiText(void* objectPtr);
	CSWGuiText();
	~CSWGuiText();

	void InitializeFunctions() override;
	void InitializeOffsets() override;

protected:
	typedef void* (__thiscall* ConstructorFn)(void* thisPtr);
	typedef void* (__thiscall* DestructorFn)(void* thisPtr);

	static ConstructorFn constructor;
	static DestructorFn  destructor;
	static int classSize;

	static bool functionsInitialized;
	static bool offsetsInitialized;

};
