#pragma once
#include "../Common.h"
#include "CSWGuiButton.h"

class CSWGuiBorder;
class CSWGuiBorderParams;
class CSWGuiTextParams;
struct CSWGuiExtent;

class CSWGuiButtonToggle : public CSWGuiButton {
public:
    explicit CSWGuiButtonToggle(void* objectPtr);
    CSWGuiButtonToggle();
    ~CSWGuiButtonToggle();

    // Accessors. Returned wrapper is heap allocated; caller owns it.
    CSWGuiBorder* GetSelectedBorder();
    CSWGuiBorder* GetHilightSelectedBorder();
    CSWGuiControl::GuiEvent GetToggleEvent();
    void SetToggleEvent(CSWGuiControl::GuiEvent toggleEvent);
    int GetBitFlags();
    void SetBitFlags(int bitFlags);
    bool GetSelected();
    void SetOptionsCheckbox();

    // Functions
    void SetSelected(UINT selected);
    void Initialize(CSWGuiExtent* extent, CSWGuiTextParams* textParams,
                    CSWGuiBorderParams* borderParams, CSWGuiBorderParams* hilightParams,
                    CSWGuiBorderParams* selectedParams, CSWGuiBorderParams* hilightSelectedParams);

    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    typedef void (__thiscall* SetSelectedFn)(void* thisPtr, UINT selected);
    typedef void (__thiscall* InitializeToggleParamsFn)(void* thisPtr, void* extent, void* textParams,
                                                 void* borderParams, void* hilightParams,
                                                 void* selectedParams, void* hilightSelectedParams);
    typedef void* (__thiscall* ConstructorFn)(void* thisPtr);
    typedef void* (__thiscall* DestructorFn)(void* thisPtr);

    static SetSelectedFn setSelected;
    static InitializeToggleParamsFn initializeToggleParams;
    static ConstructorFn constructor;
    static DestructorFn  destructor;
    static int classSize;

    static bool functionsInitialized;
    static bool offsetsInitialized;

    static int offsetToggleEvent;
    static int offsetBitFlags;
    static int offsetSelected;
    static int offsetHilightSelected;

    static void* vtableOptionsCheckbox;
};
