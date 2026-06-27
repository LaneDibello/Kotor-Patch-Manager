#pragma once
#include "../Common.h"
#include "GameAPIObject.h"

// Forward declarations (returned/passed by pointer only)
class CResRef;
class CSWGuiBorder;

/// <summary>
/// Standalone helper that wraps a CSWGuiBorderParams struct in game memory.
/// Holds the descriptive parameters (dimension, inner offset, fill angle, alpha,
/// color, corner/edge/fill image resrefs and the owning border) used to configure
/// a CSWGuiBorder. Does not derive from CSWGuiObject.
/// </summary>
class CSWGuiBorderParams : public GameAPIObject {
public:
    explicit CSWGuiBorderParams(void* objectPtr);
    ~CSWGuiBorderParams();

    // Accessors (offsets without a dedicated game setter)
    int GetDimension();
    void SetDimension(int dimension);
    int GetInnerOffset();
    void SetInnerOffset(int innerOffset);
    int GetFillAngle();
    void SetFillAngle(int fillAngle);
    float GetAlpha();
    void SetAlpha(float alpha);
    Vector GetColor();
    void SetColor(const Vector& color);

    // Accessors (offsets with a dedicated game setter -> getter only).
    // Returned wrappers are heap allocated; caller owns them.
    CResRef* GetCornerImageResRef();
    CResRef* GetEdgeImageResRef();
    CResRef* GetFillImageResRef();
    CSWGuiBorder* GetBorder();

    // Functions
    void SetBorder(CSWGuiBorder* border);
    void SetCornerImage(CResRef* image, int forceUpdate);
    void SetEdgeImage(CResRef* image, int forceUpdate);
    void SetFillImage(CResRef* image, int forceUpdate);

    // Wraps the game's CSWGuiBorderParams::operator= (copies rhs into this).
    CSWGuiBorderParams& operator=(const CSWGuiBorderParams& rhs);

    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    typedef void  (__thiscall* SetBorderFn)     (void* thisPtr, void* border);
    typedef void  (__thiscall* SetCornerImageFn)(void* thisPtr, void* image, int forceUpdate);
    typedef void  (__thiscall* SetEdgeImageFn)  (void* thisPtr, void* image, int forceUpdate);
    typedef void  (__thiscall* SetFillImageFn)  (void* thisPtr, void* image, int forceUpdate);
    typedef void* (__thiscall* AssignFn)        (void* thisPtr, void* rhs);

    static SetBorderFn      setBorder;
    static SetCornerImageFn setCornerImage;
    static SetEdgeImageFn   setEdgeImage;
    static SetFillImageFn   setFillImage;
    static AssignFn         assign;

    static bool functionsInitialized;
    static bool offsetsInitialized;

    static int offsetDimension;
    static int offsetInnerOffset;
    static int offsetFillAngle;
    static int offsetAlpha;
    static int offsetColor;
    static int offsetCornerImageResRef;
    static int offsetEdgeImageResRef;
    static int offsetFillImageResRef;
    static int offsetBorder;
};
