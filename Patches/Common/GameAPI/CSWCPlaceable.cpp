#include "CSWCPlaceable.h"
#include "GameVersion.h"
#include "CSWSPlaceable.h"
#include "CSWCCreature.h"

CSWCPlaceable::ActionMenuSecurityFn CSWCPlaceable::actionMenuSecurity = nullptr;
CSWCPlaceable::IsHostileFn CSWCPlaceable::isHostile = nullptr;
CSWCPlaceable::SetAppearanceFn CSWCPlaceable::setAppearance = nullptr;
CSWCPlaceable::GetServerPlaceableFn CSWCPlaceable::getServerPlaceable = nullptr;

bool CSWCPlaceable::functionsInitialized = false;
bool CSWCPlaceable::offsetsInitialized = false;

void CSWCPlaceable::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWCObject::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWCPlaceable] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        actionMenuSecurity = reinterpret_cast<ActionMenuSecurityFn>(
            GameVersion::GetFunctionAddress("CSWCPlaceable", "ActionMenuSecurity"));
        isHostile = reinterpret_cast<IsHostileFn>(
            GameVersion::GetFunctionAddress("CSWCPlaceable", "IsHostile"));
        setAppearance = reinterpret_cast<SetAppearanceFn>(
            GameVersion::GetFunctionAddress("CSWCPlaceable", "SetAppearance"));
        getServerPlaceable = reinterpret_cast<GetServerPlaceableFn>(
            GameVersion::GetFunctionAddress("CSWCPlaceable", "GetServerPlaceable"));
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWCPlaceable] ERROR: %s\n", e.what());
        return;
    }

    functionsInitialized = true;
}

void CSWCPlaceable::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWCObject::InitializeOffsets();

    // No CSWCPlaceable offsets wrapped yet
    offsetsInitialized = true;
}

CSWCPlaceable::CSWCPlaceable(void* objectPtr)
    : CSWCObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWCPlaceable::~CSWCPlaceable() {
    // Base class destructor handles objectPtr cleanup (we don't own the instance)
}

void CSWCPlaceable::ActionMenuSecurity(CSWCCreature* creature) {
    if (!objectPtr || !actionMenuSecurity) {
        return;
    }
    actionMenuSecurity(objectPtr, creature ? creature->GetPtr() : nullptr);
}

int CSWCPlaceable::IsHostile() {
    if (!objectPtr || !isHostile) {
        return 0;
    }
    return isHostile(objectPtr);
}

void CSWCPlaceable::SetAppearance(BYTE appearance) {
    if (!objectPtr || !setAppearance) {
        return;
    }
    setAppearance(objectPtr, appearance);
}

CSWSPlaceable* CSWCPlaceable::GetServerPlaceable() {
    if (!objectPtr || !getServerPlaceable) {
        return nullptr;
    }

    void* serverPtr = getServerPlaceable(objectPtr);
    if (!serverPtr) {
        return nullptr;
    }

    return new CSWSPlaceable(serverPtr);
}
