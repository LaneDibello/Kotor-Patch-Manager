#pragma once
#include "../Common.h"
#include "CSWGuiNavigable.h"

class CSWGuiText;

class CSWGuiButton : public CSWGuiNavigable {
public:
    explicit CSWGuiButton(void* objectPtr);
    ~CSWGuiButton();

    // Accessors
    CSWGuiText* GetText();

    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    static bool functionsInitialized;
    static bool offsetsInitialized;

    static int offsetText;
};
