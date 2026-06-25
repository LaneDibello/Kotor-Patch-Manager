#pragma once
#include "../Common.h"
#include "CSWGuiObject.h"

class CSWGuiControl : public CSWGuiObject {
public:
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
