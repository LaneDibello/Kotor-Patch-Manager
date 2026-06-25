#pragma once
#include "../Common.h"
#include "CSWGuiObject.h"

class CSWGuiImage : public CSWGuiObject {
public:
    explicit CSWGuiImage(void* objectPtr);
    ~CSWGuiImage();

    // Functions
    void GetImageExtent(CSWGuiExtent* outExtent);

    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    typedef void (__thiscall* GetImageExtentFn)(void* thisPtr, CSWGuiExtent* outExtent);

    static GetImageExtentFn getImageExtent;

    static bool functionsInitialized;
    static bool offsetsInitialized;
};
