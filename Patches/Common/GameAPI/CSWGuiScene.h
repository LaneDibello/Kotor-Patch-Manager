#pragma once
#include "../Common.h"
#include "CSWGuiObject.h"

class Scene;
class Camera;
class Gob;
class CExoString;
template<typename T> class CExoArrayList;

class CSWGuiScene : public CSWGuiObject {
public:
    explicit CSWGuiScene(void* objectPtr);
    ~CSWGuiScene();

    // Accessors
    // Note: extent (offset 4) is provided by the CSWGuiObject base class.
    Scene* GetScene();
    Camera* GetCamera();
    CExoArrayList<Gob*>* GetModels();
    Vector GetBackgroundColor();
    void SetBackgroundColor(const Vector& color);

    // Runs the game's CSWGuiScene constructor over the currently-wrapped memory.
    // Used to build an embedded scene in place (e.g. the scene field inside a
    // CSWGui3DSceneView, whose own constructor was inlined away).
    void Construct();

    // Functions
    int  AddModel(Gob* model, int insertIndex, int attachToScene);
    Gob* AddModel(CExoString* model, int insertIndex);
    Gob* GetModel(int indx);
    int  RemoveModel(int index, int freeModel);

    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    typedef int   (__thiscall* AddModelFn)   (void* thisPtr, void* model, int insertIndex, int attachToScene);
    typedef void* (__thiscall* AddModel2Fn)  (void* thisPtr, void* model, int insertIndex);
    typedef void* (__thiscall* GetModelFn)   (void* thisPtr, int indx);
    typedef int   (__thiscall* RemoveModelFn)(void* thisPtr, int index, int freeModel);
    typedef void* (__thiscall* ConstructorFn) (void* thisPtr);

    static ConstructorFn constructor;

    static AddModelFn    addModel;
    static AddModel2Fn   addModel2;
    static GetModelFn    getModel;
    static RemoveModelFn removeModel;

    static bool functionsInitialized;
    static bool offsetsInitialized;

    static int offsetScene;
    static int offsetCamera;
    static int offsetModels;
    static int offsetBackgroundColor;
};
