#pragma once
#include "../Common.h"
#include "GameAPIObject.h"

// Forward declarations (returned/passed by pointer only)
class CResRef;
class CSWGuiImage;

/// <summary>
/// Standalone helper that wraps a CSWGuiImageParams struct in game memory.
/// Holds the descriptive parameters (resref, angle, alpha, color and image
/// object) used to configure a CSWGuiImage. Does not derive from CSWGuiObject.
/// </summary>
class CSWGuiImageParams : public GameAPIObject {
public:
    explicit CSWGuiImageParams(void* objectPtr);
    ~CSWGuiImageParams();

    // Accessors (offsets without a dedicated game setter)
    float GetAngle();
    void SetAngle(float angle);
    float GetAlpha();
    void SetAlpha(float alpha);
    Vector GetColor();
    void SetColor(const Vector& color);

    // Accessors (offsets with a dedicated game setter -> getter only).
    // Returned wrappers are heap allocated; caller owns them.
    CResRef* GetResRef();
    CSWGuiImage* GetImageObject();

    // Functions
    void SetImage(CResRef* image, int forceUpdate);
    void SetImageObject(CSWGuiImage* image);
    void SetAlignment(int alignment);
    void SetDrawStyle(int drawStyle);

    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    typedef void (__thiscall* SetImageFn)      (void* thisPtr, void* image, int forceUpdate);
    typedef void (__thiscall* SetImageObjectFn)(void* thisPtr, void* image);
    typedef void (__thiscall* SetAlignmentFn)  (void* thisPtr, int alignment);
    typedef void (__thiscall* SetDrawStyleFn)  (void* thisPtr, int drawStyle);

    static SetImageFn       setImage;
    static SetImageObjectFn setImageObject;
    static SetAlignmentFn   setAlignment;
    static SetDrawStyleFn   setDrawStyle;

    static bool functionsInitialized;
    static bool offsetsInitialized;

    static int offsetResRef;
    static int offsetAngle;
    static int offsetAlpha;
    static int offsetColor;
    static int offsetImageObject;
};
