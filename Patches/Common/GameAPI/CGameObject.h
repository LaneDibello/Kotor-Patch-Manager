#pragma once
#include <windows.h>
#include "../Common.h"
#include "GameVersion.h"
#include "GameAPIObject.h"

/// <summary>
/// Values of the object_type field at CGameObject+0x8.
///
/// NOTE for anyone writing a type-switch that builds a wrapper from a raw
/// CGameObject*: that is only valid where the class embeds its base at offset 0.
/// Verified against the K1 address DB, that holds for every CSWS*/CSWC* object
/// type wrapped here, but NOT for these -- their CGameObject subobject is at a
/// non-zero offset, so the pointer the object array hands you is not the start
/// of the object:
///     AREA    -- CSWSArea game_object @284, CSWCArea @256
///     MODULE  -- CSWSModule game_object @28
///     ITEM    -- CSWSItem server_object @16, CSWCItem object @16
/// Reject those three types rather than wrapping them.
/// </summary>
enum GAME_OBJECT_TYPES {
    OBJECT_0 = 0,
    OBJECT_1 = 1,
    OBJECT_2 = 2,
    MODULE = 3,
    AREA = 4,
    CREATURE = 5,
    ITEM = 6,
    TRIGGER = 7,
    PROJECTILE = 8,
    PLACEABLE = 9,
    DOOR = 10,
    AREAOFEFFECT = 11,
    WAYPOINT = 12,
    ENCOUNTER = 13,
    STORE = 14,
    OBJECT_f = 15,
    SOUND = 16,
};

class CGameObject : public GameAPIObject {
protected:
    // Static function pointers
    static bool functionsInitialized;
    static bool offsetsInitialized;

    // Static offsets
    static int offsetId;
    static int offsetObjectType;

public:
    CGameObject(void* objectPtr);
    virtual ~CGameObject();

    // Public accessor methods
    DWORD GetId();
    WORD GetObjectType();
    // Same value as GetObjectType(), typed as the enum.
    GAME_OBJECT_TYPES GetObjectTypeEnum();

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;
};
