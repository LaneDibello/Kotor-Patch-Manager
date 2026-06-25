#pragma once
#include "../Common.h"
#include "CSWGuiObject.h"

class CSWGuiImageParams;

class CSWGuiImage : public CSWGuiObject {
public:
    explicit CSWGuiImage(void* objectPtr);
    CSWGuiImage();
    ~CSWGuiImage();

    // Accessors. Returned wrapper is heap allocated; caller owns it.
    CSWGuiImageParams* GetParams();

    // Functions
    void GetImageExtent(CSWGuiExtent* outExtent);

    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    typedef void (__thiscall* GetImageExtentFn)(void* thisPtr, CSWGuiExtent* outExtent);
    typedef void* (__thiscall* ConstructorFn)(void* thisPtr);
    typedef void* (__thiscall* DestructorFn)(void* thisPtr);

    static GetImageExtentFn getImageExtent;
    static ConstructorFn constructor;
    static DestructorFn  destructor;
    static int classSize;

    static bool functionsInitialized;
    static bool offsetsInitialized;

    static int offsetParams;
};
