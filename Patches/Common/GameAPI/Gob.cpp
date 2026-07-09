#include "Gob.h"
#include "GameVersion.h"
#include "Scene.h"
#include "CResRef.h"

Gob::AddAnimationFlagFn Gob::addAnimationFlag = nullptr;
Gob::AddAttachmentFn Gob::addAttachment = nullptr;
Gob::AmputateFn Gob::amputate = nullptr;
Gob::AppendFn Gob::append = nullptr;
Gob::AttachToSceneFn Gob::attachToScene = nullptr;
Gob::BehaviorMessageFn Gob::behaviorMessage = nullptr;
Gob::DisableAlwaysRenderFn Gob::disableAlwaysRender = nullptr;
Gob::DisableDistortionFn Gob::disableDistortion = nullptr;
Gob::DisableFogFn Gob::disableFog = nullptr;
Gob::DisableForceAnimationsFn Gob::disableForceAnimations = nullptr;
Gob::DisableInCutsceneFn Gob::disableInCutscene = nullptr;
Gob::DisableInViewVolumeFn Gob::disableInViewVolume = nullptr;
Gob::DisableRenderBumpedOutFn Gob::disableRenderBumpedOut = nullptr;
Gob::DisableRenderGobBBoxFn Gob::disableRenderGobBBox = nullptr;
Gob::EnableAlwaysRenderFn Gob::enableAlwaysRender = nullptr;
Gob::EnableDistortionFn Gob::enableDistortion = nullptr;
Gob::EnableFogFn Gob::enableFog = nullptr;
Gob::EnableForceAnimationsFn Gob::enableForceAnimations = nullptr;
Gob::EnableInCutsceneFn Gob::enableInCutscene = nullptr;
Gob::EnableInViewVolumeFn Gob::enableInViewVolume = nullptr;
Gob::EnableRenderBumpedOutFn Gob::enableRenderBumpedOut = nullptr;
Gob::EnableRenderGobBBoxFn Gob::enableRenderGobBBox = nullptr;
Gob::EndLookAtAnimateFn Gob::endLookAtAnimate = nullptr;
Gob::GetBoundingBoxFn Gob::getBoundingBox = nullptr;
Gob::GetCutsceneDummyPositionFn Gob::getCutsceneDummyPosition = nullptr;
Gob::GetMaxBlurLengthFn Gob::getMaxBlurLength = nullptr;
Gob::GetMaximumLightRadiusFn Gob::getMaximumLightRadius = nullptr;
Gob::GetMinimumBoundingBoxFn Gob::getMinimumBoundingBox = nullptr;
Gob::GetMinimumBoundingSphereFn Gob::getMinimumBoundingSphere = nullptr;
Gob::GetModelNameFn Gob::getModelName = nullptr;
Gob::GetMotionBlurredFn Gob::getMotionBlurred = nullptr;
Gob::GetNameFn Gob::getName = nullptr;
Gob::GetOrientationFn Gob::getOrientation = nullptr;
Gob::GetPartLocalPositionFn Gob::getPartLocalPosition = nullptr;
Gob::GetPositionFn Gob::getPosition = nullptr;
Gob::GetPreviousPositionFn Gob::getPreviousPosition = nullptr;
Gob::IsAlwaysRenderingFn Gob::isAlwaysRendering = nullptr;
Gob::IsAnimatingFn Gob::isAnimating = nullptr;
Gob::IsForceAnimatedFn Gob::isForceAnimated = nullptr;
Gob::IsInCutsceneFn Gob::isInCutscene = nullptr;
Gob::IsInViewVolumeFn Gob::isInViewVolume = nullptr;
Gob::LoadAddInAnimationsFn Gob::loadAddInAnimations = nullptr;
Gob::RemoveAddInAnimationsFn Gob::removeAddInAnimations = nullptr;
Gob::RemoveAnimationFlagFn Gob::removeAnimationFlag = nullptr;
Gob::RemoveAttachmentFn Gob::removeAttachment = nullptr;
Gob::RenderFn Gob::render = nullptr;
Gob::RenderBlurFn Gob::renderBlur = nullptr;
Gob::RestoreTextureFn Gob::restoreTexture = nullptr;
Gob::SetAsCharacterFn Gob::setAsCharacter = nullptr;
Gob::SetCanDownSampleFn Gob::setCanDownSample = nullptr;
Gob::SetColorShiftingFn Gob::setColorShifting = nullptr;
Gob::SetEnvironmentMapFn Gob::setEnvironmentMap = nullptr;
Gob::SetIgnoreHitCheckFn Gob::setIgnoreHitCheck = nullptr;
Gob::SetIlluminationFn Gob::setIllumination = nullptr;
Gob::SetLightPriorityFn Gob::setLightPriority = nullptr;
Gob::SetMaxBlurLengthFn Gob::setMaxBlurLength = nullptr;
Gob::SetMotionBlurredFn Gob::setMotionBlurred = nullptr;
Gob::SetObjectScaleFn Gob::setObjectScale = nullptr;
Gob::SetOrientationFn Gob::setOrientation = nullptr;
Gob::SetPositionFn Gob::setPosition = nullptr;
Gob::SetProcessFlagFn Gob::setProcessFlag = nullptr;
Gob::SetRenderPersonalSpaceFn Gob::setRenderPersonalSpace = nullptr;
Gob::SetSceneFn Gob::setScene = nullptr;
Gob::TurnOffShadowsFn Gob::turnOffShadows = nullptr;
Gob::TurnOnShadowsFn Gob::turnOnShadows = nullptr;

