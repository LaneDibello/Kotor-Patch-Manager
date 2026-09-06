#pragma once
#include "../Common.h"
#include "CSWGuiObject.h"

// CSWGuiControl virtual-function table layout for KotOR 1 (Windows): 38 entries /
// 152 bytes. Slot names come from the GuiControlMethods rows in the address
// database (byte offset / 4). The gaps are unnamed slots the game fills with its
// shared stubs 0x63E7F0 ("return NULL") and 0x641DB0 ("return this") -- the
// As<Type> downcasts a class does not participate in.
//
// Derived control classes EXTEND this table rather than matching it (editbox 40,
// listbox 42, button 40), so each reports its own VTableSlotCount(). Slots 0..37
// keep these indices in every one of them.
//
// Like PanelVTableSlot, this is specific to K1/Windows. Used to index a copied
// vtable when overriding control virtuals (see VTableOverride.h).
enum class ControlVTableSlot : int {
    Destructor = 0,
    SetExtent,
    SetActiveControl,
    HandleMouseOver,
    HandleMouseCapturedMovement,
    HandleLoseMouseFocus,
    HandleLMouseDown,
    HandleLMouseUp,
    HandleRMouseDownUnused,
    HandleRMouseUp,
    AsSWGuiPanel,
    AsSWGuiListBox,
    AsSWGuiControl,
    GetHeight,
    Draw,
    HandleInputEvent,
    SetActive,
    HitCheckMouse,
    Load,
    AsNavigable,
    AsLabel,
    AsLabelHighlight,
    AsButton,
    AsButtonToggle,
    // Slots 24..30 are unnamed As<Type> stubs.
    HandleFocusChange = 31,
    GetIsSelectable,
    HandleRMouseDown,
    SetEnabled,
    ReSetFont,
    DisplayToolTip
};

// Number of function entries in the K1/Windows control vtable above.
inline constexpr int CONTROL_VTABLE_SLOT_COUNT = 38;

class CSWGuiControl : public CSWGuiObject {
public:
    // Event flags accepted by AddEvent. Any integer is technically valid, but
    // these are the events decompiled so far. Values are the original (console)
    // event ids; the PC version remaps some inputs onto them:
    //   - AButton is fired by clicking a control or pressing Enter.
    //   - BButton is fired by the Esc key.
    enum GuiEvent {
        HoverEnter   = 0,
        HoverExit    = 1,
        AButton      = 39,
        BButton      = 40,
        XButton      = 41,
        YButton      = 42,
        BlackButton  = 43,
        UpDPad       = 49,
        DownDPad     = 50,
        LeftDPad     = 51,
        RightDPad    = 52,
        ShoulderLeft = 53,
        ShoulderRight= 54,
        UpArrow      = 61,
        DownArrow    = 62,
        LeftArrow    = 63,
        RightArrow   = 64,
        RightMouseUp = 68,
        Tab          = 206,
        MenuLeft     = 243,
        MenuRight    = 244,
        Ok           = 502,
        Cancel       = 503
    };

    explicit CSWGuiControl(void* objectPtr);
    CSWGuiControl();
    ~CSWGuiControl();

    // Accessors
    CSWGuiControl* GetParentControl();
    int GetId();
    // Scratch field the game leaves to the implementer. Controls built at
    // runtime use it to point back at whatever data they represent.
    DWORD GetCustomValue();
    void SetCustomValue(DWORD value);
    int GetControlBitFlags();
    void SetControlBitFlags(int bitFlags);
    bool GetControlBitFlag(int bitIndex);
    void SetControlBitFlag(int bitIndex, bool value);

    // Functions
    void AddChildControl(CSWGuiControl* child);
    void AddEvent(int eventFlag, CSWGuiObject* guiObject, void* menuFunc);
    bool GetIsChild(CSWGuiControl* child);
    bool GetIsSelectable();
    CSWGuiControl* GetSelectableParent();
    void SetActive(UINT active);
    void SetEnabled(UINT enabled);

    // Invokes the game's HandleFocusChange for this control's actual class,
    // bypassing any installed override -- this is how an OverrideHandleFocusChange
    // handler chains to the game's implementation.
    //
    // The game's version does the focus bookkeeping (an editbox sets keyboard mode,
    // GuiManager->focused_edit_box and the caret) and then hands focus to the
    // control's parent *panel*. A control parented to a listbox has no panel parent,
    // so that second half silently does nothing -- see OverrideHandleFocusChange.
    void HandleFocusChange(int hasFocus);

