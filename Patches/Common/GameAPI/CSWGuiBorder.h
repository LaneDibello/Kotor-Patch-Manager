#pragma once
#include "../Common.h"
#include "CSWGuiObject.h"

class CSWGuiBorderParams;

class CSWGuiBorder : public CSWGuiObject {
public:
    explicit CSWGuiBorder(void* objectPtr);
    CSWGuiBorder();
    ~CSWGuiBorder();

    // Accessors. Returned wrapper is heap allocated; caller owns it.
    CSWGuiBorderParams* GetBorderParams();

    // Functions
    void FillCenter(int height, int width, int x, int y, float alpha, Vector* color);
    void FillTile(int height, int width, int x, int y, float alpha, Vector* color);

    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    typedef void (__thiscall* FillCenterFn)(void* thisPtr, int height, int width, int x, int y, float alpha, Vector* color);
    typedef void (__thiscall* FillTileFn)  (void* thisPtr, int height, int width, int x, int y, float alpha, Vector* color);
    typedef void* (__thiscall* ConstructorFn)(void* thisPtr);
    typedef void* (__thiscall* DestructorFn)(void* thisPtr);

    static FillCenterFn fillCenter;
    static FillTileFn fillTile;
    static ConstructorFn constructor;
    static DestructorFn  destructor;
    static int classSize;

    static bool functionsInitialized;
    static bool offsetsInitialized;

    static int offsetBorderParams;
};
