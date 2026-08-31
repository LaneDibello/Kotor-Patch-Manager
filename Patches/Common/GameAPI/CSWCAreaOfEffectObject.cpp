#include "CSWCAreaOfEffectObject.h"
#include "CResRef.h"
#include "GameVersion.h"

int CSWCAreaOfEffectObject::offsetTotalActors = -1;
int CSWCAreaOfEffectObject::offsetRadius = -1;
int CSWCAreaOfEffectObject::offsetWidth = -1;
int CSWCAreaOfEffectObject::offsetLength = -1;
int CSWCAreaOfEffectObject::offsetShape = -1;
int CSWCAreaOfEffectObject::offsetSoundOneShot = -1;
int CSWCAreaOfEffectObject::offsetSoundOneShotPercentage = -1;
int CSWCAreaOfEffectObject::offsetOrientWithGround = -1;

bool CSWCAreaOfEffectObject::functionsInitialized = false;
bool CSWCAreaOfEffectObject::offsetsInitialized = false;

void CSWCAreaOfEffectObject::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWCObject::InitializeFunctions();

    // No CSWCAreaOfEffectObject functions wrapped yet
    functionsInitialized = true;
}

void CSWCAreaOfEffectObject::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWCObject::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWCAreaOfEffectObject] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        offsetTotalActors = GameVersion::GetOffset("CSWCAreaOfEffectObject", "total_actors");
        offsetRadius = GameVersion::GetOffset("CSWCAreaOfEffectObject", "radius");
        offsetWidth = GameVersion::GetOffset("CSWCAreaOfEffectObject", "width");
        offsetLength = GameVersion::GetOffset("CSWCAreaOfEffectObject", "length");
        offsetShape = GameVersion::GetOffset("CSWCAreaOfEffectObject", "shape");
        offsetSoundOneShot = GameVersion::GetOffset("CSWCAreaOfEffectObject", "sound_one_shot");
        offsetSoundOneShotPercentage = GameVersion::GetOffset("CSWCAreaOfEffectObject", "sound_one_shot_percentage");
        offsetOrientWithGround = GameVersion::GetOffset("CSWCAreaOfEffectObject", "orient_with_ground");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWCAreaOfEffectObject] ERROR: %s\n", e.what());
    }
}

CSWCAreaOfEffectObject::CSWCAreaOfEffectObject(void* objectPtr)
    : CSWCObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWCAreaOfEffectObject::~CSWCAreaOfEffectObject() {
    // Base class destructor handles objectPtr cleanup (we don't own the instance)
}

// ===== Offsets =====

int CSWCAreaOfEffectObject::GetTotalActors() {
    if (!objectPtr || offsetTotalActors < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetTotalActors);
}

void CSWCAreaOfEffectObject::SetTotalActors(int value) {
    if (!objectPtr || offsetTotalActors < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetTotalActors, value);
}

float CSWCAreaOfEffectObject::GetRadius() {
    if (!objectPtr || offsetRadius < 0) {
        return 0;
    }
    return getObjectProperty<float>(objectPtr, offsetRadius);
}

void CSWCAreaOfEffectObject::SetRadius(float value) {
    if (!objectPtr || offsetRadius < 0) {
        return;
    }
    setObjectProperty<float>(objectPtr, offsetRadius, value);
}

float CSWCAreaOfEffectObject::GetWidth() {
    if (!objectPtr || offsetWidth < 0) {
        return 0;
    }
    return getObjectProperty<float>(objectPtr, offsetWidth);
}

void CSWCAreaOfEffectObject::SetWidth(float value) {
    if (!objectPtr || offsetWidth < 0) {
        return;
    }
    setObjectProperty<float>(objectPtr, offsetWidth, value);
}

float CSWCAreaOfEffectObject::GetLength() {
    if (!objectPtr || offsetLength < 0) {
        return 0;
    }
    return getObjectProperty<float>(objectPtr, offsetLength);
}

void CSWCAreaOfEffectObject::SetLength(float value) {
    if (!objectPtr || offsetLength < 0) {
        return;
    }
    setObjectProperty<float>(objectPtr, offsetLength, value);
}

char CSWCAreaOfEffectObject::GetShape() {
    if (!objectPtr || offsetShape < 0) {
        return 0;
    }
    return getObjectProperty<char>(objectPtr, offsetShape);
}

void CSWCAreaOfEffectObject::SetShape(char value) {
    if (!objectPtr || offsetShape < 0) {
        return;
    }
    setObjectProperty<char>(objectPtr, offsetShape, value);
}

CResRef* CSWCAreaOfEffectObject::GetSoundOneShot() {
    if (!objectPtr || offsetSoundOneShot < 0) {
        return nullptr;
    }
    return new CResRef(static_cast<BYTE*>(objectPtr) + offsetSoundOneShot);
}

int CSWCAreaOfEffectObject::GetSoundOneShotPercentage() {
    if (!objectPtr || offsetSoundOneShotPercentage < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetSoundOneShotPercentage);
}

void CSWCAreaOfEffectObject::SetSoundOneShotPercentage(int value) {
    if (!objectPtr || offsetSoundOneShotPercentage < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetSoundOneShotPercentage, value);
}

int CSWCAreaOfEffectObject::GetOrientWithGround() {
    if (!objectPtr || offsetOrientWithGround < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetOrientWithGround);
}

void CSWCAreaOfEffectObject::SetOrientWithGround(int value) {
    if (!objectPtr || offsetOrientWithGround < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetOrientWithGround, value);
}
