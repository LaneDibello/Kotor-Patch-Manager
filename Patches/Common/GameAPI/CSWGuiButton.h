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
    // border is the resting border; hilight is the one drawn on hover.
    CSWGuiBorder* GetBorder();
    CSWGuiBorder* GetHilight();

    // Functions
    void ReSetFont();
    void SetActive(UINT active);
    void SetEnabled(UINT enabled);

    // Overloaded Initialize (matches the game's two CSWGuiButton::Initialize variants).
    void Initialize(CSWGuiExtent* extent, CSWGuiButton* button);
    void Initialize(CSWGuiExtent* extent, CSWGuiTextParams* textParams,
                    CSWGuiBorderParams* borderParams, CSWGuiBorderParams* hilightParams);

    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    typedef void (__thiscall* ReSetFontFn)(void* thisPtr);
    typedef void (__thiscall* SetActiveFn)(void* thisPtr, UINT active);
    typedef void (__thiscall* SetEnabledFn)(void* thisPtr, UINT enabled);
    typedef void (__thiscall* InitializeButtonFn)(void* thisPtr, void* extent, void* button);
    typedef void (__thiscall* InitializeParamsFn)(void* thisPtr, void* extent, void* textParams,
                                                  void* borderParams, void* hilightParams);
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
    static int offsetBorder;
    static int offsetHilight;
};
