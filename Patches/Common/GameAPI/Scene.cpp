#include "Scene.h"
#include "GameVersion.h"
#include "Gob.h"
#include "CExoArrayList.h"

Scene::ChildAddFn Scene::childAdd = nullptr;
Scene::ChildRemoveFn Scene::childRemove = nullptr;
Scene::DisableAnimationsFn Scene::disableAnimations = nullptr;
Scene::DisableVisibilityGraphFn Scene::disableVisibilityGraph = nullptr;
Scene::EnableAnimationsFn Scene::enableAnimations = nullptr;
Scene::EnableVisibilityGraphFn Scene::enableVisibilityGraph = nullptr;
Scene::FindRoomFn Scene::findRoom = nullptr;
Scene::GetFogRangeFn Scene::getFogRange = nullptr;
Scene::GetIgnoreVisibilityGraphFn Scene::getIgnoreVisibilityGraph = nullptr;
Scene::GetNumRoomsFn Scene::getNumRooms = nullptr;
Scene::GetObjectAFn Scene::getObjectA = nullptr;
Scene::GetObjectScaleFn Scene::getObjectScale = nullptr;
Scene::IsAnimatingFn Scene::isAnimating = nullptr;
Scene::IsGeomFadeEnabledFn Scene::isGeomFadeEnabled = nullptr;
Scene::LoadVisibilityFn Scene::loadVisibility = nullptr;
Scene::RemoveFromSceneManagerFn Scene::removeFromSceneManager = nullptr;
Scene::SetBlindLightsFn Scene::setBlindLights = nullptr;
Scene::SetCurrentRoomFn Scene::setCurrentRoom = nullptr;
Scene::SetFogFn Scene::setFog = nullptr;
Scene::SetFogColorFn Scene::setFogColor = nullptr;
Scene::SetFogRangeFn Scene::setFogRange = nullptr;
Scene::SetGeomFadePropertiesFn Scene::setGeomFadeProperties = nullptr;
Scene::SetObjectScaleFn Scene::setObjectScale = nullptr;
Scene::SetSceneFocusFn Scene::setSceneFocus = nullptr;
Scene::SpawnRoomFn Scene::spawnRoom = nullptr;
Scene::UpdateEmittersFn Scene::updateEmitters = nullptr;

bool Scene::functionsInitialized = false;
bool Scene::offsetsInitialized = false;

int Scene::offsetObjects = -1;
int Scene::offsetFog = -1;
int Scene::offsetFogStart = -1;
int Scene::offsetFogEnd = -1;
int Scene::offsetFogDensity = -1;
int Scene::offsetFogColor = -1;

