#include "CGameObject.h"
#include "CSWSObject.h"
#include "CSWCObject.h"
#include "CSWSCreature.h"
#include "CSWSPlaceable.h"
#include "CSWSDoor.h"
#include "CSWSTrigger.h"
#include "CSWSEncounter.h"
#include "CSWSAreaOfEffectObject.h"
#include "CSWSStore.h"
#include "CSWSWaypoint.h"
#include "CSWCCreature.h"
#include "CSWCSoundObject.h"
#include "CSWCPlaceable.h"
#include "CSWCDoor.h"
#include "CSWCVisualEffect.h"
#include "CSWCTrigger.h"
#include "CSWCMapPin.h"
#include "CSWCProjectile.h"
#include "CSWCAreaOfEffectObject.h"

// Initialize static members
bool CGameObject::functionsInitialized = false;
bool CGameObject::offsetsInitialized = false;
int CGameObject::offsetId = -1;
int CGameObject::offsetObjectType = -1;

CGameObject::CGameObject(void* objectPtr)
    : GameAPIObject(objectPtr, false)  // false = don't free (wrapping existing game memory)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CGameObject::~CGameObject() {
    // Base class destructor handles setting objectPtr to nullptr
}

void CGameObject::InitializeFunctions() {
    // CGameObject has no function pointers currently
    // This method exists for consistency and future extensibility
    functionsInitialized = true;
}

void CGameObject::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CGameObject] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        offsetId = GameVersion::GetOffset("CGameObject", "Id");
        offsetObjectType = GameVersion::GetOffset("CGameObject", "ObjectType");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CGameObject] ERROR: %s\n", e.what());
    }
}

DWORD CGameObject::GetId() {
    if (!objectPtr || offsetId < 0) {
        return OBJECT_DEFAULT;
    }
    return getObjectProperty<DWORD>(objectPtr, offsetId);
}

WORD CGameObject::GetObjectType() {
    if (!objectPtr || offsetObjectType < 0) {
        return 0;
    }
    return getObjectProperty<WORD>(objectPtr, offsetObjectType);
}

GAME_OBJECT_TYPES CGameObject::GetObjectTypeEnum() {
    return static_cast<GAME_OBJECT_TYPES>(GetObjectType());
}

CSWSObject* CGameObject::AsSWSObject() {
    if (!objectPtr) {
        return nullptr;
    }
    return new CSWSObject(objectPtr);
}

CSWCObject* CGameObject::AsSWCObject() {
    if (!objectPtr) {
        return nullptr;
    }
    return new CSWCObject(objectPtr);
}

CSWSCreature* CGameObject::AsSWSCreature() {
    if (!objectPtr || GetObjectTypeEnum() != CREATURE) {
        return nullptr;
    }
    return new CSWSCreature(objectPtr);
}

CSWSPlaceable* CGameObject::AsSWSPlaceable() {
    if (!objectPtr || GetObjectTypeEnum() != PLACEABLE) {
        return nullptr;
    }
    return new CSWSPlaceable(objectPtr);
}

CSWSDoor* CGameObject::AsSWSDoor() {
    if (!objectPtr || GetObjectTypeEnum() != DOOR) {
        return nullptr;
    }
    return new CSWSDoor(objectPtr);
}

CSWSTrigger* CGameObject::AsSWSTrigger() {
    if (!objectPtr || GetObjectTypeEnum() != TRIGGER) {
        return nullptr;
    }
    return new CSWSTrigger(objectPtr);
}

CSWSEncounter* CGameObject::AsSWSEncounter() {
    if (!objectPtr || GetObjectTypeEnum() != ENCOUNTER) {
        return nullptr;
    }
    return new CSWSEncounter(objectPtr);
}

CSWSAreaOfEffectObject* CGameObject::AsSWSAreaOfEffectObject() {
    if (!objectPtr || GetObjectTypeEnum() != AREAOFEFFECT) {
        return nullptr;
    }
    return new CSWSAreaOfEffectObject(objectPtr);
}

CSWSStore* CGameObject::AsSWSStore() {
    if (!objectPtr || GetObjectTypeEnum() != STORE) {
        return nullptr;
    }
    return new CSWSStore(objectPtr);
}

CSWSWaypoint* CGameObject::AsSWSWaypoint() {
    if (!objectPtr || GetObjectTypeEnum() != WAYPOINT) {
        return nullptr;
    }
    return new CSWSWaypoint(objectPtr);
}

CSWCCreature* CGameObject::AsSWCCreature() {
    if (!objectPtr || GetObjectTypeEnum() != CREATURE) {
        return nullptr;
    }
    return new CSWCCreature(objectPtr);
}

CSWCSoundObject* CGameObject::AsSWCSoundObject() {
    if (!objectPtr || GetObjectTypeEnum() != SOUND) {
        return nullptr;
    }
    return new CSWCSoundObject(objectPtr);
}

CSWCPlaceable* CGameObject::AsSWCPlaceable() {
    if (!objectPtr || GetObjectTypeEnum() != PLACEABLE) {
        return nullptr;
    }
    return new CSWCPlaceable(objectPtr);
}

CSWCDoor* CGameObject::AsSWCDoor() {
    if (!objectPtr || GetObjectTypeEnum() != DOOR) {
        return nullptr;
    }
    return new CSWCDoor(objectPtr);
}

CSWCVisualEffect* CGameObject::AsSWCVisualEffect() {
    if (!objectPtr || GetObjectTypeEnum() != AREAOFEFFECT) {
        return nullptr;
    }
    return new CSWCVisualEffect(objectPtr);
}

CSWCTrigger* CGameObject::AsSWCTrigger() {
    if (!objectPtr || GetObjectTypeEnum() != TRIGGER) {
        return nullptr;
    }
    return new CSWCTrigger(objectPtr);
}

CSWCMapPin* CGameObject::AsSWCMapPin() {
    if (!objectPtr || GetObjectTypeEnum() != WAYPOINT) {
        return nullptr;
    }
    return new CSWCMapPin(objectPtr);
}

CSWCProjectile* CGameObject::AsSWCProjectile() {
    if (!objectPtr || GetObjectTypeEnum() != PROJECTILE) {
        return nullptr;
    }
    return new CSWCProjectile(objectPtr);
}

CSWCAreaOfEffectObject* CGameObject::AsSWCAreaOfEffectObject() {
    if (!objectPtr || GetObjectTypeEnum() != AREAOFEFFECT) {
        return nullptr;
    }
    return new CSWCAreaOfEffectObject(objectPtr);
}
