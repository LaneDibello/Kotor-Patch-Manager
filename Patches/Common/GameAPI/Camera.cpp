#include "Camera.h"
#include "GameVersion.h"
#include "Gob.h"

Camera::AsCAurObjectFn Camera::asCAurObject = nullptr;
Camera::AttachToObjectFn Camera::attachToObject = nullptr;
Camera::BehaviorMessageFn Camera::behaviorMessage = nullptr;
Camera::EndViewAngleAnimationFn Camera::endViewAngleAnimation = nullptr;
Camera::GetClipDistFn Camera::getClipDist = nullptr;
Camera::GetDistFn Camera::getDist = nullptr;
Camera::GetOrientationFn Camera::getOrientation = nullptr;
Camera::GetPitchFn Camera::getPitch = nullptr;
Camera::GetPositionFn Camera::getPosition = nullptr;
Camera::GetViewAngleFn Camera::getViewAngle = nullptr;
Camera::GetViewPortHeightFn Camera::getViewPortHeight = nullptr;
Camera::GetYawFn Camera::getYaw = nullptr;
Camera::RenderSceneFn Camera::renderScene = nullptr;
Camera::SetClipDistFn Camera::setClipDist = nullptr;
Camera::SetFollowFn Camera::setFollow = nullptr;
Camera::SetOrientationFn Camera::setOrientation = nullptr;
Camera::SetPositionFn Camera::setPosition = nullptr;
Camera::SetViewAngleFn Camera::setViewAngle = nullptr;
Camera::SetViewPortFn Camera::setViewPort = nullptr;

bool Camera::functionsInitialized = false;
bool Camera::offsetsInitialized = false;

int Camera::offsetObject = -1;

void Camera::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[Camera] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        asCAurObject = reinterpret_cast<AsCAurObjectFn>(GameVersion::GetFunctionAddress("Camera", "AsCAurObject"));
        attachToObject = reinterpret_cast<AttachToObjectFn>(GameVersion::GetFunctionAddress("Camera", "AttachToObject"));
        behaviorMessage = reinterpret_cast<BehaviorMessageFn>(GameVersion::GetFunctionAddress("Camera", "BehaviorMessage"));
        endViewAngleAnimation = reinterpret_cast<EndViewAngleAnimationFn>(GameVersion::GetFunctionAddress("Camera", "EndViewAngleAnimation"));
        getClipDist = reinterpret_cast<GetClipDistFn>(GameVersion::GetFunctionAddress("Camera", "GetClipDist"));
        getDist = reinterpret_cast<GetDistFn>(GameVersion::GetFunctionAddress("Camera", "GetDist"));
        getOrientation = reinterpret_cast<GetOrientationFn>(GameVersion::GetFunctionAddress("Camera", "GetOrientation"));
        getPitch = reinterpret_cast<GetPitchFn>(GameVersion::GetFunctionAddress("Camera", "GetPitch"));
        getPosition = reinterpret_cast<GetPositionFn>(GameVersion::GetFunctionAddress("Camera", "GetPosition"));
        getViewAngle = reinterpret_cast<GetViewAngleFn>(GameVersion::GetFunctionAddress("Camera", "GetViewAngle"));
        getViewPortHeight = reinterpret_cast<GetViewPortHeightFn>(GameVersion::GetFunctionAddress("Camera", "GetViewPortHeight"));
        getYaw = reinterpret_cast<GetYawFn>(GameVersion::GetFunctionAddress("Camera", "GetYaw"));
        renderScene = reinterpret_cast<RenderSceneFn>(GameVersion::GetFunctionAddress("Camera", "RenderScene"));
        setClipDist = reinterpret_cast<SetClipDistFn>(GameVersion::GetFunctionAddress("Camera", "SetClipDist"));
        setFollow = reinterpret_cast<SetFollowFn>(GameVersion::GetFunctionAddress("Camera", "SetFollow"));
        setOrientation = reinterpret_cast<SetOrientationFn>(GameVersion::GetFunctionAddress("Camera", "SetOrientation"));
        setPosition = reinterpret_cast<SetPositionFn>(GameVersion::GetFunctionAddress("Camera", "SetPosition"));
        setViewAngle = reinterpret_cast<SetViewAngleFn>(GameVersion::GetFunctionAddress("Camera", "SetViewAngle"));
        setViewPort = reinterpret_cast<SetViewPortFn>(GameVersion::GetFunctionAddress("Camera", "SetViewPort"));

        functionsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[Camera] ERROR: %s\n", e.what());
        return;
    }
}