void Scene::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[Scene] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        childAdd = reinterpret_cast<ChildAddFn>(GameVersion::GetFunctionAddress("Scene", "ChildAdd"));
        childRemove = reinterpret_cast<ChildRemoveFn>(GameVersion::GetFunctionAddress("Scene", "ChildRemove"));
        disableAnimations = reinterpret_cast<DisableAnimationsFn>(GameVersion::GetFunctionAddress("Scene", "DisableAnimations"));
        disableVisibilityGraph = reinterpret_cast<DisableVisibilityGraphFn>(GameVersion::GetFunctionAddress("Scene", "DisableVisibilityGraph"));
        enableAnimations = reinterpret_cast<EnableAnimationsFn>(GameVersion::GetFunctionAddress("Scene", "EnableAnimations"));
        enableVisibilityGraph = reinterpret_cast<EnableVisibilityGraphFn>(GameVersion::GetFunctionAddress("Scene", "EnableVisibilityGraph"));
        findRoom = reinterpret_cast<FindRoomFn>(GameVersion::GetFunctionAddress("Scene", "FindRoom"));
        getFogRange = reinterpret_cast<GetFogRangeFn>(GameVersion::GetFunctionAddress("Scene", "GetFogRange"));
        getIgnoreVisibilityGraph = reinterpret_cast<GetIgnoreVisibilityGraphFn>(GameVersion::GetFunctionAddress("Scene", "GetIgnoreVisibilityGraph"));
        getNumRooms = reinterpret_cast<GetNumRoomsFn>(GameVersion::GetFunctionAddress("Scene", "GetNumRooms"));
        getObjectA = reinterpret_cast<GetObjectAFn>(GameVersion::GetFunctionAddress("Scene", "GetObjectA"));
        getObjectScale = reinterpret_cast<GetObjectScaleFn>(GameVersion::GetFunctionAddress("Scene", "GetObjectScale"));
        isAnimating = reinterpret_cast<IsAnimatingFn>(GameVersion::GetFunctionAddress("Scene", "IsAnimating"));
        isGeomFadeEnabled = reinterpret_cast<IsGeomFadeEnabledFn>(GameVersion::GetFunctionAddress("Scene", "IsGeomFadeEnabled"));
        loadVisibility = reinterpret_cast<LoadVisibilityFn>(GameVersion::GetFunctionAddress("Scene", "LoadVisibility"));
        removeFromSceneManager = reinterpret_cast<RemoveFromSceneManagerFn>(GameVersion::GetFunctionAddress("Scene", "RemoveFromSceneManager_2"));
        setBlindLights = reinterpret_cast<SetBlindLightsFn>(GameVersion::GetFunctionAddress("Scene", "SetBlindLights"));
        setCurrentRoom = reinterpret_cast<SetCurrentRoomFn>(GameVersion::GetFunctionAddress("Scene", "SetCurrentRoom_2"));
        setFog = reinterpret_cast<SetFogFn>(GameVersion::GetFunctionAddress("Scene", "SetFog"));
        setFogColor = reinterpret_cast<SetFogColorFn>(GameVersion::GetFunctionAddress("Scene", "SetFogColor"));
        setFogRange = reinterpret_cast<SetFogRangeFn>(GameVersion::GetFunctionAddress("Scene", "SetFogRange"));
        setGeomFadeProperties = reinterpret_cast<SetGeomFadePropertiesFn>(GameVersion::GetFunctionAddress("Scene", "SetGeomFadeProperties"));
        setObjectScale = reinterpret_cast<SetObjectScaleFn>(GameVersion::GetFunctionAddress("Scene", "SetObjectScale"));
        setSceneFocus = reinterpret_cast<SetSceneFocusFn>(GameVersion::GetFunctionAddress("Scene", "SetSceneFocus"));
        spawnRoom = reinterpret_cast<SpawnRoomFn>(GameVersion::GetFunctionAddress("Scene", "SpawnRoom"));
        updateEmitters = reinterpret_cast<UpdateEmittersFn>(GameVersion::GetFunctionAddress("Scene", "UpdateEmitters"));

        functionsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[Scene] ERROR: %s\n", e.what());
        return;
    }
}

void Scene::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[Scene] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        offsetObjects = GameVersion::GetOffset("Scene", "objects");
        offsetFog = GameVersion::GetOffset("Scene", "fog");
        offsetFogStart = GameVersion::GetOffset("Scene", "fog_start");
        offsetFogEnd = GameVersion::GetOffset("Scene", "fog_end");
        offsetFogDensity = GameVersion::GetOffset("Scene", "fog_density");
        offsetFogColor = GameVersion::GetOffset("Scene", "fog_color");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[Scene] ERROR: %s\n", e.what());
    }
}

Scene::Scene(void* objectPtr)
    : GameAPIObject(objectPtr, false)  // false = don't free (wrapping existing)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

Scene::~Scene()
{
    // Base class destructor handles objectPtr cleanup
}

CExoArrayList<Gob*>* Scene::GetObjects() {
    if (!objectPtr || offsetObjects < 0) {
        return nullptr;
    }
    // objects is an embedded CExoArrayList<Gob*>, not a pointer.
    return new CExoArrayList<Gob*>((char*)objectPtr + offsetObjects);
}

int Scene::GetFog() {
    if (!objectPtr || offsetFog < 0) return 0;
    return getObjectProperty<int>(objectPtr, offsetFog);
}

float Scene::GetFogStart() {
    if (!objectPtr || offsetFogStart < 0) return 0;
    return getObjectProperty<float>(objectPtr, offsetFogStart);
}

float Scene::GetFogEnd() {
    if (!objectPtr || offsetFogEnd < 0) return 0;
    return getObjectProperty<float>(objectPtr, offsetFogEnd);
}

float Scene::GetFogDensity() {
    if (!objectPtr || offsetFogDensity < 0) return 0;
    return getObjectProperty<float>(objectPtr, offsetFogDensity);
}

Vector Scene::GetFogColor() {
    Vector result = {0.0f, 0.0f, 0.0f};
    if (!objectPtr || offsetFogColor < 0) {
        return result;
    }
    return getObjectProperty<Vector>(objectPtr, offsetFogColor);
}

void Scene::ChildAdd(Gob* child) {
    if (!objectPtr || !childAdd) return;
    childAdd(objectPtr, child ? child->GetPtr() : nullptr);
}

void Scene::ChildRemove(Gob* child) {
    if (!objectPtr || !childRemove) return;
    childRemove(objectPtr, child ? child->GetPtr() : nullptr);
}

