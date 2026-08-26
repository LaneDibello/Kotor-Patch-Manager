#pragma once
#include "../Common.h"
#include "CSWGuiButton.h"

class CSWGuiBorder;

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

    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    typedef void (__thiscall* SetSelectedFn)(void* thisPtr, UINT selected);
    typedef void* (__thiscall* ConstructorFn)(void* thisPtr);
    typedef void* (__thiscall* DestructorFn)(void* thisPtr);

    static SetSelectedFn setSelected;
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
