#pragma once
#include "../Common.h"
#include "CSWGuiControl.h"

class CSWGuiText;

class CSWGuiLabel : public CSWGuiControl {
public:
    explicit CSWGuiLabel(void* objectPtr);
    ~CSWGuiLabel();

    // Accessors
    CSWGuiText* GetText();

    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    static bool functionsInitialized;
    static bool offsetsInitialized;

    static int offsetText;
};
