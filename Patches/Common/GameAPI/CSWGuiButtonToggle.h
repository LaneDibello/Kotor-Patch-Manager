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
    CSWGuiBorder* GetBorderSelected();
    CSWGuiBorder* GetBorderHilight();
    CSWGuiControl::GuiEvent GetToggleEvent();
    void SetToggleEvent(CSWGuiControl::GuiEvent toggleEvent);
    int GetBitFlags();
    void SetBitFlags(int bitFlags);
    bool GetSelected();

    // Functions
    void SetSelected(UINT selected);

    // The toggle has two extra embedded CSWGuiBorder members that CSWGuiButton knows
    // nothing about (border_selected at +460, border_hilight at +576), so it has its
    // own six-parameter Initialize. This overload deliberately hides the inherited
    // four-parameter one: calling the base version on a toggle leaves both of those
    // borders as raw malloc garbage, which the first Draw then walks.
    void Initialize(CSWGuiExtent* extent, CSWGuiTextParams* textParams,
                    CSWGuiBorderParams* border1, CSWGuiBorderParams* border2,
                    CSWGuiBorderParams* borderSelected, CSWGuiBorderParams* borderHilight);

    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    typedef void (__thiscall* SetSelectedFn)(void* thisPtr, UINT selected);
    typedef void (__thiscall* InitializeToggleParamsFn)(void* thisPtr, void* extent, void* textParams,
                                                 void* border1, void* border2,
                                                 void* borderSelected, void* borderHilight);
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
    static int offsetBorderSelected;
    static int offsetBorderHilight;
};
