#pragma once
#include "../Common.h"
#include "GameAPIObject.h"

/// <summary>
/// Minimal GameAPI wrapper for the game's Scene type.
/// In the game code this type is sometimes aliased as CAurScene.
/// Offsets/functions are intentionally left unpopulated for now; this is a
/// basic wrapper to give the other GameAPI classes a concrete type to return.
/// </summary>
class Scene : public GameAPIObject {
public:
    explicit Scene(void* objectPtr);
    ~Scene();

    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    static bool functionsInitialized;
    static bool offsetsInitialized;
};

// The game refers to this type as CAurScene in some places.
typedef Scene CAurScene;