bool Gob::functionsInitialized = false;
bool Gob::offsetsInitialized = false;

int Gob::offsetScene = -1;

void Gob::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[Gob] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        addAnimationFlag = reinterpret_cast<AddAnimationFlagFn>(GameVersion::GetFunctionAddress("Gob", "AddAnimationFlag"));
        addAttachment = reinterpret_cast<AddAttachmentFn>(GameVersion::GetFunctionAddress("Gob", "AddAttachment"));
        amputate = reinterpret_cast<AmputateFn>(GameVersion::GetFunctionAddress("Gob", "Amputate"));
        append = reinterpret_cast<AppendFn>(GameVersion::GetFunctionAddress("Gob", "Append"));
        attachToScene = reinterpret_cast<AttachToSceneFn>(GameVersion::GetFunctionAddress("Gob", "AttachToScene"));
        behaviorMessage = reinterpret_cast<BehaviorMessageFn>(GameVersion::GetFunctionAddress("Gob", "BehaviorMessage"));
        disableAlwaysRender = reinterpret_cast<DisableAlwaysRenderFn>(GameVersion::GetFunctionAddress("Gob", "DisableAlwaysRender"));
        disableDistortion = reinterpret_cast<DisableDistortionFn>(GameVersion::GetFunctionAddress("Gob", "DisableDistortion"));
        disableFog = reinterpret_cast<DisableFogFn>(GameVersion::GetFunctionAddress("Gob", "DisableFog"));
        disableForceAnimations = reinterpret_cast<DisableForceAnimationsFn>(GameVersion::GetFunctionAddress("Gob", "DisableForceAnimations"));
        disableInCutscene = reinterpret_cast<DisableInCutsceneFn>(GameVersion::GetFunctionAddress("Gob", "DisableInCutscene"));
        disableInViewVolume = reinterpret_cast<DisableInViewVolumeFn>(GameVersion::GetFunctionAddress("Gob", "DisableInViewVolume"));
        disableRenderBumpedOut = reinterpret_cast<DisableRenderBumpedOutFn>(GameVersion::GetFunctionAddress("Gob", "DisableRenderBumpedOut"));
        disableRenderGobBBox = reinterpret_cast<DisableRenderGobBBoxFn>(GameVersion::GetFunctionAddress("Gob", "DisableRenderGobBBox"));
        enableAlwaysRender = reinterpret_cast<EnableAlwaysRenderFn>(GameVersion::GetFunctionAddress("Gob", "EnableAlwaysRender"));
        enableDistortion = reinterpret_cast<EnableDistortionFn>(GameVersion::GetFunctionAddress("Gob", "EnableDistortion"));
        enableFog = reinterpret_cast<EnableFogFn>(GameVersion::GetFunctionAddress("Gob", "EnableFog"));
        enableForceAnimations = reinterpret_cast<EnableForceAnimationsFn>(GameVersion::GetFunctionAddress("Gob", "EnableForceAnimations"));
        enableInCutscene = reinterpret_cast<EnableInCutsceneFn>(GameVersion::GetFunctionAddress("Gob", "EnableInCutscene"));
        enableInViewVolume = reinterpret_cast<EnableInViewVolumeFn>(GameVersion::GetFunctionAddress("Gob", "EnableInViewVolume"));
        enableRenderBumpedOut = reinterpret_cast<EnableRenderBumpedOutFn>(GameVersion::GetFunctionAddress("Gob", "EnableRenderBumpedOut"));
        enableRenderGobBBox = reinterpret_cast<EnableRenderGobBBoxFn>(GameVersion::GetFunctionAddress("Gob", "EnableRenderGobBBox"));
        endLookAtAnimate = reinterpret_cast<EndLookAtAnimateFn>(GameVersion::GetFunctionAddress("Gob", "EndLookAtAnimate"));
        getBoundingBox = reinterpret_cast<GetBoundingBoxFn>(GameVersion::GetFunctionAddress("Gob", "GetBoundingBox"));
        getCutsceneDummyPosition = reinterpret_cast<GetCutsceneDummyPositionFn>(GameVersion::GetFunctionAddress("Gob", "GetCutsceneDummyPosition"));
        getMaxBlurLength = reinterpret_cast<GetMaxBlurLengthFn>(GameVersion::GetFunctionAddress("Gob", "GetMaxBlurLength"));
        getMaximumLightRadius = reinterpret_cast<GetMaximumLightRadiusFn>(GameVersion::GetFunctionAddress("Gob", "GetMaximumLightRadius"));
        getMinimumBoundingBox = reinterpret_cast<GetMinimumBoundingBoxFn>(GameVersion::GetFunctionAddress("Gob", "GetMinimumBoundingBox_2"));
        getMinimumBoundingSphere = reinterpret_cast<GetMinimumBoundingSphereFn>(GameVersion::GetFunctionAddress("Gob", "GetMinimumBoundingSphere"));
        getModelName = reinterpret_cast<GetModelNameFn>(GameVersion::GetFunctionAddress("Gob", "GetModelName"));
        getMotionBlurred = reinterpret_cast<GetMotionBlurredFn>(GameVersion::GetFunctionAddress("Gob", "GetMotionBlurred"));
        getName = reinterpret_cast<GetNameFn>(GameVersion::GetFunctionAddress("Gob", "GetName"));
        getOrientation = reinterpret_cast<GetOrientationFn>(GameVersion::GetFunctionAddress("Gob", "GetOrientation"));
        getPartLocalPosition = reinterpret_cast<GetPartLocalPositionFn>(GameVersion::GetFunctionAddress("Gob", "GetPartLocalPosition"));
        getPosition = reinterpret_cast<GetPositionFn>(GameVersion::GetFunctionAddress("Gob", "GetPosition"));
        getPreviousPosition = reinterpret_cast<GetPreviousPositionFn>(GameVersion::GetFunctionAddress("Gob", "GetPreviousPosition"));
        isAlwaysRendering = reinterpret_cast<IsAlwaysRenderingFn>(GameVersion::GetFunctionAddress("Gob", "IsAlwaysRendering"));
        isAnimating = reinterpret_cast<IsAnimatingFn>(GameVersion::GetFunctionAddress("Gob", "IsAnimating"));
        isForceAnimated = reinterpret_cast<IsForceAnimatedFn>(GameVersion::GetFunctionAddress("Gob", "IsForceAnimated"));
        isInCutscene = reinterpret_cast<IsInCutsceneFn>(GameVersion::GetFunctionAddress("Gob", "IsInCutscene"));
        isInViewVolume = reinterpret_cast<IsInViewVolumeFn>(GameVersion::GetFunctionAddress("Gob", "IsInViewVolume"));
        loadAddInAnimations = reinterpret_cast<LoadAddInAnimationsFn>(GameVersion::GetFunctionAddress("Gob", "LoadAddInAnimations"));
        removeAddInAnimations = reinterpret_cast<RemoveAddInAnimationsFn>(GameVersion::GetFunctionAddress("Gob", "RemoveAddInAnimations"));
        removeAnimationFlag = reinterpret_cast<RemoveAnimationFlagFn>(GameVersion::GetFunctionAddress("Gob", "RemoveAnimationFlag"));
        removeAttachment = reinterpret_cast<RemoveAttachmentFn>(GameVersion::GetFunctionAddress("Gob", "RemoveAttachment"));
        render = reinterpret_cast<RenderFn>(GameVersion::GetFunctionAddress("Gob", "Render"));
        renderBlur = reinterpret_cast<RenderBlurFn>(GameVersion::GetFunctionAddress("Gob", "RenderBlur"));
        restoreTexture = reinterpret_cast<RestoreTextureFn>(GameVersion::GetFunctionAddress("Gob", "RestoreTexture"));
        setAsCharacter = reinterpret_cast<SetAsCharacterFn>(GameVersion::GetFunctionAddress("Gob", "SetAsCharacter"));
        setCanDownSample = reinterpret_cast<SetCanDownSampleFn>(GameVersion::GetFunctionAddress("Gob", "SetCanDownSample"));
        setColorShifting = reinterpret_cast<SetColorShiftingFn>(GameVersion::GetFunctionAddress("Gob", "SetColorShifting"));
        setEnvironmentMap = reinterpret_cast<SetEnvironmentMapFn>(GameVersion::GetFunctionAddress("Gob", "SetEnvironmentMap"));
        setIgnoreHitCheck = reinterpret_cast<SetIgnoreHitCheckFn>(GameVersion::GetFunctionAddress("Gob", "SetIgnoreHitCheck"));
        setIllumination = reinterpret_cast<SetIlluminationFn>(GameVersion::GetFunctionAddress("Gob", "SetIllumination"));
        setLightPriority = reinterpret_cast<SetLightPriorityFn>(GameVersion::GetFunctionAddress("Gob", "SetLightPriority"));
        setMaxBlurLength = reinterpret_cast<SetMaxBlurLengthFn>(GameVersion::GetFunctionAddress("Gob", "SetMaxBlurLength"));
        setMotionBlurred = reinterpret_cast<SetMotionBlurredFn>(GameVersion::GetFunctionAddress("Gob", "SetMotionBlurred"));
        setObjectScale = reinterpret_cast<SetObjectScaleFn>(GameVersion::GetFunctionAddress("Gob", "SetObjectScale"));
        setOrientation = reinterpret_cast<SetOrientationFn>(GameVersion::GetFunctionAddress("Gob", "SetOrientation"));
        setPosition = reinterpret_cast<SetPositionFn>(GameVersion::GetFunctionAddress("Gob", "SetPosition"));
        setProcessFlag = reinterpret_cast<SetProcessFlagFn>(GameVersion::GetFunctionAddress("Gob", "SetProcessFlag"));
        setRenderPersonalSpace = reinterpret_cast<SetRenderPersonalSpaceFn>(GameVersion::GetFunctionAddress("Gob", "SetRenderPersonalSpace"));
        setScene = reinterpret_cast<SetSceneFn>(GameVersion::GetFunctionAddress("Gob", "SetScene"));
        turnOffShadows = reinterpret_cast<TurnOffShadowsFn>(GameVersion::GetFunctionAddress("Gob", "TurnOffShadows"));
        turnOnShadows = reinterpret_cast<TurnOnShadowsFn>(GameVersion::GetFunctionAddress("Gob", "TurnOnShadows"));

        functionsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[Gob] ERROR: %s\n", e.what());
        return;
    }
}

