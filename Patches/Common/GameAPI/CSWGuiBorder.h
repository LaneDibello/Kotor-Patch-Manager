#pragma once
#include "../Common.h"
#include "CSWGuiObject.h"

class CResGFF;
struct CResStruct;
class CExoString;

class CSWGuiBorderParams;

class CSWGuiBorder : public CSWGuiObject {
public:
    explicit CSWGuiBorder(void* objectPtr);
    CSWGuiBorder();
    ~CSWGuiBorder();

    // Accessors. Returned wrapper is heap allocated; caller owns it.
    CSWGuiBorderParams* GetBorderParams();

    // Functions
    void Initialize(CSWGuiExtent* extent, CSWGuiBorderParams* borderParams);
    void GetInnerExtent(CSWGuiExtent* outExtent);
    void Draw(float alpha);
    void Load(CResGFF* gff, CResStruct* item, CExoString* label);
    void FillCenter(int height, int width, int x, int y, float alpha, Vector* color);
    void FillTile(int height, int width, int x, int y, float alpha, Vector* color);

    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    typedef void (__thiscall* FillCenterFn)(void* thisPtr, int height, int width, int x, int y, float alpha, Vector* color);
    typedef void (__thiscall* FillTileFn)  (void* thisPtr, int height, int width, int x, int y, float alpha, Vector* color);
    typedef void (__thiscall* InitializeFn)(void* thisPtr, void* extent, void* borderParams);
    typedef void (__thiscall* GetInnerExtentFn)(void* thisPtr, void* outExtent);
    typedef void (__thiscall* DrawFn)(void* thisPtr, float alpha);
    typedef void (__thiscall* LoadFn)(void* thisPtr, void* gff, void* item, void* label);
    typedef void* (__thiscall* ConstructorFn)(void* thisPtr);
    typedef void* (__thiscall* DestructorFn)(void* thisPtr);

    static FillCenterFn fillCenter;
    static FillTileFn fillTile;
    static InitializeFn     initialize;
    static GetInnerExtentFn getInnerExtent;
    static DrawFn           drawFn;
    static LoadFn           loadFn;
    static ConstructorFn constructor;
    static DestructorFn  destructor;
    static int classSize;

    static bool functionsInitialized;
    static bool offsetsInitialized;

    static int offsetBorderParams;
};
