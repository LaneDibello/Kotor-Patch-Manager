#include "CSWCVisualEffect.h"
#include "GameVersion.h"

int CSWCVisualEffect::offsetEffectRow = -1;

bool CSWCVisualEffect::functionsInitialized = false;
bool CSWCVisualEffect::offsetsInitialized = false;

void CSWCVisualEffect::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWCObject::InitializeFunctions();

    // No CSWCVisualEffect functions wrapped yet
    functionsInitialized = true;
}

void CSWCVisualEffect::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWCObject::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWCVisualEffect] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        offsetEffectRow = GameVersion::GetOffset("CSWCVisualEffect", "effect_row");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWCVisualEffect] ERROR: %s\n", e.what());
    }
}

CSWCVisualEffect::CSWCVisualEffect(void* objectPtr)
    : CSWCObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWCVisualEffect::~CSWCVisualEffect() {
    // Base class destructor handles objectPtr cleanup (we don't own the instance)
}

// ===== Offsets =====

int CSWCVisualEffect::GetEffectRow() {
    if (!objectPtr || offsetEffectRow < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetEffectRow);
}

void CSWCVisualEffect::SetEffectRow(int value) {
    if (!objectPtr || offsetEffectRow < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetEffectRow, value);
}
