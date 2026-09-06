#pragma once
#include "../Common.h"
#include "CSWGuiNavigable.h"

class CSWGuiBorder;
class CSWGuiEditText;
class CSWGuiTextParams;
class CSWGuiBorderParams;
struct CSWGuiExtent;

// CSWGuiEditBox virtual-function table for KotOR 1 (Windows): 40 entries.
// Adds Initialize (38) and HandleKeyPress (39) past the 38 shared control slots.
// Slots 0..37 keep their ControlVTableSlot indices (see CSWGuiControl.h).
inline constexpr int EDITBOX_VTABLE_SLOT_COUNT = 40;

class CSWGuiEditBox : public CSWGuiNavigable {
public:
    explicit CSWGuiEditBox(void* objectPtr);
    CSWGuiEditBox();
    ~CSWGuiEditBox();

    // Accessors. Returned wrapper is heap allocated; caller owns it.
    CSWGuiBorder* GetBorder();
    CSWGuiEditText* GetEditText();

    // Functions
    bool GetIsSelectable();
    void Initialize(CSWGuiExtent* extent, CSWGuiTextParams* textParams,
                    CSWGuiBorderParams* borderParams);
    void ReSetFont();
    void SetEnabled(UINT enabled);
    void SetExtent(CSWGuiExtent* extent);
    void SetFocus();

    void InitializeFunctions() override;
    void InitializeOffsets() override;

    int VTableSlotCount() override;

protected:
    typedef bool (__thiscall* GetIsSelectableFn)(void* thisPtr);
    typedef void (__thiscall* InitializeFn)(void* thisPtr, void* extent, void* textParams,
                                            void* borderParams);
    typedef void (__thiscall* ReSetFontFn)(void* thisPtr);
    typedef void (__thiscall* SetEnabledFn)(void* thisPtr, UINT enabled);
    typedef void (__thiscall* SetExtentFn)(void* thisPtr, void* extent);
    typedef void (__thiscall* SetFocusFn)(void* thisPtr);
    typedef void* (__thiscall* ConstructorFn)(void* thisPtr);
    typedef void* (__thiscall* DestructorFn)(void* thisPtr);

    static GetIsSelectableFn getIsSelectable;
    static InitializeFn initialize;
    static ReSetFontFn reSetFont;
    static SetEnabledFn setEnabled;
    static SetExtentFn setExtent;
    static SetFocusFn setFocus;
    static ConstructorFn constructor;
    static DestructorFn  destructor;
    static int classSize;

    static bool functionsInitialized;
    static bool offsetsInitialized;

    static int offsetBorder;
    static int offsetEditText;
};
