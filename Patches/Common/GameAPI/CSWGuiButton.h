#pragma once
#include "../Common.h"
#include "CSWGuiNavigable.h"

class CSWGuiText;
class CSWGuiBorder;
class CSWGuiTextParams;
class CSWGuiBorderParams;
struct CSWGuiExtent;

class CSWGuiButton : public CSWGuiNavigable {
public:
    explicit CSWGuiButton(void* objectPtr);
    CSWGuiButton();
    ~CSWGuiButton();

    // Accessors. Returned wrapper is heap allocated; caller owns it.
    CSWGuiText* GetText();
    CSWGuiBorder* GetBorder1();
    CSWGuiBorder* GetBorder2();

    // Functions
    void ReSetFont();
    void SetActive(UINT active);
    void SetEnabled(UINT enabled);

    // Overloaded Initialize (matches the game's two CSWGuiButton::Initialize variants).
    void Initialize(CSWGuiExtent* extent, CSWGuiButton* button);
    void Initialize(CSWGuiExtent* extent, CSWGuiTextParams* textParams,
                    CSWGuiBorderParams* border1Params, CSWGuiBorderParams* border2Params);

    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    typedef void (__thiscall* ReSetFontFn)(void* thisPtr);
    typedef void (__thiscall* SetActiveFn)(void* thisPtr, UINT active);
    typedef void (__thiscall* SetEnabledFn)(void* thisPtr, UINT enabled);
    typedef void (__thiscall* InitializeButtonFn)(void* thisPtr, void* extent, void* button);
    typedef void (__thiscall* InitializeParamsFn)(void* thisPtr, void* extent, void* textParams,
                                                  void* border1Params, void* border2Params);
    typedef void* (__thiscall* ConstructorFn)(void* thisPtr);
    typedef void* (__thiscall* DestructorFn)(void* thisPtr);

    static ReSetFontFn reSetFont;
    static SetActiveFn setActive;
    static SetEnabledFn setEnabled;
    static InitializeButtonFn initializeButton;
    static InitializeParamsFn initializeParams;
    static ConstructorFn constructor;
    static DestructorFn  destructor;
    static int classSize;

    static bool functionsInitialized;
    static bool offsetsInitialized;

    static int offsetText;
    static int offsetBorder1;
    static int offsetBorder2;
};
