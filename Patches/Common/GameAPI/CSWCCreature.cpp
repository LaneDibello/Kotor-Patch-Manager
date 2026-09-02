#include "CSWCCreature.h"
#include "GameVersion.h"

bool CSWCCreature::functionsInitialized = false;
int CSWCCreature::offsetRunning = -1;
int CSWCCreature::offsetStealth = -1;
int CSWCCreature::offsetStartWaypoint = -1;
int CSWCCreature::offsetCurrentWaypoint = -1;
int CSWCCreature::offsetMovingOrientation = -1;
int CSWCCreature::offsetWaypointCount = -1;
int CSWCCreature::offsetCreatureType = -1;
int CSWCCreature::offsetHeadId = -1;
int CSWCCreature::offsetTorsoId = -1;
int CSWCCreature::offsetHandsId = -1;
int CSWCCreature::offsetMainHandId = -1;
int CSWCCreature::offsetOffHandId = -1;
int CSWCCreature::offsetLeftArmBandId = -1;
int CSWCCreature::offsetRightArmBandId = -1;
int CSWCCreature::offsetImplantId = -1;
int CSWCCreature::offsetBeltId = -1;
int CSWCCreature::offsetLeftClawId = -1;
int CSWCCreature::offsetRightClawId = -1;
int CSWCCreature::offsetBiteId = -1;
int CSWCCreature::offsetHideId = -1;
int CSWCCreature::offsetHeadLastInstant = -1;
int CSWCCreature::offsetTorsoLastInstant = -1;
int CSWCCreature::offsetHandsLastInstant = -1;
int CSWCCreature::offsetMainHandLastInstant = -1;
int CSWCCreature::offsetOffHandLastInstant = -1;
int CSWCCreature::offsetLeftArmBandLastInstant = -1;
int CSWCCreature::offsetRightArmBandLastInstant = -1;
int CSWCCreature::offsetImplantLastInstant = -1;
int CSWCCreature::offsetBeltLastInstant = -1;
int CSWCCreature::offsetLeftClawLastInstant = -1;
int CSWCCreature::offsetRightClawLastInstant = -1;
int CSWCCreature::offsetBiteLastInstant = -1;
int CSWCCreature::offsetHideLastInstant = -1;
int CSWCCreature::offsetSoundSet = -1;
int CSWCCreature::offsetThrownLightsaber = -1;
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
        offsetStartWaypoint = GameVersion::GetOffset("CSWCCreature", "start_waypoint");
        offsetCurrentWaypoint = GameVersion::GetOffset("CSWCCreature", "current_waypoint");
        offsetMovingOrientation = GameVersion::GetOffset("CSWCCreature", "moving_orientation");
        offsetWaypointCount = GameVersion::GetOffset("CSWCCreature", "waypoint_count");
        offsetCreatureType = GameVersion::GetOffset("CSWCCreature", "creature_type");
        offsetHeadId = GameVersion::GetOffset("CSWCCreature", "head_id");
        offsetTorsoId = GameVersion::GetOffset("CSWCCreature", "torso_id");
        offsetHandsId = GameVersion::GetOffset("CSWCCreature", "hands_id");
        offsetMainHandId = GameVersion::GetOffset("CSWCCreature", "main_hand_id");
        offsetOffHandId = GameVersion::GetOffset("CSWCCreature", "off_hand_id");
        offsetLeftArmBandId = GameVersion::GetOffset("CSWCCreature", "left_arm_band_id");
        offsetRightArmBandId = GameVersion::GetOffset("CSWCCreature", "right_arm_band_id");
        offsetImplantId = GameVersion::GetOffset("CSWCCreature", "implant_id");
        offsetBeltId = GameVersion::GetOffset("CSWCCreature", "belt_id");
        offsetLeftClawId = GameVersion::GetOffset("CSWCCreature", "left_claw_id");
        offsetRightClawId = GameVersion::GetOffset("CSWCCreature", "right_claw_id");
        offsetBiteId = GameVersion::GetOffset("CSWCCreature", "bite_id");
        offsetHideId = GameVersion::GetOffset("CSWCCreature", "hide_id");
        offsetHeadLastInstant = GameVersion::GetOffset("CSWCCreature", "head_last_instant");
        offsetTorsoLastInstant = GameVersion::GetOffset("CSWCCreature", "torso_last_instant");
        offsetHandsLastInstant = GameVersion::GetOffset("CSWCCreature", "hands_last_instant");
        offsetMainHandLastInstant = GameVersion::GetOffset("CSWCCreature", "main_hand_last_instant");
        offsetOffHandLastInstant = GameVersion::GetOffset("CSWCCreature", "off_hand_last_instant");
        offsetLeftArmBandLastInstant = GameVersion::GetOffset("CSWCCreature", "left_arm_band_last_instant");
        offsetRightArmBandLastInstant = GameVersion::GetOffset("CSWCCreature", "right_arm_band_last_instant");
        offsetImplantLastInstant = GameVersion::GetOffset("CSWCCreature", "implant_last_instant");
        offsetBeltLastInstant = GameVersion::GetOffset("CSWCCreature", "belt_last_instant");
        offsetLeftClawLastInstant = GameVersion::GetOffset("CSWCCreature", "left_claw_last_instant");
        offsetRightClawLastInstant = GameVersion::GetOffset("CSWCCreature", "right_claw_last_instant");
        offsetBiteLastInstant = GameVersion::GetOffset("CSWCCreature", "bite_last_instant");
        offsetHideLastInstant = GameVersion::GetOffset("CSWCCreature", "hide_last_instant");
        offsetSoundSet = GameVersion::GetOffset("CSWCCreature", "sound_set");
        offsetThrownLightsaber = GameVersion::GetOffset("CSWCCreature", "thrown_lightsaber");

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

