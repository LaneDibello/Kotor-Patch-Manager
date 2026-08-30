#include "CSWCCreature.h"
#include "GameVersion.h"

bool CSWCCreature::functionsInitialized = false;
int CSWCCreature::offsetRunning = -1;
int CSWCCreature::offsetStealth = -1;
bool CSWCCreature::offsetsInitialized = false;

void CSWCCreature::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWCObject::InitializeFunctions();

    // CSWCCreature has no functions
    functionsInitialized = true;
}

void CSWCCreature::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWCObject::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWCCreature] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        offsetRunning = GameVersion::GetOffset("CSWCCreature", "Running");
        offsetStealth = GameVersion::GetOffset("CSWCCreature", "Stealth");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWCCreature] ERROR: %s\n", e.what());
    }
}

CSWCCreature::CSWCCreature(void* creaturePtr)
    : CSWCObject(creaturePtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWCCreature::~CSWCCreature() {
    // Base class destructor handles objectPtr cleanup
}

bool CSWCCreature::GetRunning() {
    if (!objectPtr || offsetRunning < 0) {
        return false;
    }
    int value = getObjectProperty<int>(objectPtr, offsetRunning);
    return value != 0;
}

bool CSWCCreature::GetStealth() {
    if (!objectPtr || offsetStealth < 0) {
        return false;
    }
    int value = getObjectProperty<int>(objectPtr, offsetStealth);
    return value != 0;
}

void CSWCCreature::SetRunning(bool running) {
    if (!objectPtr || offsetRunning < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetRunning, running ? TRUE : FALSE);
}

void CSWCCreature::SetStealth(bool stealth) {
    if (!objectPtr || offsetStealth < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetStealth, stealth ? TRUE : FALSE);
}