void Scene::DisableAnimations() {
    if (!objectPtr || !disableAnimations) return;
    disableAnimations(objectPtr);
}

void Scene::DisableVisibilityGraph() {
    if (!objectPtr || !disableVisibilityGraph) return;
    disableVisibilityGraph(objectPtr);
}

void Scene::EnableAnimations() {
    if (!objectPtr || !enableAnimations) return;
    enableAnimations(objectPtr);
}

bool Scene::EnableVisibilityGraph() {
    if (!objectPtr || !enableVisibilityGraph) return false;
    return enableVisibilityGraph(objectPtr);
}

int Scene::FindRoom(char* roomName) {
    if (!objectPtr || !findRoom) return 0;
    return findRoom(objectPtr, roomName);
}

void Scene::GetFogRange(float* start, float* end) {
    if (!objectPtr || !getFogRange) return;
    getFogRange(objectPtr, start, end);
}

int Scene::GetIgnoreVisibilityGraph() {
    if (!objectPtr || !getIgnoreVisibilityGraph) return 0;
    return getIgnoreVisibilityGraph(objectPtr);
}

int Scene::GetNumRooms() {
    if (!objectPtr || !getNumRooms) return 0;
    return getNumRooms(objectPtr);
}

Gob* Scene::GetObjectA(char* objectName) {
    if (!objectPtr || !getObjectA) return nullptr;
    void* __ret = getObjectA(objectPtr, objectName);
    return __ret ? new Gob(__ret) : nullptr;
}

// Static in the game (__stdcall, no this). x87 FPU call: return comes back in ST0 (typed as double).
double Scene::GetObjectScale(Gob* object) {
    if (!getObjectScale) return 0;
    return getObjectScale(object ? object->GetPtr() : nullptr);
}

bool Scene::IsAnimating() {
    if (!objectPtr || !isAnimating) return false;
    return isAnimating(objectPtr);
}

// Static in the game (__stdcall, no this).
bool Scene::IsGeomFadeEnabled() {
    if (!isGeomFadeEnabled) return false;
    return isGeomFadeEnabled();
}

int Scene::LoadVisibility(char* visibility) {
    if (!objectPtr || !loadVisibility) return 0;
    return loadVisibility(objectPtr, visibility);
}

// Uses the DB's RemoveFromSceneManager_2. DB marks it __cdecl (request said __thiscall); 'this' is passed as the first explicit stack arg. The other overload comes later.
void Scene::RemoveFromSceneManager(Gob* object) {
    if (!objectPtr || !removeFromSceneManager) return;
    removeFromSceneManager(objectPtr, object ? object->GetPtr() : nullptr);
}

void Scene::SetBlindLights(Gob* object) {
    if (!objectPtr || !setBlindLights) return;
    setBlindLights(objectPtr, object ? object->GetPtr() : nullptr);
}

// Uses the DB's SetCurrentRoom_2 variant.
void Scene::SetCurrentRoom(int index) {
    if (!objectPtr || !setCurrentRoom) return;
    setCurrentRoom(objectPtr, index);
}

void Scene::SetFog(int fog) {
    if (!objectPtr || !setFog) return;
    setFog(objectPtr, fog);
}

void Scene::SetFogColor(Vector* color) {
    if (!objectPtr || !setFogColor) return;
    setFogColor(objectPtr, color);
}

void Scene::SetFogRange(float fogStart, float fogEnd) {
    if (!objectPtr || !setFogRange) return;
    setFogRange(objectPtr, fogStart, fogEnd);
}

// Static in the game (__stdcall, no this).
unsigned char Scene::SetGeomFadeProperties(int property) {
    if (!setGeomFadeProperties) return 0;
    return setGeomFadeProperties(property);
}

void Scene::SetObjectScale(Gob* object, float scale) {
    if (!objectPtr || !setObjectScale) return;
    setObjectScale(objectPtr, object ? object->GetPtr() : nullptr, scale);
}

void Scene::SetSceneFocus(Vector focus) {
    if (!objectPtr || !setSceneFocus) return;
    setSceneFocus(objectPtr, focus);
}

Gob* Scene::SpawnRoom(char* modelName, Vector* position, Quaternion* orientation) {
    if (!objectPtr || !spawnRoom) return nullptr;
    Gob* __ret = spawnRoom(objectPtr, modelName, position, orientation);
    return __ret ? new Gob(__ret) : nullptr;
}

void Scene::UpdateEmitters() {
    if (!objectPtr || !updateEmitters) return;
    updateEmitters(objectPtr);
}
