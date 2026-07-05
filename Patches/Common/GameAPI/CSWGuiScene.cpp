#include "CSWGuiScene.h"
#include "GameVersion.h"
#include "Scene.h"
#include "Camera.h"
#include "Gob.h"
#include "CExoString.h"
#include "CExoArrayList.h"

CSWGuiScene::ConstructorFn CSWGuiScene::constructor = nullptr;

CSWGuiScene::AddModelFn    CSWGuiScene::addModel    = nullptr;
CSWGuiScene::AddModel2Fn   CSWGuiScene::addModel2   = nullptr;
CSWGuiScene::GetModelFn    CSWGuiScene::getModel    = nullptr;
CSWGuiScene::RemoveModelFn CSWGuiScene::removeModel = nullptr;

bool CSWGuiScene::functionsInitialized = false;
bool CSWGuiScene::offsetsInitialized = false;

int CSWGuiScene::offsetScene = -1;
int CSWGuiScene::offsetCamera = -1;
int CSWGuiScene::offsetModels = -1;
int CSWGuiScene::offsetBackgroundColor = -1;

void CSWGuiScene::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWGuiObject::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiScene] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        constructor = reinterpret_cast<ConstructorFn>(GameVersion::GetFunctionAddress("CSWGuiScene", "Constructor"));
        addModel    = reinterpret_cast<AddModelFn>   (GameVersion::GetFunctionAddress("CSWGuiScene", "AddModel"));
        addModel2   = reinterpret_cast<AddModel2Fn>  (GameVersion::GetFunctionAddress("CSWGuiScene", "AddModel_2"));
        getModel    = reinterpret_cast<GetModelFn>   (GameVersion::GetFunctionAddress("CSWGuiScene", "GetModel"));
        removeModel = reinterpret_cast<RemoveModelFn>(GameVersion::GetFunctionAddress("CSWGuiScene", "RemoveModel"));

        functionsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiScene] ERROR: %s\n", e.what());
        return;
    }
}

void CSWGuiScene::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWGuiObject::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiScene] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        offsetScene           = GameVersion::GetOffset("CSWGuiScene", "scene");
        offsetCamera          = GameVersion::GetOffset("CSWGuiScene", "camera");
        offsetModels          = GameVersion::GetOffset("CSWGuiScene", "models");
        offsetBackgroundColor = GameVersion::GetOffset("CSWGuiScene", "background_color");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiScene] ERROR: %s\n", e.what());
    }
}

CSWGuiScene::CSWGuiScene(void* objectPtr)
    : CSWGuiObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWGuiScene::~CSWGuiScene()
{
    // Base class destructor handles objectPtr cleanup
}

void CSWGuiScene::Construct() {
    if (objectPtr && constructor) {
        constructor(objectPtr);
    }
}

Scene* CSWGuiScene::GetScene() {
    if (!objectPtr || offsetScene < 0) {
        return nullptr;
    }
    void* scenePtr = getObjectProperty<void*>(objectPtr, offsetScene);
    if (!scenePtr) {
        return nullptr;
    }
    return new Scene(scenePtr);
}

Camera* CSWGuiScene::GetCamera() {
    if (!objectPtr || offsetCamera < 0) {
        return nullptr;
    }
    void* cameraPtr = getObjectProperty<void*>(objectPtr, offsetCamera);
    if (!cameraPtr) {
        return nullptr;
    }
    return new Camera(cameraPtr);
}

CExoArrayList<Gob*>* CSWGuiScene::GetModels() {
    if (!objectPtr || offsetModels < 0) {
        return nullptr;
    }
    // models is an embedded CExoArrayList<Gob*>, not a pointer.
    return new CExoArrayList<Gob*>((char*)objectPtr + offsetModels);
}

Vector CSWGuiScene::GetBackgroundColor() {
    Vector result = {0.0f, 0.0f, 0.0f};
    if (!objectPtr || offsetBackgroundColor < 0) {
        return result;
    }
    return getObjectProperty<Vector>(objectPtr, offsetBackgroundColor);
}

void CSWGuiScene::SetBackgroundColor(const Vector& color) {
    if (!objectPtr || offsetBackgroundColor < 0) {
        return;
    }
    setObjectProperty<Vector>(objectPtr, offsetBackgroundColor, color);
}

int CSWGuiScene::AddModel(Gob* model, int insertIndex, int attachToScene) {
    if (!objectPtr || !addModel) return 0;
    return addModel(objectPtr, model ? model->GetPtr() : nullptr, insertIndex, attachToScene);
}

Gob* CSWGuiScene::AddModel(CExoString* model, int insertIndex) {
    if (!objectPtr || !addModel2) return nullptr;
    void* modelPtr = addModel2(objectPtr, model ? model->GetPtr() : nullptr, insertIndex);
    if (!modelPtr) return nullptr;
    return new Gob(modelPtr);
}

Gob* CSWGuiScene::GetModel(int indx) {
    if (!objectPtr || !getModel) return nullptr;
    void* modelPtr = getModel(objectPtr, indx);
    if (!modelPtr) return nullptr;
    return new Gob(modelPtr);
}

int CSWGuiScene::RemoveModel(int index, int freeModel) {
    if (!objectPtr || !removeModel) return 0;
    return removeModel(objectPtr, index, freeModel);
}
