#pragma once

#include "../Common.h"
#include "CSWCObject.h"

class CSWCCreature : public CSWCObject {
public:
    explicit CSWCCreature(void* creaturePtr);
    virtual ~CSWCCreature();

    bool GetRunning();
    bool GetStealth();

    void SetRunning(bool running);
    void SetStealth(bool stealth);


    // ===== Offsets =====
    Vector* GetStartWaypoint();
    void SetStartWaypoint(Vector* value);
    Vector* GetCurrentWaypoint();
    void SetCurrentWaypoint(Vector* value);
    Vector GetMovingOrientation();
    void SetMovingOrientation(const Vector& value);
    WORD GetWaypointCount();
    void SetWaypointCount(WORD value);
    BYTE GetCreatureType();
    void SetCreatureType(BYTE value);
    DWORD GetHeadId();
    void SetHeadId(DWORD value);
    DWORD GetTorsoId();
    void SetTorsoId(DWORD value);
    DWORD GetHandsId();
    void SetHandsId(DWORD value);
    DWORD GetMainHandId();
    void SetMainHandId(DWORD value);
    DWORD GetOffHandId();
    void SetOffHandId(DWORD value);
    DWORD GetLeftArmBandId();
    void SetLeftArmBandId(DWORD value);
    DWORD GetRightArmBandId();
    void SetRightArmBandId(DWORD value);
    DWORD GetImplantId();
    void SetImplantId(DWORD value);
    DWORD GetBeltId();
    void SetBeltId(DWORD value);
    DWORD GetLeftClawId();
    void SetLeftClawId(DWORD value);
    DWORD GetRightClawId();
    void SetRightClawId(DWORD value);
    DWORD GetBiteId();
    void SetBiteId(DWORD value);
    DWORD GetHideId();
    void SetHideId(DWORD value);
    int GetHeadLastInstant();
    void SetHeadLastInstant(int value);
    int GetTorsoLastInstant();
    void SetTorsoLastInstant(int value);
    int GetHandsLastInstant();
    void SetHandsLastInstant(int value);
    int GetMainHandLastInstant();
    void SetMainHandLastInstant(int value);
    int GetOffHandLastInstant();
    void SetOffHandLastInstant(int value);
    int GetLeftArmBandLastInstant();
    void SetLeftArmBandLastInstant(int value);
    int GetRightArmBandLastInstant();
    void SetRightArmBandLastInstant(int value);
    int GetImplantLastInstant();
    void SetImplantLastInstant(int value);
    int GetBeltLastInstant();
    void SetBeltLastInstant(int value);
    int GetLeftClawLastInstant();
    void SetLeftClawLastInstant(int value);
    int GetRightClawLastInstant();
    void SetRightClawLastInstant(int value);
    int GetBiteLastInstant();
    void SetBiteLastInstant(int value);
    int GetHideLastInstant();
    void SetHideLastInstant(int value);
    WORD GetSoundSet();
    void SetSoundSet(WORD value);
    DWORD GetThrownLightsaber();
    void SetThrownLightsaber(DWORD value);

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    static int offsetStartWaypoint;
    static int offsetCurrentWaypoint;
    static int offsetMovingOrientation;
    static int offsetWaypointCount;
    static int offsetCreatureType;
    static int offsetHeadId;
    static int offsetTorsoId;
    static int offsetHandsId;
    static int offsetMainHandId;
    static int offsetOffHandId;
    static int offsetLeftArmBandId;
    static int offsetRightArmBandId;
    static int offsetImplantId;
    static int offsetBeltId;
    static int offsetLeftClawId;
    static int offsetRightClawId;
    static int offsetBiteId;
    static int offsetHideId;
    static int offsetHeadLastInstant;
    static int offsetTorsoLastInstant;
    static int offsetHandsLastInstant;
    static int offsetMainHandLastInstant;
    static int offsetOffHandLastInstant;
    static int offsetLeftArmBandLastInstant;
    static int offsetRightArmBandLastInstant;
    static int offsetImplantLastInstant;
    static int offsetBeltLastInstant;
    static int offsetLeftClawLastInstant;
    static int offsetRightClawLastInstant;
    static int offsetBiteLastInstant;
    static int offsetHideLastInstant;
    static int offsetSoundSet;
    static int offsetThrownLightsaber;

    static bool functionsInitialized;
    static bool offsetsInitialized;

    static int offsetRunning;
    static int offsetStealth;
};
