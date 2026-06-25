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