// ===== Offsets =====

Vector* CSWCCreature::GetStartWaypoint() {
    if (!objectPtr || offsetStartWaypoint < 0) {
        return nullptr;
    }
    return getObjectProperty<Vector*>(objectPtr, offsetStartWaypoint);
}

void CSWCCreature::SetStartWaypoint(Vector* value) {
    if (!objectPtr || offsetStartWaypoint < 0) {
        return;
    }
    setObjectProperty<Vector*>(objectPtr, offsetStartWaypoint, value);
}

Vector* CSWCCreature::GetCurrentWaypoint() {
    if (!objectPtr || offsetCurrentWaypoint < 0) {
        return nullptr;
    }
    return getObjectProperty<Vector*>(objectPtr, offsetCurrentWaypoint);
}

void CSWCCreature::SetCurrentWaypoint(Vector* value) {
    if (!objectPtr || offsetCurrentWaypoint < 0) {
        return;
    }
    setObjectProperty<Vector*>(objectPtr, offsetCurrentWaypoint, value);
}

Vector CSWCCreature::GetMovingOrientation() {
    Vector result = { 0.0f, 0.0f, 0.0f };

    if (!objectPtr || offsetMovingOrientation < 0) {
        return result;
    }

    return getObjectProperty<Vector>(objectPtr, offsetMovingOrientation);
}

void CSWCCreature::SetMovingOrientation(const Vector& value) {
    if (!objectPtr || offsetMovingOrientation < 0) {
        return;
    }
    setObjectProperty<Vector>(objectPtr, offsetMovingOrientation, value);
}

WORD CSWCCreature::GetWaypointCount() {
    if (!objectPtr || offsetWaypointCount < 0) {
        return 0;
    }
    return getObjectProperty<WORD>(objectPtr, offsetWaypointCount);
}

void CSWCCreature::SetWaypointCount(WORD value) {
    if (!objectPtr || offsetWaypointCount < 0) {
        return;
    }
    setObjectProperty<WORD>(objectPtr, offsetWaypointCount, value);
}

BYTE CSWCCreature::GetCreatureType() {
    if (!objectPtr || offsetCreatureType < 0) {
        return 0;
    }
    return getObjectProperty<BYTE>(objectPtr, offsetCreatureType);
}

void CSWCCreature::SetCreatureType(BYTE value) {
    if (!objectPtr || offsetCreatureType < 0) {
        return;
    }
    setObjectProperty<BYTE>(objectPtr, offsetCreatureType, value);
}

DWORD CSWCCreature::GetHeadId() {
    if (!objectPtr || offsetHeadId < 0) {
        return 0;
    }
    return getObjectProperty<DWORD>(objectPtr, offsetHeadId);
}

void CSWCCreature::SetHeadId(DWORD value) {
    if (!objectPtr || offsetHeadId < 0) {
        return;
    }
    setObjectProperty<DWORD>(objectPtr, offsetHeadId, value);
}

DWORD CSWCCreature::GetTorsoId() {
    if (!objectPtr || offsetTorsoId < 0) {
        return 0;
    }
    return getObjectProperty<DWORD>(objectPtr, offsetTorsoId);
}

void CSWCCreature::SetTorsoId(DWORD value) {
    if (!objectPtr || offsetTorsoId < 0) {
        return;
    }
    setObjectProperty<DWORD>(objectPtr, offsetTorsoId, value);
}

DWORD CSWCCreature::GetHandsId() {
    if (!objectPtr || offsetHandsId < 0) {
        return 0;
    }
    return getObjectProperty<DWORD>(objectPtr, offsetHandsId);
}

void CSWCCreature::SetHandsId(DWORD value) {
    if (!objectPtr || offsetHandsId < 0) {
        return;
    }
    setObjectProperty<DWORD>(objectPtr, offsetHandsId, value);
}

