#pragma once
#include "../Common.h"
#include "CSWGuiControl.h"

class CSWGuiText;
class CSWGuiTextParams;
class CSWGuiBorderParams;
struct CSWGuiExtent;

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
    void Initialize(CSWGuiExtent* extent, CSWGuiLabel* labelPrototype, float pointSize);
    void Initialize(CSWGuiExtent* extent, CSWGuiTextParams* textParams,
                    CSWGuiBorderParams* borderParams, float pointSize);

    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    typedef void (__thiscall* ReSetFontFn)(void* thisPtr);
    typedef void (__thiscall* SetEnabledFn)(void* thisPtr, UINT enabled);
    typedef void (__thiscall* InitializeLabelFn)(void* thisPtr, void* extent, void* labelPrototype,
                                                 float pointSize);
    typedef void (__thiscall* InitializeParamsFn)(void* thisPtr, void* extent, void* textParams,
                                                  void* borderParams, float pointSize);
    typedef void* (__thiscall* ConstructorFn)(void* thisPtr);
    typedef void* (__thiscall* DestructorFn)(void* thisPtr);

    static ReSetFontFn reSetFont;
    static SetEnabledFn setEnabled;
    static InitializeLabelFn initializeLabel;
    static InitializeParamsFn initializeParams;
    static ConstructorFn constructor;
    static DestructorFn  destructor;
    static int classSize;

    static bool functionsInitialized;
    static bool offsetsInitialized;

    static int offsetText;
};
