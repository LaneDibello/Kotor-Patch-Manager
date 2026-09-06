#pragma once
#include "../Common.h"
#include "CSWGuiNavigable.h"

class CSWGuiBorder;
class CSWGuiEditText;
class CSWGuiTextParams;
class CSWGuiBorderParams;
struct CSWGuiExtent;

// CSWGuiEditBox virtual-function table for KotOR 1 (Windows): 40 entries.
// Slots 0..37 keep their ControlVTableSlot indices (see CSWGuiControl.h); these two
// are the edit box's own additions past the shared control table.
enum class EditBoxVTableSlot : int {
    Initialize = 38,
    HandleKeyPress = 39
};

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

    // Feeds one keystroke to the game's handler, bypassing any installed override.
    // Handles backspace/delete, printable characters, and the arrow-key events; its
    // Enter and Escape cases route through the parent *panel*, so they do nothing for
    // a box parented to a list box.
    void HandleKeyPress(int key);

    // Redirect the edit box's HandleKeyPress virtual. Same contract as the
    // CSWGuiControl::Override* family: pass the handler through memberFuncAddr, and
    // call HandleKeyPress() to chain.
    void OverrideHandleKeyPress(void* handler);

    void InitializeFunctions() override;
    void InitializeOffsets() override;

    int VTableSlotCount() override;

protected:
    typedef bool (__thiscall* GetIsSelectableFn)(void* thisPtr);
    typedef void (__thiscall* InitializeFn)(void* thisPtr, void* extent, void* textParams,
                                            void* borderParams);
    typedef void (__thiscall* HandleKeyPressFn)(void* thisPtr, int key);
    typedef void (__thiscall* ReSetFontFn)(void* thisPtr);
    typedef void (__thiscall* SetEnabledFn)(void* thisPtr, UINT enabled);
    typedef void (__thiscall* SetExtentFn)(void* thisPtr, void* extent);
    typedef void (__thiscall* SetFocusFn)(void* thisPtr);
    typedef void* (__thiscall* ConstructorFn)(void* thisPtr);
    typedef void* (__thiscall* DestructorFn)(void* thisPtr);

    static GetIsSelectableFn getIsSelectable;
    static InitializeFn initialize;
    static HandleKeyPressFn handleKeyPress;
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

    void* keyPressHandler = nullptr;

    static void __fastcall HandleKeyPressThunk(void* gameObj, void* edx, int key);
};
