#pragma once
#include "GameVersion.h"
#include "GameAPIObject.h"
#include "CSWGuiExtent.h"
#include "../Common.h"

class CSWGuiObject : public GameAPIObject {
public:
    explicit CSWGuiObject(void* objectPtr);
    ~CSWGuiObject();

    // Accessors
    CSWGuiExtent GetExtent();
    void SetExtent(const CSWGuiExtent& extent);

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    static bool functionsInitialized;
    static bool offsetsInitialized;

    static int offsetExtent;
};
