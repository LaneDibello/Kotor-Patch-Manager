#pragma once
#include "../Common.h"
#include "CSWGuiObject.h"

class CSWGuiBorder : public CSWGuiObject {
public:
    explicit CSWGuiBorder(void* objectPtr);
    ~CSWGuiBorder();

    // Functions
    void FillCenter(int height, int width, int x, int y, float alpha, Vector* color);
    void FillTile(int height, int width, int x, int y, float alpha, Vector* color);

    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    typedef void (__thiscall* FillCenterFn)(void* thisPtr, int height, int width, int x, int y, float alpha, Vector* color);
    typedef void (__thiscall* FillTileFn)  (void* thisPtr, int height, int width, int x, int y, float alpha, Vector* color);

    static FillCenterFn fillCenter;
    static FillTileFn fillTile;

    static bool functionsInitialized;
    static bool offsetsInitialized;
};
