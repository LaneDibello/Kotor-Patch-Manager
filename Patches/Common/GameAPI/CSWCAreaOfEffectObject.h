#pragma once

#include "../Common.h"
#include "CSWCObject.h"

class CResRef;

/// <summary>
/// Wraps a client-side area-of-effect object.
///
/// Skeleton only: the inheritance chain is in place, but no functions or offsets
/// are wrapped yet. Add them following the pattern in CSWSObject / CSWGuiLabel.
/// </summary>
class CSWCAreaOfEffectObject : public CSWCObject {
public:
    explicit CSWCAreaOfEffectObject(void* objectPtr);
    virtual ~CSWCAreaOfEffectObject();


    // ===== Offsets =====
    int GetTotalActors();
    void SetTotalActors(int value);
    float GetRadius();
    void SetRadius(float value);
    float GetWidth();
    void SetWidth(float value);
    float GetLength();
    void SetLength(float value);
    char GetShape();
    void SetShape(char value);
    // Wrapper over the embedded field; caller owns the wrapper, not the memory.
    CResRef* GetSoundOneShot();
    int GetSoundOneShotPercentage();
    void SetSoundOneShotPercentage(int value);
    int GetOrientWithGround();
    void SetOrientWithGround(int value);

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    static int offsetTotalActors;
    static int offsetRadius;
    static int offsetWidth;
    static int offsetLength;
    static int offsetShape;
    static int offsetSoundOneShot;
    static int offsetSoundOneShotPercentage;
    static int offsetOrientWithGround;

    static bool functionsInitialized;
    static bool offsetsInitialized;
};
