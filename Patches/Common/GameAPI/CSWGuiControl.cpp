#include "CSWGuiControl.h"
#include "GameVersion.h"

CSWGuiControl::AddChildControlFn       CSWGuiControl::addChildControl       = nullptr;
CSWGuiControl::AddEventFn              CSWGuiControl::addEvent              = nullptr;
CSWGuiControl::GetIsChildFn            CSWGuiControl::getIsChild            = nullptr;
CSWGuiControl::GetIsSelectableFn       CSWGuiControl::getIsSelectable       = nullptr;
CSWGuiControl::GetSelectableParentFn   CSWGuiControl::getSelectableParent   = nullptr;
CSWGuiControl::SetActiveFn             CSWGuiControl::setActive             = nullptr;
CSWGuiControl::SetEnabledFn            CSWGuiControl::setEnabled            = nullptr;

bool CSWGuiControl::functionsInitialized = false;
bool CSWGuiControl::offsetsInitialized = false;

int CSWGuiControl::offsetParentControl = -1;
int CSWGuiControl::offsetId = -1;
int CSWGuiControl::offsetCustomValue = -1;
int CSWGuiControl::offsetBitFlags = -1;

CSWGuiControl::ConstructorFn CSWGuiControl::constructor = nullptr;
CSWGuiControl::DestructorFn  CSWGuiControl::destructor  = nullptr;
int CSWGuiControl::classSize = -1;

void CSWGuiControl::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWGuiObject::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiControl] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        addChildControl     = reinterpret_cast<AddChildControlFn>    (GameVersion::GetFunctionAddress("CSWGuiControl", "AddChildControl"));
        addEvent            = reinterpret_cast<AddEventFn>           (GameVersion::GetFunctionAddress("CSWGuiControl", "AddEvent"));
        getIsChild          = reinterpret_cast<GetIsChildFn>         (GameVersion::GetFunctionAddress("CSWGuiControl", "GetIsChild"));
        getIsSelectable     = reinterpret_cast<GetIsSelectableFn>    (GameVersion::GetFunctionAddress("CSWGuiControl", "GetIsSelectable"));
        getSelectableParent = reinterpret_cast<GetSelectableParentFn>(GameVersion::GetFunctionAddress("CSWGuiControl", "GetSelectableParent"));
        setActive           = reinterpret_cast<SetActiveFn>          (GameVersion::GetFunctionAddress("CSWGuiControl", "SetActive"));
        setEnabled          = reinterpret_cast<SetEnabledFn>         (GameVersion::GetFunctionAddress("CSWGuiControl", "SetEnabled"));
        constructor = reinterpret_cast<ConstructorFn>(GameVersion::GetFunctionAddress("CSWGuiControl", "Constructor"));
        destructor  = reinterpret_cast<DestructorFn> (GameVersion::GetFunctionAddress("CSWGuiControl", "Destructor"));

        functionsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiControl] ERROR: %s\n", e.what());
        return;
    }
}

void CSWGuiControl::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWGuiObject::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiControl] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        offsetParentControl = GameVersion::GetOffset("CSWGuiControl", "parent_control");
        offsetId = GameVersion::GetOffset("CSWGuiControl", "id");
        offsetCustomValue = GameVersion::GetOffset("CSWGuiControl", "custom_value");
        offsetBitFlags = GameVersion::GetOffset("CSWGuiControl", "bit_flags");
        classSize = GameVersion::GetClassSize("CSWGuiControl");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiControl] ERROR: %s\n", e.what());
    }
}

CSWGuiControl::CSWGuiControl(void* objectPtr)
    : CSWGuiObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWGuiControl::CSWGuiControl()
    : CSWGuiObject(nullptr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }

    if (classSize > 0 && constructor) {
        objectPtr = malloc(classSize);
        if (objectPtr) {
            constructor(objectPtr);
            shouldFree = true;
        }
    }
}

CSWGuiControl::~CSWGuiControl()
{
    // Put the game's vtable back before the game's destructor runs, so it
    // dispatches against its own vtable (and before we free the memory).
    RestoreVTable();

    if (shouldFree && objectPtr) {
        if (destructor) {
            destructor(objectPtr);
        }
        free(objectPtr);
        objectPtr = nullptr;
        shouldFree = false;
    }
}

CSWGuiControl* CSWGuiControl::GetParentControl() {
    if (!objectPtr || offsetParentControl < 0) {
        return nullptr;
    }
    void* parentPtr = getObjectProperty<void*>(objectPtr, offsetParentControl);
    if (!parentPtr) {
        return nullptr;
    }
    return new CSWGuiControl(parentPtr);
}

int CSWGuiControl::GetId() {
    if (!objectPtr || offsetId < 0) {
        return -1;
    }
    return getObjectProperty<int>(objectPtr, offsetId);
}

DWORD CSWGuiControl::GetCustomValue() {
    if (!objectPtr || offsetCustomValue < 0) {
        return 0;
    }
    return getObjectProperty<DWORD>(objectPtr, offsetCustomValue);
}

void CSWGuiControl::SetCustomValue(DWORD value) {
    if (!objectPtr || offsetCustomValue < 0) return;
    setObjectProperty<DWORD>(objectPtr, offsetCustomValue, value);
}

int CSWGuiControl::GetControlBitFlags() {
    if (!objectPtr || offsetBitFlags < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetBitFlags);
}

void CSWGuiControl::SetControlBitFlags(int bitFlags) {
    if (!objectPtr || offsetBitFlags < 0) return;
    setObjectProperty<int>(objectPtr, offsetBitFlags, bitFlags);
}

