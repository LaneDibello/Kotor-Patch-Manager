#pragma once
#include "../Common.h"
#include "CSWGuiNavigable.h"

class CSWGuiEditBox : public CSWGuiNavigable {
public:
    explicit CSWGuiEditBox(void* objectPtr);
    CSWGuiEditBox();
    ~CSWGuiEditBox();

    // Functions
    bool GetIsSelectable();
    void ReSetFont();
    void SetEnabled(UINT enabled);
    void SetFocus();

    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    typedef bool (__thiscall* GetIsSelectableFn)(void* thisPtr);
    typedef void (__thiscall* ReSetFontFn)(void* thisPtr);
    typedef void (__thiscall* SetEnabledFn)(void* thisPtr, UINT enabled);
    typedef void (__thiscall* SetFocusFn)(void* thisPtr);
    typedef void* (__thiscall* ConstructorFn)(void* thisPtr);
    typedef void* (__thiscall* DestructorFn)(void* thisPtr);

    static GetIsSelectableFn getIsSelectable;
    static ReSetFontFn reSetFont;
    static SetEnabledFn setEnabled;
    static SetFocusFn setFocus;
    static ConstructorFn constructor;
    static DestructorFn  destructor;
    static int classSize;

    static bool functionsInitialized;
    static bool offsetsInitialized;
};
