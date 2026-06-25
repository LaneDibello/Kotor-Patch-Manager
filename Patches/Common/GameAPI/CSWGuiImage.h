#pragma once
#include "../Common.h"
#include "CSWGuiObject.h"

class CSWGuiImage : public CSWGuiObject {
public:
    explicit CSWGuiImage(void* objectPtr);
    CSWGuiImage();
    ~CSWGuiImage();

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
};
