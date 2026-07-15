#pragma once
#include "../Common.h"
#include "GameAPIObject.h"

class Gob;

/// <summary>
/// GameAPI wrapper for the game's Camera type - renders a Scene from a viewpoint.
/// Also aliased as CAurCamera (the name used in the game's own source; Camera is the
/// internal Aurora name).
/// </summary>
class Camera : public GameAPIObject {
public:
    explicit Camera(void* objectPtr);
    ~Camera();

    // Accessors
    Gob* GetObject();

    // Functions
    // Returns this Camera's underlying game object (the embedded Gob / CAurObject).
    Gob* AsCAurObject();
    void AttachToObject(Gob* object, char* partName, int behavior);
    void BehaviorMessage(char* message);
    void EndViewAngleAnimation(bool resetAngle);
    void GetClipDist(float* clipStart, float* clipEnd);
    // x87 FPU call: return comes back in ST0 (typed as double).
    double GetDist();
    Quaternion* GetOrientation(Quaternion* outOrientation);
    // x87 FPU call: return comes back in ST0 (typed as double).
    double GetPitch();
    Vector* GetPosition(Vector* outPosition);
    // x87 FPU call: return comes back in ST0 (typed as double).
    double GetViewAngle();
    int GetViewPortHeight();
    // x87 FPU call: return comes back in ST0 (typed as double).
    double GetYaw();
    void RenderScene();
    void SetClipDist(float clipStart, float clipEnd);
    void SetFollow(Gob* object);
    void SetOrientation(Quaternion* outOrientation, Quaternion orientation);
    void SetPosition(Vector* outPosition, Vector position);
    void SetViewAngle(float angle);
    void SetViewPort(int left, int top, int width, int height);

    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    typedef Gob* (__thiscall* AsCAurObjectFn)(void* thisPtr);
    typedef void (__thiscall* AttachToObjectFn)(void* thisPtr, void* object, char* partName, int _unused);
    typedef void (__thiscall* BehaviorMessageFn)(void* thisPtr, char* message);
    typedef void (__thiscall* EndViewAngleAnimationFn)(void* thisPtr, bool resetAngle);
    typedef void (__thiscall* GetClipDistFn)(void* thisPtr, float* clipStart, float* clipEnd);
    typedef double (__thiscall* GetDistFn)(void* thisPtr);
    typedef Quaternion* (__thiscall* GetOrientationFn)(void* thisPtr, Quaternion* outOrientation);
    typedef double (__thiscall* GetPitchFn)(void* thisPtr);
    typedef Vector* (__thiscall* GetPositionFn)(void* thisPtr, Vector* outPosition);
    typedef double (__thiscall* GetViewAngleFn)(void* thisPtr);
    typedef int (__thiscall* GetViewPortHeightFn)(void* thisPtr);
    typedef double (__thiscall* GetYawFn)(void* thisPtr);
    typedef void (__thiscall* RenderSceneFn)(void* thisPtr);
    typedef void (__thiscall* SetClipDistFn)(void* thisPtr, float clipStart, float clipEnd);
    typedef void (__thiscall* SetFollowFn)(void* thisPtr, void* object);
    typedef void (__thiscall* SetOrientationFn)(void* thisPtr, Quaternion* outOrientation, Quaternion orientation);
    typedef void (__thiscall* SetPositionFn)(void* thisPtr, Vector* outPosition, Vector position);
    typedef void (__thiscall* SetViewAngleFn)(void* thisPtr, float angle);
    typedef void (__thiscall* SetViewPortFn)(void* thisPtr, int left, int top, int width, int height);

    static AsCAurObjectFn asCAurObject;
    static AttachToObjectFn attachToObject;
    static BehaviorMessageFn behaviorMessage;
    static EndViewAngleAnimationFn endViewAngleAnimation;
    static GetClipDistFn getClipDist;
    static GetDistFn getDist;
    static GetOrientationFn getOrientation;
    static GetPitchFn getPitch;
    static GetPositionFn getPosition;
    static GetViewAngleFn getViewAngle;
    static GetViewPortHeightFn getViewPortHeight;
    static GetYawFn getYaw;
    static RenderSceneFn renderScene;
    static SetClipDistFn setClipDist;
    static SetFollowFn setFollow;
    static SetOrientationFn setOrientation;
    static SetPositionFn setPosition;
    static SetViewAngleFn setViewAngle;
    static SetViewPortFn setViewPort;

    static bool functionsInitialized;
    static bool offsetsInitialized;

    static int offsetObject;
};

// The game refers to this type as CAurCamera in some places; accept either name.
typedef Camera CAurCamera;