void Camera::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[Camera] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        offsetObject = GameVersion::GetOffset("Camera", "object");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[Camera] ERROR: %s\n", e.what());
    }
}

Camera::Camera(void* objectPtr)
    : GameAPIObject(objectPtr, false)  // false = don't free (wrapping existing)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

Camera::~Camera()
{
    // Base class destructor handles objectPtr cleanup
}

Gob* Camera::GetObject() {
    if (!objectPtr || offsetObject < 0) {
        return nullptr;
    }
    // object is an embedded Gob sub-object, not a pointer.
    return new Gob((char*)objectPtr + offsetObject);
}

// Returns this Camera's underlying game object (the embedded Gob / CAurObject).
Gob* Camera::AsCAurObject() {
    if (!objectPtr || !asCAurObject) return nullptr;
    void* __ret = asCAurObject(objectPtr);
    return __ret ? new Gob(__ret) : nullptr;
}

void Camera::AttachToObject(Gob* object, char* partName, int behavior) {
    if (!objectPtr || !attachToObject) return;
    attachToObject(objectPtr, object ? object->GetPtr() : nullptr, partName, behavior);
}

void Camera::BehaviorMessage(char* message) {
    if (!objectPtr || !behaviorMessage) return;
    behaviorMessage(objectPtr, message);
}

void Camera::EndViewAngleAnimation(bool resetAngle) {
    if (!objectPtr || !endViewAngleAnimation) return;
    endViewAngleAnimation(objectPtr, resetAngle);
}

void Camera::GetClipDist(float* clipStart, float* clipEnd) {
    if (!objectPtr || !getClipDist) return;
    getClipDist(objectPtr, clipStart, clipEnd);
}

// x87 FPU call: return comes back in ST0 (typed as double).
double Camera::GetDist() {
    if (!objectPtr || !getDist) return 0;
    return getDist(objectPtr);
}

Quaternion* Camera::GetOrientation(Quaternion* outOrientation) {
    if (!objectPtr || !getOrientation) return nullptr;
    return getOrientation(objectPtr, outOrientation);
}

// x87 FPU call: return comes back in ST0 (typed as double).
double Camera::GetPitch() {
    if (!objectPtr || !getPitch) return 0;
    return getPitch(objectPtr);
}

Vector* Camera::GetPosition(Vector* outPosition) {
    if (!objectPtr || !getPosition) return nullptr;
    return getPosition(objectPtr, outPosition);
}

// x87 FPU call: return comes back in ST0 (typed as double).
double Camera::GetViewAngle() {
    if (!objectPtr || !getViewAngle) return 0;
    return getViewAngle(objectPtr);
}

int Camera::GetViewPortHeight() {
    if (!objectPtr || !getViewPortHeight) return 0;
    return getViewPortHeight(objectPtr);
}

// x87 FPU call: return comes back in ST0 (typed as double).
double Camera::GetYaw() {
    if (!objectPtr || !getYaw) return 0;
    return getYaw(objectPtr);
}

void Camera::RenderScene() {
    if (!objectPtr || !renderScene) return;
    renderScene(objectPtr);
}

void Camera::SetClipDist(float clipStart, float clipEnd) {
    if (!objectPtr || !setClipDist) return;
    setClipDist(objectPtr, clipStart, clipEnd);
}

void Camera::SetFollow(Gob* object) {
    if (!objectPtr || !setFollow) return;
    setFollow(objectPtr, object ? object->GetPtr() : nullptr);
}

void Camera::SetOrientation(Quaternion* outOrientation, Quaternion orientation) {
    if (!objectPtr || !setOrientation) return;
    setOrientation(objectPtr, outOrientation, orientation);
}

void Camera::SetPosition(Vector* outPosition, Vector position) {
    if (!objectPtr || !setPosition) return;
    setPosition(objectPtr, outPosition, position);
}

void Camera::SetViewAngle(float angle) {
    if (!objectPtr || !setViewAngle) return;
    setViewAngle(objectPtr, angle);
}

void Camera::SetViewPort(int left, int top, int width, int height) {
    if (!objectPtr || !setViewPort) return;
    setViewPort(objectPtr, left, top, width, height);
}
