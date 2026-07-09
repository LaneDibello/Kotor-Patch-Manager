#pragma once
#include "../Common.h"
#include "GameAPIObject.h"

class Gob;
template<typename T> class CExoArrayList;

/// <summary>
/// GameAPI wrapper for the game's Scene type - a 3D scene that owns and renders a
/// collection of Gobs. Also aliased as CAurScene (the name used in the game's own
/// source; Scene is the internal Aurora name).
/// </summary>
class Scene : public GameAPIObject {
public:
    explicit Scene(void* objectPtr);
    ~Scene();

    // Accessors
    CExoArrayList<Gob*>* GetObjects();
    int GetFog();
    float GetFogStart();
    float GetFogEnd();
    float GetFogDensity();
    Vector GetFogColor();

    // Functions
    void ChildAdd(Gob* child);
    void ChildRemove(Gob* child);
    void DisableAnimations();
    void DisableVisibilityGraph();
    void EnableAnimations();
    bool EnableVisibilityGraph();
    int FindRoom(char* roomName);
    void GetFogRange(float* start, float* end);
    int GetIgnoreVisibilityGraph();
    int GetNumRooms();
    Gob* GetObjectA(char* objectName);
    // Static in the game (__stdcall, no this). x87 FPU call: return comes back in ST0 (typed as double).
    double GetObjectScale(Gob* object);
    bool IsAnimating();
    // Static in the game (__stdcall, no this).
    bool IsGeomFadeEnabled();
    int LoadVisibility(char* visibility);
    // Uses the DB's RemoveFromSceneManager_2. DB marks it __cdecl (request said __thiscall); 'this' is passed as the first explicit stack arg. The other overload comes later.
    void RemoveFromSceneManager(Gob* object);
    void SetBlindLights(Gob* object);
    // Uses the DB's SetCurrentRoom_2 variant.
    void SetCurrentRoom(int index);
    void SetFog(int fog);
    void SetFogColor(Vector* color);
    void SetFogRange(float fogStart, float fogEnd);
    // Static in the game (__stdcall, no this).
    unsigned char SetGeomFadeProperties(int property);
    void SetObjectScale(Gob* object, float scale);
    void SetSceneFocus(Vector focus);
    Gob* SpawnRoom(char* modelName, Vector* position, Quaternion* orientation);
    void UpdateEmitters();

    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    typedef void (__thiscall* ChildAddFn)(void* thisPtr, void* child);
    typedef void (__thiscall* ChildRemoveFn)(void* thisPtr, void* child);
    typedef void (__thiscall* DisableAnimationsFn)(void* thisPtr);
    typedef void (__thiscall* DisableVisibilityGraphFn)(void* thisPtr);
    typedef void (__thiscall* EnableAnimationsFn)(void* thisPtr);
    typedef bool (__thiscall* EnableVisibilityGraphFn)(void* thisPtr);
    typedef int (__thiscall* FindRoomFn)(void* thisPtr, char* roomName);
    typedef void (__thiscall* GetFogRangeFn)(void* thisPtr, float* start, float* end);
    typedef int (__thiscall* GetIgnoreVisibilityGraphFn)(void* thisPtr);
    typedef int (__thiscall* GetNumRoomsFn)(void* thisPtr);
    typedef Gob* (__thiscall* GetObjectAFn)(void* thisPtr, char* objectName);
    typedef double (__stdcall* GetObjectScaleFn)(void* object);
    typedef bool (__thiscall* IsAnimatingFn)(void* thisPtr);
    typedef bool (__stdcall* IsGeomFadeEnabledFn)();
    typedef int (__thiscall* LoadVisibilityFn)(void* thisPtr, char* visibility);
    typedef void (__cdecl* RemoveFromSceneManagerFn)(void* thisPtr, void* object);
    typedef void (__thiscall* SetBlindLightsFn)(void* thisPtr, void* object);
    typedef void (__thiscall* SetCurrentRoomFn)(void* thisPtr, int index);
    typedef void (__thiscall* SetFogFn)(void* thisPtr, int fog);
    typedef void (__thiscall* SetFogColorFn)(void* thisPtr, Vector* color);
    typedef void (__thiscall* SetFogRangeFn)(void* thisPtr, float fogStart, float fogEnd);
    typedef unsigned char (__stdcall* SetGeomFadePropertiesFn)(int property);
    typedef void (__thiscall* SetObjectScaleFn)(void* thisPtr, void* object, float scale);
    typedef void (__thiscall* SetSceneFocusFn)(void* thisPtr, Vector focus);
    typedef Gob* (__thiscall* SpawnRoomFn)(void* thisPtr, char* modelName, Vector* position, Quaternion* orientation);
    typedef void (__thiscall* UpdateEmittersFn)(void* thisPtr);

    static ChildAddFn childAdd;
    static ChildRemoveFn childRemove;
    static DisableAnimationsFn disableAnimations;
    static DisableVisibilityGraphFn disableVisibilityGraph;
    static EnableAnimationsFn enableAnimations;
    static EnableVisibilityGraphFn enableVisibilityGraph;
    static FindRoomFn findRoom;
    static GetFogRangeFn getFogRange;
    static GetIgnoreVisibilityGraphFn getIgnoreVisibilityGraph;
    static GetNumRoomsFn getNumRooms;
    static GetObjectAFn getObjectA;
    static GetObjectScaleFn getObjectScale;
    static IsAnimatingFn isAnimating;
    static IsGeomFadeEnabledFn isGeomFadeEnabled;
    static LoadVisibilityFn loadVisibility;
    static RemoveFromSceneManagerFn removeFromSceneManager;
    static SetBlindLightsFn setBlindLights;
    static SetCurrentRoomFn setCurrentRoom;
    static SetFogFn setFog;
    static SetFogColorFn setFogColor;
    static SetFogRangeFn setFogRange;
    static SetGeomFadePropertiesFn setGeomFadeProperties;
    static SetObjectScaleFn setObjectScale;
    static SetSceneFocusFn setSceneFocus;
    static SpawnRoomFn spawnRoom;
    static UpdateEmittersFn updateEmitters;

    static bool functionsInitialized;
    static bool offsetsInitialized;

    static int offsetObjects;
    static int offsetFog;
    static int offsetFogStart;
    static int offsetFogEnd;
    static int offsetFogDensity;
    static int offsetFogColor;
};

// The game refers to this type as CAurScene in some places; accept either name.
typedef Scene CAurScene;
