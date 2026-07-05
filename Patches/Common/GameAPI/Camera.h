#pragma once
#include "../Common.h"
#include "GameAPIObject.h"

/// <summary>
/// Minimal GameAPI wrapper for the game's Camera type.
/// In the game code this type is rarely aliased as CAurCamera.
/// Offsets/functions are intentionally left unpopulated for now; this is a
/// basic wrapper to give the other GameAPI classes a concrete type to return.
/// </summary>
class Camera : public GameAPIObject {
public:
    explicit Camera(void* objectPtr);
    ~Camera();

    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    static bool functionsInitialized;
    static bool offsetsInitialized;
};

// The game refers to this type as CAurCamera in some places.
typedef Camera CAurCamera;