DWORD CSWCCreature::GetMainHandId() {
    if (!objectPtr || offsetMainHandId < 0) {
        return 0;
    }
    return getObjectProperty<DWORD>(objectPtr, offsetMainHandId);
}

void CSWCCreature::SetMainHandId(DWORD value) {
    if (!objectPtr || offsetMainHandId < 0) {
        return;
    }
    setObjectProperty<DWORD>(objectPtr, offsetMainHandId, value);
}

DWORD CSWCCreature::GetOffHandId() {
    if (!objectPtr || offsetOffHandId < 0) {
        return 0;
    }
    return getObjectProperty<DWORD>(objectPtr, offsetOffHandId);
}

void CSWCCreature::SetOffHandId(DWORD value) {
    if (!objectPtr || offsetOffHandId < 0) {
        return;
    }
    setObjectProperty<DWORD>(objectPtr, offsetOffHandId, value);
}

DWORD CSWCCreature::GetLeftArmBandId() {
    if (!objectPtr || offsetLeftArmBandId < 0) {
        return 0;
    }
    return getObjectProperty<DWORD>(objectPtr, offsetLeftArmBandId);
}

void CSWCCreature::SetLeftArmBandId(DWORD value) {
    if (!objectPtr || offsetLeftArmBandId < 0) {
        return;
    }
    setObjectProperty<DWORD>(objectPtr, offsetLeftArmBandId, value);
}

DWORD CSWCCreature::GetRightArmBandId() {
    if (!objectPtr || offsetRightArmBandId < 0) {
        return 0;
    }
    return getObjectProperty<DWORD>(objectPtr, offsetRightArmBandId);
}

void CSWCCreature::SetRightArmBandId(DWORD value) {
    if (!objectPtr || offsetRightArmBandId < 0) {
        return;
    }
    setObjectProperty<DWORD>(objectPtr, offsetRightArmBandId, value);
}

DWORD CSWCCreature::GetImplantId() {
    if (!objectPtr || offsetImplantId < 0) {
        return 0;
    }
    return getObjectProperty<DWORD>(objectPtr, offsetImplantId);
}

void CSWCCreature::SetImplantId(DWORD value) {
    if (!objectPtr || offsetImplantId < 0) {
        return;
    }
    setObjectProperty<DWORD>(objectPtr, offsetImplantId, value);
}

DWORD CSWCCreature::GetBeltId() {
    if (!objectPtr || offsetBeltId < 0) {
        return 0;
    }
    return getObjectProperty<DWORD>(objectPtr, offsetBeltId);
}

void CSWCCreature::SetBeltId(DWORD value) {
    if (!objectPtr || offsetBeltId < 0) {
        return;
    }
    setObjectProperty<DWORD>(objectPtr, offsetBeltId, value);
}

DWORD CSWCCreature::GetLeftClawId() {
    if (!objectPtr || offsetLeftClawId < 0) {
        return 0;
    }
    return getObjectProperty<DWORD>(objectPtr, offsetLeftClawId);
}

void CSWCCreature::SetLeftClawId(DWORD value) {
    if (!objectPtr || offsetLeftClawId < 0) {
        return;
    }
    setObjectProperty<DWORD>(objectPtr, offsetLeftClawId, value);
}

DWORD CSWCCreature::GetRightClawId() {
    if (!objectPtr || offsetRightClawId < 0) {
        return 0;
    }
    return getObjectProperty<DWORD>(objectPtr, offsetRightClawId);
}

void CSWCCreature::SetRightClawId(DWORD value) {
    if (!objectPtr || offsetRightClawId < 0) {
        return;
    }
    setObjectProperty<DWORD>(objectPtr, offsetRightClawId, value);
}

DWORD CSWCCreature::GetBiteId() {
    if (!objectPtr || offsetBiteId < 0) {
        return 0;
    }
    return getObjectProperty<DWORD>(objectPtr, offsetBiteId);
}

void CSWCCreature::SetBiteId(DWORD value) {
    if (!objectPtr || offsetBiteId < 0) {
        return;
    }
    setObjectProperty<DWORD>(objectPtr, offsetBiteId, value);
}

DWORD CSWCCreature::GetHideId() {
    if (!objectPtr || offsetHideId < 0) {
        return 0;
    }
    return getObjectProperty<DWORD>(objectPtr, offsetHideId);
}

void CSWCCreature::SetHideId(DWORD value) {
    if (!objectPtr || offsetHideId < 0) {
        return;
    }
    setObjectProperty<DWORD>(objectPtr, offsetHideId, value);
}

int CSWCCreature::GetHeadLastInstant() {
    if (!objectPtr || offsetHeadLastInstant < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetHeadLastInstant);
}

void CSWCCreature::SetHeadLastInstant(int value) {
    if (!objectPtr || offsetHeadLastInstant < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetHeadLastInstant, value);
}

