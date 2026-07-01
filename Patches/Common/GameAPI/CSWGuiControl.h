#pragma once
#include "../Common.h"
#include "CSWGuiObject.h"

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

    // Functions
    void AddChildControl(CSWGuiControl* child);
    void AddEvent(int eventFlag, CSWGuiObject* guiObject, void* menuFunc);
    bool GetIsChild(CSWGuiControl* child);
    bool GetIsSelectable();
    CSWGuiControl* GetSelectableParent();
    void SetActive(UINT active);
    void SetEnabled(UINT enabled);

    void InitializeFunctions() override;
    void InitializeOffsets() override;

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

    static ConstructorFn constructor;
    static DestructorFn  destructor;
    static int classSize;
};
