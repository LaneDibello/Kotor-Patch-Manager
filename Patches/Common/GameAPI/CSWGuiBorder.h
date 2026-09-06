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
    // Lays the border out over `extent` and copies `borderParams` into it, loading
    // the corner/edge/fill images. Cheap enough to re-run on a relayout.
    void Initialize(CSWGuiExtent* extent, CSWGuiBorderParams* borderParams);
    // The area inside the border art -- what the game passes to CSWGuiText::Initialize
    // when it lays text inside a bordered control.
    void GetInnerExtent(CSWGuiExtent* outExtent);
    void Draw(float alpha);
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
    typedef void* (__thiscall* ConstructorFn)(void* thisPtr);
    typedef void* (__thiscall* DestructorFn)(void* thisPtr);

    static FillCenterFn fillCenter;
    static FillTileFn fillTile;
    static InitializeFn     initialize;
    static GetInnerExtentFn getInnerExtent;
    static DrawFn           drawFn;
    static ConstructorFn constructor;
    static DestructorFn  destructor;
    static int classSize;

    static bool functionsInitialized;
    static bool offsetsInitialized;

    static int offsetBorderParams;
};
