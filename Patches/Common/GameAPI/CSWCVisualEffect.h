#pragma once

#include "../Common.h"
#include "CSWCObject.h"

/// <summary>
/// Wraps a client-side visual effect instance.
///
/// Skeleton only: the inheritance chain is in place, but no functions or offsets
/// are wrapped yet. Add them following the pattern in CSWSObject / CSWGuiLabel.
/// </summary>
class CSWCVisualEffect : public CSWCObject {
public:
    explicit CSWCVisualEffect(void* objectPtr);
    virtual ~CSWCVisualEffect();


    // ===== Offsets =====
    int GetEffectRow();
    void SetEffectRow(int value);

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    static int offsetEffectRow;

    static bool functionsInitialized;
    static bool offsetsInitialized;
};