    // Redirect the control's HandleFocusChange virtual to a method on the derived
    // wrapper. Pass it through memberFuncAddr (see Common.h); the handler runs with
    // this wrapper as its `this`, and may call HandleFocusChange() to chain.
    void OverrideHandleFocusChange(void* handler);

    // Invokes the game's HandleLMouseUp for this control's actual class, bypassing
    // any installed override. CSWGuiControl's version raises the AButton event;
    // CSWGuiEditbox replaces it with one that only releases the mouse capture.
    void HandleLMouseUp();

    // Redirect the control's HandleLMouseUp virtual, same contract as
    // OverrideHandleFocusChange.
    void OverrideHandleLMouseUp(void* handler);

    // Redirect the control's Draw virtual. A handler here replaces the control's
    // whole appearance, so it usually composes sub-objects itself rather than
    // chaining. NOTE: this runs every frame -- do not allocate in the handler.
    void OverrideDraw(void* handler);

    // Redirect the control's SetExtent virtual. This is the game's relayout hook:
    // a container (CSWGuiListBox) calls it whenever it repositions the control, so
    // a composite control derives its sub-extents here rather than at Initialize.
    void OverrideSetExtent(void* handler);

    // DEBUG: game .text addresses that invoked the overridden virtuals most recently.
    // The game is not ASLR'd, so these paste straight into Ghidra.
    void* LastFocusChangeCaller() const { return lastFocusChangeCaller; }
    void* LastLMouseUpCaller() const { return lastLMouseUpCaller; }

    void InitializeFunctions() override;
    void InitializeOffsets() override;

    int VTableSlotCount() override;

protected:
    typedef void  (__thiscall* AddChildControlFn)(void* thisPtr, void* child);
    typedef void  (__thiscall* AddEventFn)(void* thisPtr, int eventFlag, void* guiObject, void* menuFunc);
    typedef bool  (__thiscall* GetIsChildFn)(void* thisPtr, void* child);
    typedef bool  (__thiscall* GetIsSelectableFn)(void* thisPtr);
    typedef void* (__thiscall* GetSelectableParentFn)(void* thisPtr);
    typedef void  (__thiscall* SetActiveFn)(void* thisPtr, UINT active);
    typedef void  (__thiscall* SetEnabledFn)(void* thisPtr, UINT enabled);
    typedef void* (__thiscall* ConstructorFn)(void* thisPtr);
    typedef void* (__thiscall* DestructorFn)(void* thisPtr);

    static AddChildControlFn addChildControl;
    static AddEventFn addEvent;
    static GetIsChildFn getIsChild;
    static GetIsSelectableFn getIsSelectable;
    static GetSelectableParentFn getSelectableParent;
    static SetActiveFn setActive;
    static SetEnabledFn setEnabled;

    static bool functionsInitialized;
    static bool offsetsInitialized;

    static int offsetParentControl;
    static int offsetId;
    static int offsetCustomValue;
    static int offsetBitFlags;

    static ConstructorFn constructor;
    static DestructorFn  destructor;
    static int classSize;

    // Raw address of the derived wrapper's handler (via memberFuncAddr), invoked by
    // the thunk. Null when the override is not registered.
    void* focusChangeHandler = nullptr;
    void* lastFocusChangeCaller = nullptr;
    void* lmouseUpHandler = nullptr;
    void* lastLMouseUpCaller = nullptr;
    void* drawHandler = nullptr;
    void* setExtentHandler = nullptr;

    // Installed into ControlVTableSlot::HandleFocusChange. The game calls this as
    // __thiscall (game object in ECX); we recover the owning wrapper from the
    // override's back-pointer and forward to its registered handler. __fastcall
    // stands in for __thiscall on this free-standing function.
    static void __fastcall HandleFocusChangeThunk(void* gameObj, void* edx, int hasFocus);
    static void __fastcall HandleLMouseUpThunk(void* gameObj, void* edx);
    static void __fastcall DrawThunk(void* gameObj, void* edx, float alpha);
    static void __fastcall SetExtentThunk(void* gameObj, void* edx, void* extent);

    // Shared by the call-through helpers: the game function in `slot` for this
    // object's actual class, with an installed override stepped around.
    void* originalVirtual(ControlVTableSlot slot);
};
