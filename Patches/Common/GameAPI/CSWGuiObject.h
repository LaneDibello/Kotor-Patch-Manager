#pragma once
#include "GameVersion.h"
#include "GameAPIObject.h"
#include "../Common.h"

class CSWGuiObject : public GameAPIObject {
public:
    explicit CSWGuiObject(void* objectPtr);
    ~CSWGuiObject();

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    static bool functionsInitialized;
    static bool offsetsInitialized;
};