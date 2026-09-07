#pragma once
#include "../Common.h"
#include "CSWGuiObject.h"

class CResGFF;
class CExoString;
struct CResStruct;
struct CResList;

// CSWGuiControl virtual-function table layout for KotOR 1 (Windows): 38 entries /
// 152 bytes.
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
        // Synthesised by CSWGuiSlider::HandleLMouseDown for a click on the track
        // either side of the thumb; it feeds them straight back into its own
        // HandleInputEvent. Not Ok/Cancel, which are 502/503.
        SliderTrackUp   = 500,
        SliderTrackDown = 501,
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
    void HandleFocusChange(int hasFocus);
    // Fired every mouse-move frame while this control holds the mouse capture.
    int HandleMouseCapturedMovement(int x, int y);
    void HandleLMouseDown();
    void HandleLMouseUp();

    // Loads this control from a layout GFF by tag. Walks `controls` (a panel's
    // CONTROLS list) comparing each element's TAG, then dispatches the control's own
    // virtual Load against the struct it found. Also points the control's gui object
    // (offset 0x34) at `owner`, which is where every mouse entry point resolves its
    // coordinates from.
    void LoadFromLayout(CSWGuiObject* owner, CResGFF* gff, CResList* controls, CExoString* tag);

    // The control's own virtual Load, by way of the original vtable slot -- so a
    // class that has overridden Load can still reach the game's implementation.
    void LoadFromGff(CResGFF* gff, CResStruct* item);

    // Sets the extent through the object's CURRENT vtable, so a type that lays its
    // sub-parts out on SetExtent still does -- and any override installed on this
    // instance runs too. CSWGuiObject::SetExtent only writes the field.
    void LayoutExtent(CSWGuiExtent* extent);

    void OverrideHandleFocusChange(void* handler);
    void OverrideDraw(void* handler);
    void OverrideSetExtent(void* handler);
    void OverrideHandleMouseCapturedMovement(void* handler);
    void OverrideHandleLMouseDown(void* handler);
    void OverrideHandleLMouseUp(void* handler);
    void OverrideLoad(void* handler);

    void InitializeFunctions() override;
    void InitializeOffsets() override;

    int VTableSlotCount() override;

protected:
    typedef void  (__thiscall* AddChildControlFn)(void* thisPtr, void* child);
    typedef void  (__thiscall* AddEventFn)(void* thisPtr, int eventFlag, void* guiObject, void* menuFunc);
    typedef void  (__thiscall* LoadFromLayoutFn)(void* thisPtr, void* owner, void* gff, void* controls, void* tag);
    typedef bool  (__thiscall* GetIsChildFn)(void* thisPtr, void* child);
    typedef bool  (__thiscall* GetIsSelectableFn)(void* thisPtr);
    typedef void* (__thiscall* GetSelectableParentFn)(void* thisPtr);
    typedef void  (__thiscall* SetActiveFn)(void* thisPtr, UINT active);
    typedef void  (__thiscall* SetEnabledFn)(void* thisPtr, UINT enabled);
    typedef void* (__thiscall* ConstructorFn)(void* thisPtr);
    typedef void* (__thiscall* DestructorFn)(void* thisPtr);

    static AddChildControlFn addChildControl;
    static AddEventFn addEvent;
    static LoadFromLayoutFn loadFromLayout;
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
    void* drawHandler = nullptr;
    void* setExtentHandler = nullptr;
    void* mouseCapturedMovementHandler = nullptr;
    void* lMouseDownHandler = nullptr;
    void* lMouseUpHandler = nullptr;
    void* loadHandler = nullptr;

    // Installed into ControlVTableSlot::HandleFocusChange. The game calls this as
    // __thiscall (game object in ECX)
    static void __fastcall HandleFocusChangeThunk(void* gameObj, void* edx, int hasFocus);
    static void __fastcall DrawThunk(void* gameObj, void* edx, float alpha);
    static void __fastcall SetExtentThunk(void* gameObj, void* edx, void* extent);
    // Unlike the others this one returns a value, so the no-handler path has to
    // hand back what the game's own implementation does rather than fall through.
    static int __fastcall HandleMouseCapturedMovementThunk(void* gameObj, void* edx, int x, int y);
    static void __fastcall HandleLMouseDownThunk(void* gameObj, void* edx);
    static void __fastcall HandleLMouseUpThunk(void* gameObj, void* edx);
    static void __fastcall LoadThunk(void* gameObj, void* edx, void* gff, void* item);

    void* originalVirtual(ControlVTableSlot slot);
};
