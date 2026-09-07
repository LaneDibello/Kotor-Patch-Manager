#pragma once
#include "../Common.h"
#include "CSWGuiObject.h"

class CResGFF;
struct CResStruct;
class CExoString;

class CSWGuiTextParams;
struct CSWGuiExtent;

class CSWGuiText : public CSWGuiObject {
public:
	explicit CSWGuiText(void* objectPtr);
	CSWGuiText();
	~CSWGuiText();

	// Accessors. Returned wrapper is heap allocated; caller owns it.
	CSWGuiTextParams* GetTextParams();

	// Functions
	int GetFontHeight();
	int GetIdealHeight();
	void Initialize(CSWGuiExtent* extent, CSWGuiTextParams* textParams, float scale);
	void SetExtent(CSWGuiExtent* extent);
	void Draw(float alpha);
	void Load(CResGFF* gff, CResStruct* item, CExoString* label);
	void wrapText();

	void InitializeFunctions() override;
	void InitializeOffsets() override;

protected:
	typedef void* (__thiscall* ConstructorFn)(void* thisPtr);
	typedef void* (__thiscall* DestructorFn)(void* thisPtr);
	typedef int  (__thiscall* GetFontHeightFn)(void* thisPtr);
	typedef int  (__thiscall* GetIdealHeightFn)(void* thisPtr);
	typedef void (__thiscall* InitializeFn)(void* thisPtr, void* extent, void* textParams, float scale);
	typedef void (__thiscall* SetExtentFn)(void* thisPtr, void* extent);
	typedef void (__thiscall* DrawFn)(void* thisPtr, float alpha);
	typedef void (__thiscall* LoadFn)(void* thisPtr, void* gff, void* item, void* label);
	typedef void (__thiscall* WrapTextFn)(void* thisPtr);

	static ConstructorFn constructor;
	static DestructorFn  destructor;
	static GetFontHeightFn  getFontHeight;
	static GetIdealHeightFn getIdealHeight;
	static InitializeFn     initialize;
	static SetExtentFn      setExtent;
	static DrawFn           drawFn;
	static LoadFn loadFn;
	static WrapTextFn       wrapTextFn;
	static int classSize;

	static bool functionsInitialized;
	static bool offsetsInitialized;

	static int offsetTextParams;
};