void Gob::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[Gob] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        offsetScene = GameVersion::GetOffset("Gob", "scene");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[Gob] ERROR: %s\n", e.what());
    }
}

Gob::Gob(void* objectPtr)
    : GameAPIObject(objectPtr, false)  // false = don't free (wrapping existing)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

Gob::~Gob()
{
    // Base class destructor handles objectPtr cleanup
}

Scene* Gob::GetScene() {
    if (!objectPtr || offsetScene < 0) {
        return nullptr;
    }
    void* scenePtr = getObjectProperty<void*>(objectPtr, offsetScene);
    if (!scenePtr) {
        return nullptr;
    }
    return new Scene(scenePtr);
}

void Gob::AddAnimationFlag(unsigned long flag) {
    if (!objectPtr || !addAnimationFlag) return;
    addAnimationFlag(objectPtr, flag);
}

void Gob::AddAttachment(Gob* attachment) {
    if (!objectPtr || !addAttachment) return;
    addAttachment(objectPtr, attachment ? attachment->GetPtr() : nullptr);
}

int Gob::Amputate(char* partName) {
    if (!objectPtr || !amputate) return 0;
    return amputate(objectPtr, partName);
}

int Gob::Append(char* modelName, char* partName) {
    if (!objectPtr || !append) return 0;
    return append(objectPtr, modelName, partName);
}

