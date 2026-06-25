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

    // Functions
    void ReSetFont();
    void SetActive(UINT active);
    void SetEnabled(UINT enabled);

    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    typedef void (__thiscall* ReSetFontFn)(void* thisPtr);
    typedef void (__thiscall* SetActiveFn)(void* thisPtr, UINT active);
    typedef void (__thiscall* SetEnabledFn)(void* thisPtr, UINT enabled);

    static ReSetFontFn reSetFont;
    static SetActiveFn setActive;
    static SetEnabledFn setEnabled;

    static bool functionsInitialized;
    static bool offsetsInitialized;

    static int offsetText;
};