int CSWCCreature::GetTorsoLastInstant() {
    if (!objectPtr || offsetTorsoLastInstant < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetTorsoLastInstant);
}

void CSWCCreature::SetTorsoLastInstant(int value) {
    if (!objectPtr || offsetTorsoLastInstant < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetTorsoLastInstant, value);
}

int CSWCCreature::GetHandsLastInstant() {
    if (!objectPtr || offsetHandsLastInstant < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetHandsLastInstant);
}

void CSWCCreature::SetHandsLastInstant(int value) {
    if (!objectPtr || offsetHandsLastInstant < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetHandsLastInstant, value);
}

int CSWCCreature::GetMainHandLastInstant() {
    if (!objectPtr || offsetMainHandLastInstant < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetMainHandLastInstant);
}

void CSWCCreature::SetMainHandLastInstant(int value) {
    if (!objectPtr || offsetMainHandLastInstant < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetMainHandLastInstant, value);
}

int CSWCCreature::GetOffHandLastInstant() {
    if (!objectPtr || offsetOffHandLastInstant < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetOffHandLastInstant);
}

void CSWCCreature::SetOffHandLastInstant(int value) {
    if (!objectPtr || offsetOffHandLastInstant < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetOffHandLastInstant, value);
}

int CSWCCreature::GetLeftArmBandLastInstant() {
    if (!objectPtr || offsetLeftArmBandLastInstant < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetLeftArmBandLastInstant);
}

void CSWCCreature::SetLeftArmBandLastInstant(int value) {
    if (!objectPtr || offsetLeftArmBandLastInstant < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetLeftArmBandLastInstant, value);
}

int CSWCCreature::GetRightArmBandLastInstant() {
    if (!objectPtr || offsetRightArmBandLastInstant < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetRightArmBandLastInstant);
}

void CSWCCreature::SetRightArmBandLastInstant(int value) {
    if (!objectPtr || offsetRightArmBandLastInstant < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetRightArmBandLastInstant, value);
}

int CSWCCreature::GetImplantLastInstant() {
    if (!objectPtr || offsetImplantLastInstant < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetImplantLastInstant);
}

void CSWCCreature::SetImplantLastInstant(int value) {
    if (!objectPtr || offsetImplantLastInstant < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetImplantLastInstant, value);
}

int CSWCCreature::GetBeltLastInstant() {
    if (!objectPtr || offsetBeltLastInstant < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetBeltLastInstant);
}

void CSWCCreature::SetBeltLastInstant(int value) {
    if (!objectPtr || offsetBeltLastInstant < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetBeltLastInstant, value);
}

int CSWCCreature::GetLeftClawLastInstant() {
    if (!objectPtr || offsetLeftClawLastInstant < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetLeftClawLastInstant);
}

void CSWCCreature::SetLeftClawLastInstant(int value) {
    if (!objectPtr || offsetLeftClawLastInstant < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetLeftClawLastInstant, value);
}

int CSWCCreature::GetRightClawLastInstant() {
    if (!objectPtr || offsetRightClawLastInstant < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetRightClawLastInstant);
}

void CSWCCreature::SetRightClawLastInstant(int value) {
    if (!objectPtr || offsetRightClawLastInstant < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetRightClawLastInstant, value);
}

int CSWCCreature::GetBiteLastInstant() {
    if (!objectPtr || offsetBiteLastInstant < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetBiteLastInstant);
}

void CSWCCreature::SetBiteLastInstant(int value) {
    if (!objectPtr || offsetBiteLastInstant < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetBiteLastInstant, value);
}

int CSWCCreature::GetHideLastInstant() {
    if (!objectPtr || offsetHideLastInstant < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetHideLastInstant);
}

void CSWCCreature::SetHideLastInstant(int value) {
    if (!objectPtr || offsetHideLastInstant < 0) {
        return;
    }
    setObjectProperty<int>(objectPtr, offsetHideLastInstant, value);
}

WORD CSWCCreature::GetSoundSet() {
    if (!objectPtr || offsetSoundSet < 0) {
        return 0;
    }
    return getObjectProperty<WORD>(objectPtr, offsetSoundSet);
}

void CSWCCreature::SetSoundSet(WORD value) {
    if (!objectPtr || offsetSoundSet < 0) {
        return;
    }
    setObjectProperty<WORD>(objectPtr, offsetSoundSet, value);
}

DWORD CSWCCreature::GetThrownLightsaber() {
    if (!objectPtr || offsetThrownLightsaber < 0) {
        return 0;
    }
    return getObjectProperty<DWORD>(objectPtr, offsetThrownLightsaber);
}

void CSWCCreature::SetThrownLightsaber(DWORD value) {
    if (!objectPtr || offsetThrownLightsaber < 0) {
        return;
    }
    setObjectProperty<DWORD>(objectPtr, offsetThrownLightsaber, value);
}
