#include "CSWSPlaceable.h"
#include "GameVersion.h"
#include "CSWCPlaceable.h"
#include "CResRef.h"

CSWSPlaceable::GetBodyBagAppearanceFn CSWSPlaceable::getBodyBagAppearance = nullptr;
CSWSPlaceable::GetDeadFn CSWSPlaceable::getDead = nullptr;
CSWSPlaceable::GetEffectSpellIdFn CSWSPlaceable::getEffectSpellId = nullptr;
CSWSPlaceable::SetEffectSpellIdFn CSWSPlaceable::setEffectSpellId = nullptr;
CSWSPlaceable::GetLightIsOnFn CSWSPlaceable::getLightIsOn = nullptr;
CSWSPlaceable::SetLightIsOnFn CSWSPlaceable::setLightIsOn = nullptr;
CSWSPlaceable::GetDialogResrefFn CSWSPlaceable::getDialogResref = nullptr;
CSWSPlaceable::GetClientPlaceableFn CSWSPlaceable::getClientPlaceable = nullptr;
CSWSPlaceable::RemoveFromAreaFn CSWSPlaceable::removeFromArea = nullptr;
CSWSPlaceable::SetOrientationFn CSWSPlaceable::setOrientation = nullptr;

bool CSWSPlaceable::functionsInitialized = false;
bool CSWSPlaceable::offsetsInitialized = false;

void CSWSPlaceable::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWSObject::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWSPlaceable] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        getBodyBagAppearance = reinterpret_cast<GetBodyBagAppearanceFn>(
            GameVersion::GetFunctionAddress("CSWSPlaceable", "GetBodyBagAppearance"));
        getDead = reinterpret_cast<GetDeadFn>(
            GameVersion::GetFunctionAddress("CSWSPlaceable", "GetDead"));
        getEffectSpellId = reinterpret_cast<GetEffectSpellIdFn>(
            GameVersion::GetFunctionAddress("CSWSPlaceable", "GetEffectSpellId"));
        setEffectSpellId = reinterpret_cast<SetEffectSpellIdFn>(
            GameVersion::GetFunctionAddress("CSWSPlaceable", "SetEffectSpellId"));
        getLightIsOn = reinterpret_cast<GetLightIsOnFn>(
            GameVersion::GetFunctionAddress("CSWSPlaceable", "GetLightIsOn"));
        setLightIsOn = reinterpret_cast<SetLightIsOnFn>(
            GameVersion::GetFunctionAddress("CSWSPlaceable", "SetLightIsOn"));
        getDialogResref = reinterpret_cast<GetDialogResrefFn>(
            GameVersion::GetFunctionAddress("CSWSPlaceable", "GetDialogResref"));
        getClientPlaceable = reinterpret_cast<GetClientPlaceableFn>(
            GameVersion::GetFunctionAddress("CSWSPlaceable", "GetClientPlaceable"));
        removeFromArea = reinterpret_cast<RemoveFromAreaFn>(
            GameVersion::GetFunctionAddress("CSWSPlaceable", "RemoveFromArea"));
        setOrientation = reinterpret_cast<SetOrientationFn>(
            GameVersion::GetFunctionAddress("CSWSPlaceable", "SetOrientation"));
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWSPlaceable] ERROR: %s\n", e.what());
        return;
    }

    functionsInitialized = true;
}

void CSWSPlaceable::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWSObject::InitializeOffsets();

    // No CSWSPlaceable offsets wrapped yet
    offsetsInitialized = true;
}

CSWSPlaceable::CSWSPlaceable(void* objectPtr)
    : CSWSObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWSPlaceable::~CSWSPlaceable() {
    // Base class destructor handles objectPtr cleanup (we don't own the instance)
}

BYTE CSWSPlaceable::GetBodyBagAppearance() {
    if (!objectPtr || !getBodyBagAppearance) {
        return 0;
    }
    return getBodyBagAppearance(objectPtr);
}

int CSWSPlaceable::GetDead() {
    if (!objectPtr || !getDead) {
        return 0;
    }
    return getDead(objectPtr);
}

DWORD CSWSPlaceable::GetEffectSpellId() {
    if (!objectPtr || !getEffectSpellId) {
        return 0;
    }
    return getEffectSpellId(objectPtr);
}

void CSWSPlaceable::SetEffectSpellId(DWORD spellId) {
    if (!objectPtr || !setEffectSpellId) {
        return;
    }
    setEffectSpellId(objectPtr, spellId);
}

int CSWSPlaceable::GetLightIsOn() {
    if (!objectPtr || !getLightIsOn) {
        return 0;
    }
    return getLightIsOn(objectPtr);
}

void CSWSPlaceable::SetLightIsOn(int lightState) {
    if (!objectPtr || !setLightIsOn) {
        return;
    }
    setLightIsOn(objectPtr, lightState);
}

CResRef* CSWSPlaceable::GetDialogResref(CResRef* outResref) {
    if (!objectPtr || !getDialogResref) {
        return nullptr;
    }

    void* resultPtr = getDialogResref(objectPtr, outResref ? outResref->GetPtr() : nullptr);
    if (!resultPtr) {
        return nullptr;
    }

    return new CResRef(resultPtr);
}

CSWCPlaceable* CSWSPlaceable::GetClientPlaceable() {
    if (!objectPtr || !getClientPlaceable) {
        return nullptr;
    }

    void* clientPtr = getClientPlaceable(objectPtr);
    if (!clientPtr) {
        return nullptr;
    }

    return new CSWCPlaceable(clientPtr);
}

void CSWSPlaceable::RemoveFromArea() {
    if (!objectPtr || !removeFromArea) {
        return;
    }
    removeFromArea(objectPtr);
}

void CSWSPlaceable::SetOrientation(Quaternion* orientation) {
    if (!objectPtr || !setOrientation) {
        return;
    }
    setOrientation(objectPtr, orientation);
}
