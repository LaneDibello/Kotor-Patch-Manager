#pragma once

#include "../Common.h"
#include "CSWSObject.h"

class CExoString;

class CSWSAreaOfEffectObject : public CSWSObject {
public:
    explicit CSWSAreaOfEffectObject(void* objectPtr);
    virtual ~CSWSAreaOfEffectObject();


    // ===== Offsets =====
    BYTE GetShape();
    void SetShape(BYTE value);
    DWORD GetSpellId();
    void SetSpellId(DWORD value);
    Vector* GetCorners();
    void SetCorners(Vector* value);
    CExoString* GetScriptOnHeartbeat();
    CExoString* GetScriptOnUserDefined();
    CExoString* GetScriptOnEnter();
    CExoString* GetScriptOnExit();
    BYTE GetDurationType();
    void SetDurationType(BYTE value);

    int GetLastHeartbeatDay();
    void SetLastHeartbeatDay(int value);
    int GetLastHeartbeatTime();
    void SetLastHeartbeatTime(int value);

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    static int offsetShape;
    static int offsetSpellId;
    static int offsetCorners;
    static int offsetScriptOnHeartbeat;
    static int offsetScriptOnUserDefined;
    static int offsetScriptOnEnter;
    static int offsetScriptOnExit;
    static int offsetDurationType;
    static int offsetLastHeartbeatDay;
    static int offsetLastHeartbeatTime;

    static bool functionsInitialized;
    static bool offsetsInitialized;
};