bool CSWGuiControl::GetControlBitFlag(int bitIndex) {
    if (bitIndex < 0 || bitIndex > 31) {
        return false;
    }
    return (GetControlBitFlags() & (1 << bitIndex)) != 0;
}

void CSWGuiControl::SetControlBitFlag(int bitIndex, bool value) {
    if (!objectPtr || offsetBitFlags < 0 || bitIndex < 0 || bitIndex > 31) {
        return;
    }
    int flags = GetControlBitFlags();
    if (value) {
        flags |= (1 << bitIndex);
    }
    else {
        flags &= ~(1 << bitIndex);
    }
    SetControlBitFlags(flags);
}

void CSWGuiControl::AddChildControl(CSWGuiControl* child) {
    if (!objectPtr || !addChildControl) return;
    addChildControl(objectPtr, child ? child->GetPtr() : nullptr);
}

void CSWGuiControl::AddEvent(int eventFlag, CSWGuiObject* guiObject, void* menuFunc) {
    if (!objectPtr || !addEvent) return;
    addEvent(objectPtr, eventFlag, guiObject ? guiObject->GetPtr() : nullptr, menuFunc);
}

bool CSWGuiControl::GetIsChild(CSWGuiControl* child) {
    if (!objectPtr || !getIsChild) return false;
    return getIsChild(objectPtr, child ? child->GetPtr() : nullptr);
}

bool CSWGuiControl::GetIsSelectable() {
    if (!objectPtr || !getIsSelectable) return false;
    return getIsSelectable(objectPtr);
}

CSWGuiControl* CSWGuiControl::GetSelectableParent() {
    if (!objectPtr || !getSelectableParent) return nullptr;
    void* parentPtr = getSelectableParent(objectPtr);
    if (!parentPtr) return nullptr;
    return new CSWGuiControl(parentPtr);
}

void CSWGuiControl::SetActive(UINT active) {
    if (!objectPtr || !setActive) return;
    setActive(objectPtr, active);
}

void CSWGuiControl::SetEnabled(UINT enabled) {
    if (!objectPtr || !setEnabled) return;
    setEnabled(objectPtr, enabled);
}

int CSWGuiControl::VTableSlotCount() {
    // Only KotOR 1 on Windows is supported for now. Other versions/platforms
    // return -1 so callers disable vtable overriding rather than corrupting a
    // mismatched layout.
    if (GameVersion::GetTitle() == GameTitle::KOTOR1 &&
        GameVersion::GetPlatform() == GamePlatform::Windows) {
        return CONTROL_VTABLE_SLOT_COUNT;
    }

    debugLog("[CSWGuiControl] WARNING: control vtable layout unknown for this game version; vtable overriding disabled\n");
    return -1;
}

void* CSWGuiControl::originalVirtual(ControlVTableSlot slot) {
    if (!objectPtr) {
        return nullptr;
    }

    const int index = static_cast<int>(slot);

    // With an override installed the object's vtable is our copy, so the slot holds
    // our thunk -- calling it would recurse. Take the saved original instead.
    void* fn = vtableOverride ? vtableOverride->GetOriginal(index) : nullptr;
    if (fn) {
        return fn;
    }

    void** vtable = *reinterpret_cast<void***>(objectPtr);
    return vtable ? vtable[index] : nullptr;
}

void CSWGuiControl::HandleFocusChange(int hasFocus) {
    void* fn = originalVirtual(ControlVTableSlot::HandleFocusChange);
    if (!fn) {
        return;
    }
    reinterpret_cast<void(__thiscall*)(void*, int)>(fn)(objectPtr, hasFocus);
}

void CSWGuiControl::HandleLMouseUp() {
    void* fn = originalVirtual(ControlVTableSlot::HandleLMouseUp);
    if (!fn) {
        return;
    }
    reinterpret_cast<void(__thiscall*)(void*)>(fn)(objectPtr);
}

void CSWGuiControl::OverrideHandleLMouseUp(void* handler) {
    if (!EnsureVTableOverride()) {
        return;
    }
    lmouseUpHandler = handler;
    vtableOverride->Override(static_cast<int>(ControlVTableSlot::HandleLMouseUp),
                             reinterpret_cast<void*>(&CSWGuiControl::HandleLMouseUpThunk));
}

void __fastcall CSWGuiControl::HandleLMouseUpThunk(void* gameObj, void* /*edx*/) {
    void* caller = callerAddress();

    CSWGuiControl* self = static_cast<CSWGuiControl*>(VTableOverride::GetOwner(gameObj));
    if (!self || !self->lmouseUpHandler) return;

    self->lastLMouseUpCaller = caller;

    auto handler = reinterpret_cast<void(__thiscall*)(void*)>(self->lmouseUpHandler);
    handler(self);
}

void CSWGuiControl::OverrideHandleFocusChange(void* handler) {
    if (!EnsureVTableOverride()) {
        return;
    }
    focusChangeHandler = handler;
    vtableOverride->Override(static_cast<int>(ControlVTableSlot::HandleFocusChange),
                             reinterpret_cast<void*>(&CSWGuiControl::HandleFocusChangeThunk));
}

void __fastcall CSWGuiControl::HandleFocusChangeThunk(void* gameObj, void* /*edx*/, int hasFocus) {
    void* caller = callerAddress();

    CSWGuiControl* self = static_cast<CSWGuiControl*>(VTableOverride::GetOwner(gameObj));
    if (!self || !self->focusChangeHandler) return;

    self->lastFocusChangeCaller = caller;

    auto handler = reinterpret_cast<void(__thiscall*)(void*, int)>(self->focusChangeHandler);
    handler(self, hasFocus);
}
