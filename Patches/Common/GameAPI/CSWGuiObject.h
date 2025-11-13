#pragma once
#include "GameVersion.h"
#include "../Common.h"

class CSWGuiObject {
public:
    explicit CSWGuiObject(void* objectPtr);
    ~CSWGuiObject();

    void* GetPtr() const { return objectPtr; }

protected:
    void* objectPtr;

    static void InitializeFunctions();
    static void InitializeOffsets();
    static bool functionsInitialized;
    static bool offsetsInitialized;
};