int Gob::AttachToScene(Scene* scene) {
    if (!objectPtr || !attachToScene) return 0;
    return attachToScene(objectPtr, scene ? scene->GetPtr() : nullptr);
}

void Gob::BehaviorMessage(char* message) {
    if (!objectPtr || !behaviorMessage) return;
    behaviorMessage(objectPtr, message);
}

void Gob::DisableAlwaysRender() {
    if (!objectPtr || !disableAlwaysRender) return;
    disableAlwaysRender(objectPtr);
}

void Gob::DisableDistortion() {
    if (!objectPtr || !disableDistortion) return;
    disableDistortion(objectPtr);
}

void Gob::DisableFog() {
    if (!objectPtr || !disableFog) return;
    disableFog(objectPtr);
}

void Gob::DisableForceAnimations() {
    if (!objectPtr || !disableForceAnimations) return;
    disableForceAnimations(objectPtr);
}

void Gob::DisableInCutscene() {
    if (!objectPtr || !disableInCutscene) return;
    disableInCutscene(objectPtr);
}

void Gob::DisableInViewVolume() {
    if (!objectPtr || !disableInViewVolume) return;
    disableInViewVolume(objectPtr);
}

void Gob::DisableRenderBumpedOut() {
    if (!objectPtr || !disableRenderBumpedOut) return;
    disableRenderBumpedOut(objectPtr);
}

