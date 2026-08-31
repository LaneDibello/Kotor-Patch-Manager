#include "CSWSTrigger.h"
#include "GameVersion.h"
#include "CExoLocString.h"

CSWSTrigger::GetCanFireMineOnObjectFn CSWSTrigger::getCanFireMineOnObject = nullptr;
CSWSTrigger::OnEnterMineFn CSWSTrigger::onEnterMine = nullptr;
CSWSTrigger::GetFirstNameFn CSWSTrigger::getFirstName = nullptr;
CSWSTrigger::GetTargetAreaFn CSWSTrigger::getTargetArea = nullptr;
CSWSTrigger::InTriggerFn CSWSTrigger::inTrigger = nullptr;
CSWSTrigger::SetCreatorFn CSWSTrigger::setCreator = nullptr;
CSWSTrigger::RemoveFromAreaFn CSWSTrigger::removeFromArea = nullptr;

bool CSWSTrigger::functionsInitialized = false;
bool CSWSTrigger::offsetsInitialized = false;

void CSWSTrigger::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWSObject::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWSTrigger] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        getCanFireMineOnObject = reinterpret_cast<GetCanFireMineOnObjectFn>(
            GameVersion::GetFunctionAddress("CSWSTrigger", "GetCanFireMineOnObject"));
        onEnterMine = reinterpret_cast<OnEnterMineFn>(
            GameVersion::GetFunctionAddress("CSWSTrigger", "OnEnterMine"));
        getFirstName = reinterpret_cast<GetFirstNameFn>(
            GameVersion::GetFunctionAddress("CSWSTrigger", "GetFirstName"));
        getTargetArea = reinterpret_cast<GetTargetAreaFn>(
            GameVersion::GetFunctionAddress("CSWSTrigger", "GetTargetArea"));
        inTrigger = reinterpret_cast<InTriggerFn>(
            GameVersion::GetFunctionAddress("CSWSTrigger", "InTrigger"));
        setCreator = reinterpret_cast<SetCreatorFn>(
            GameVersion::GetFunctionAddress("CSWSTrigger", "SetCreator"));
        removeFromArea = reinterpret_cast<RemoveFromAreaFn>(
            GameVersion::GetFunctionAddress("CSWSTrigger", "RemoveFromArea"));
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWSTrigger] ERROR: %s\n", e.what());
        return;
    }

    functionsInitialized = true;
}

void CSWSTrigger::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWSObject::InitializeOffsets();

    // No CSWSTrigger offsets wrapped yet
    offsetsInitialized = true;
}

CSWSTrigger::CSWSTrigger(void* objectPtr)
    : CSWSObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWSTrigger::~CSWSTrigger() {
    // Base class destructor handles objectPtr cleanup (we don't own the instance)
}

int CSWSTrigger::GetCanFireMineOnObject(DWORD objectId, int skipFactionCheck) {
    if (!objectPtr || !getCanFireMineOnObject) {
        return 0;
    }
    return getCanFireMineOnObject(objectPtr, objectId, skipFactionCheck);
}

void CSWSTrigger::OnEnterMine(int skipFactionCheck) {
    if (!objectPtr || !onEnterMine) {
        return;
    }
    onEnterMine(objectPtr, skipFactionCheck);
}

CExoLocString* CSWSTrigger::GetFirstName() {
    if (!objectPtr || !getFirstName) {
        return nullptr;
    }

    void* resultPtr = getFirstName(objectPtr);
    if (!resultPtr) {
        return nullptr;
    }

    return new CExoLocString(resultPtr);
}

DWORD CSWSTrigger::GetTargetArea() {
    if (!objectPtr || !getTargetArea) {
        return OBJECT_DEFAULT;
    }
    return getTargetArea(objectPtr);
}

int CSWSTrigger::InTrigger(Vector* point) {
    if (!objectPtr || !inTrigger) {
        return 0;
    }
    return inTrigger(objectPtr, point);
}

void CSWSTrigger::SetCreator(DWORD creatorId) {
    if (!objectPtr || !setCreator) {
        return;
    }
    setCreator(objectPtr, creatorId);
}

void CSWSTrigger::RemoveFromArea() {
    if (!objectPtr || !removeFromArea) {
        return;
    }
    removeFromArea(objectPtr);
}
