#pragma once
#include "../Common.h"
#include "CSWGuiControl.h"

class CSWGuiText;

class CSWGuiLabel : public CSWGuiControl {
public:
    explicit CSWGuiLabel(void* objectPtr);
    CSWGuiLabel();
    ~CSWGuiLabel();

    // Accessors
    CSWGuiText* GetText();

    // Functions
    void ReSetFont();
    void SetEnabled(UINT enabled);

    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    typedef void (__thiscall* ReSetFontFn)(void* thisPtr);
    typedef void (__thiscall* SetEnabledFn)(void* thisPtr, UINT enabled);
    typedef void* (__thiscall* ConstructorFn)(void* thisPtr);
    typedef void* (__thiscall* DestructorFn)(void* thisPtr);

    static ReSetFontFn reSetFont;
    static SetEnabledFn setEnabled;
    static ConstructorFn constructor;
    static DestructorFn  destructor;
    static int classSize;

    static bool functionsInitialized;
    static bool offsetsInitialized;

    static int offsetText;
};