void Gob::DisableRenderGobBBox() {
    if (!objectPtr || !disableRenderGobBBox) return;
    disableRenderGobBBox(objectPtr);
}

void Gob::EnableAlwaysRender() {
    if (!objectPtr || !enableAlwaysRender) return;
    enableAlwaysRender(objectPtr);
}

void Gob::EnableDistortion() {
    if (!objectPtr || !enableDistortion) return;
    enableDistortion(objectPtr);
}

void Gob::EnableFog() {
    if (!objectPtr || !enableFog) return;
    enableFog(objectPtr);
}

void Gob::EnableForceAnimations() {
    if (!objectPtr || !enableForceAnimations) return;
    enableForceAnimations(objectPtr);
}

void Gob::EnableInCutscene() {
    if (!objectPtr || !enableInCutscene) return;
    enableInCutscene(objectPtr);
}

void Gob::EnableInViewVolume() {
    if (!objectPtr || !enableInViewVolume) return;
    enableInViewVolume(objectPtr);
}

void Gob::EnableRenderBumpedOut() {
    if (!objectPtr || !enableRenderBumpedOut) return;
    enableRenderBumpedOut(objectPtr);
}

// DB has only the pointer-param variant (no separate no-arg entry). The Vector* color param is unverified.
void Gob::EnableRenderGobBBox(Vector* color) {
    if (!objectPtr || !enableRenderGobBBox) return;
    enableRenderGobBBox(objectPtr, color);
}

