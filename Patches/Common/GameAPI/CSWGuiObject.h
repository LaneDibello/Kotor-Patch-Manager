#pragma once
#include "GameVersion.h"
#include "../Common.h"

class CSWGuiObject {
public:
    explicit CSWGuiObject(void* optionsPtr);
    ~CSWGuiObject();

    void* GetPtr() const { return optionsPtr; }

protected:
    void* objectPtr;

    static void InitializeFunctions();
    static void InitializeOffsets();
    static bool functionsInitialized;
    static bool offsestInitialized;
};