void Gob::EndLookAtAnimate() {
    if (!objectPtr || !endLookAtAnimate) return;
    endLookAtAnimate(objectPtr);
}

void Gob::GetBoundingBox(Vector* min, Vector* max) {
    if (!objectPtr || !getBoundingBox) return;
    getBoundingBox(objectPtr, min, max);
}

Vector* Gob::GetCutsceneDummyPosition(Vector* outPosition) {
    if (!objectPtr || !getCutsceneDummyPosition) return nullptr;
    return getCutsceneDummyPosition(objectPtr, outPosition);
}

// x87 FPU call: the game returns the value in ST0 (typed here as double).
double Gob::GetMaxBlurLength() {
    if (!objectPtr || !getMaxBlurLength) return 0;
    return getMaxBlurLength(objectPtr);
}

// x87 FPU call: the game returns the value in ST0 (typed here as double).
double Gob::GetMaximumLightRadius() {
    if (!objectPtr || !getMaximumLightRadius) return 0;
    return getMaximumLightRadius(objectPtr);
}

// Maps to the DB's GetMinimumBoundingBox_2 (the 0x8-byte-param variant).
void Gob::GetMinimumBoundingBox(Vector* min, Vector* max) {
    if (!objectPtr || !getMinimumBoundingBox) return;
    getMinimumBoundingBox(objectPtr, min, max);
}

void Gob::GetMinimumBoundingSphere(Vector* center, float* radius) {
    if (!objectPtr || !getMinimumBoundingSphere) return;
    getMinimumBoundingSphere(objectPtr, center, radius);
}

CResRef* Gob::GetModelName() {
    if (!objectPtr || !getModelName) return nullptr;
    void* __ret = getModelName(objectPtr);
    return __ret ? new CResRef(__ret) : nullptr;
}

unsigned char Gob::GetMotionBlurred() {
    if (!objectPtr || !getMotionBlurred) return 0;
    return getMotionBlurred(objectPtr);
}

CResRef* Gob::GetName() {
    if (!objectPtr || !getName) return nullptr;
    void* __ret = getName(objectPtr);
    return __ret ? new CResRef(__ret) : nullptr;
}

void Gob::GetOrientation(Quaternion* orientation) {
    if (!objectPtr || !getOrientation) return;
    getOrientation(objectPtr, orientation);
}

int Gob::GetPartLocalPosition(Vector* outPosition, Quaternion* outOrientation) {
    if (!objectPtr || !getPartLocalPosition) return 0;
    return getPartLocalPosition(objectPtr, outPosition, outOrientation);
}

void Gob::GetPosition(Vector* outPosition) {
    if (!objectPtr || !getPosition) return;
    getPosition(objectPtr, outPosition);
}

void Gob::GetPreviousPosition(Vector* outPosition) {
    if (!objectPtr || !getPreviousPosition) return;
    getPreviousPosition(objectPtr, outPosition);
}

unsigned char Gob::IsAlwaysRendering() {
    if (!objectPtr || !isAlwaysRendering) return 0;
    return isAlwaysRendering(objectPtr);
}

unsigned char Gob::IsAnimating() {
    if (!objectPtr || !isAnimating) return 0;
    return isAnimating(objectPtr);
}

unsigned char Gob::IsForceAnimated() {
    if (!objectPtr || !isForceAnimated) return 0;
    return isForceAnimated(objectPtr);
}

unsigned char Gob::IsInCutscene() {
    if (!objectPtr || !isInCutscene) return 0;
    return isInCutscene(objectPtr);
}

unsigned char Gob::IsInViewVolume() {
    if (!objectPtr || !isInViewVolume) return 0;
    return isInViewVolume(objectPtr);
}

int Gob::LoadAddInAnimations(char* modelName) {
    if (!objectPtr || !loadAddInAnimations) return 0;
    return loadAddInAnimations(objectPtr, modelName);
}

void Gob::RemoveAddInAnimations() {
    if (!objectPtr || !removeAddInAnimations) return;
    removeAddInAnimations(objectPtr);
}

void Gob::RemoveAnimationFlag(int flag) {
    if (!objectPtr || !removeAnimationFlag) return;
    removeAnimationFlag(objectPtr, flag);
}

void Gob::RemoveAttachment(Gob* attachment) {
    if (!objectPtr || !removeAttachment) return;
    removeAttachment(objectPtr, attachment ? attachment->GetPtr() : nullptr);
}

void Gob::Render(bool cull) {
    if (!objectPtr || !render) return;
    render(objectPtr, cull);
}

void Gob::RenderBlur() {
    if (!objectPtr || !renderBlur) return;
    renderBlur(objectPtr);
}

int Gob::RestoreTexture() {
    if (!objectPtr || !restoreTexture) return 0;
    return restoreTexture(objectPtr);
}

void Gob::SetAsCharacter() {
    if (!objectPtr || !setAsCharacter) return;
    setAsCharacter(objectPtr);
}

int Gob::SetCanDownSample(bool canDownSample) {
    if (!objectPtr || !setCanDownSample) return 0;
    return setCanDownSample(objectPtr, canDownSample);
}

void Gob::SetColorShifting(Vector color, float alpha, int propagate) {
    if (!objectPtr || !setColorShifting) return;
    setColorShifting(objectPtr, color, alpha, propagate);
}

int Gob::SetEnvironmentMap(char* envMapName) {
    if (!objectPtr || !setEnvironmentMap) return 0;
    return setEnvironmentMap(objectPtr, envMapName);
}

void Gob::SetIgnoreHitCheck(bool ignore) {
    if (!objectPtr || !setIgnoreHitCheck) return;
    setIgnoreHitCheck(objectPtr, ignore);
}

void Gob::SetIllumination(Vector color, int propagate) {
    if (!objectPtr || !setIllumination) return;
    setIllumination(objectPtr, color, propagate);
}

void Gob::SetLightPriority(int priority) {
    if (!objectPtr || !setLightPriority) return;
    setLightPriority(objectPtr, priority);
}

void Gob::SetMaxBlurLength(float length) {
    if (!objectPtr || !setMaxBlurLength) return;
    setMaxBlurLength(objectPtr, length);
}

void Gob::SetMotionBlurred(bool motionBlurred) {
    if (!objectPtr || !setMotionBlurred) return;
    setMotionBlurred(objectPtr, motionBlurred);
}

int Gob::SetObjectScale(float scale, bool inherit) {
    if (!objectPtr || !setObjectScale) return 0;
    return setObjectScale(objectPtr, scale, inherit);
}

Quaternion* Gob::SetOrientation(Quaternion* outOrientation, Quaternion orientation) {
    if (!objectPtr || !setOrientation) return nullptr;
    return setOrientation(objectPtr, outOrientation, orientation);
}

void Gob::SetPosition(Vector* outPosition, Vector position) {
    if (!objectPtr || !setPosition) return;
    setPosition(objectPtr, outPosition, position);
}

void Gob::SetProcessFlag() {
    if (!objectPtr || !setProcessFlag) return;
    setProcessFlag(objectPtr);
}

void Gob::SetRenderPersonalSpace(bool render) {
    if (!objectPtr || !setRenderPersonalSpace) return;
    setRenderPersonalSpace(objectPtr, render);
}

void Gob::SetScene(Scene* scene) {
    if (!objectPtr || !setScene) return;
    setScene(objectPtr, scene ? scene->GetPtr() : nullptr);
}

void Gob::TurnOffShadows() {
    if (!objectPtr || !turnOffShadows) return;
    turnOffShadows(objectPtr);
}

void Gob::TurnOnShadows() {
    if (!objectPtr || !turnOnShadows) return;
    turnOnShadows(objectPtr);
}
