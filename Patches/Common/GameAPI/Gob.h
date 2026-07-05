#pragma once
#include "../Common.h"
#include "GameAPIObject.h"

class Scene;
class CResRef;

/// <summary>
/// GameAPI wrapper for the game's Gob type ("Game Object") - the helper the
/// engine uses to manage any 3D model within a Scene. Also aliased as CAurObject
/// (the name used in the game's own source; Gob is the internal Aurora name).
/// </summary>
class Gob : public GameAPIObject {
public:
    explicit Gob(void* objectPtr);
    ~Gob();

    // Accessors
    Scene* GetScene();

    // Functions
    void AddAnimationFlag(unsigned long flag);
    void AddAttachment(Gob* attachment);
    int Amputate(char* partName);
    int Append(char* modelName, char* partName);
    int AttachToScene(Scene* scene);
    void BehaviorMessage(char* message);
    void DisableAlwaysRender();
    void DisableDistortion();
    void DisableFog();
    void DisableForceAnimations();
    void DisableInCutscene();
    void DisableInViewVolume();
    void DisableRenderBumpedOut();
    void DisableRenderGobBBox();
    void EnableAlwaysRender();
    void EnableDistortion();
    void EnableFog();
    void EnableForceAnimations();
    void EnableInCutscene();
    void EnableInViewVolume();
    void EnableRenderBumpedOut();
    // DB has only the pointer-param variant (no separate no-arg entry). The Vector* color param is unverified.
    void EnableRenderGobBBox(Vector* color);
    void EndLookAtAnimate();
    void GetBoundingBox(Vector* min, Vector* max);
    Vector* GetCutsceneDummyPosition(Vector* outPosition);
    // x87 FPU call: the game returns the value in ST0 (typed here as double).
    double GetMaxBlurLength();
    // x87 FPU call: the game returns the value in ST0 (typed here as double).
    double GetMaximumLightRadius();
    // Maps to the DB's GetMinimumBoundingBox_2 (the 0x8-byte-param variant).
    void GetMinimumBoundingBox(Vector* min, Vector* max);
    void GetMinimumBoundingSphere(Vector* center, float* radius);
    CResRef* GetModelName();
    unsigned char GetMotionBlurred();
    CResRef* GetName();
    void GetOrientation(Quaternion* orientation);
    int GetPartLocalPosition(Vector* outPosition, Quaternion* outOrientation);
    void GetPosition(Vector* outPosition);
    void GetPreviousPosition(Vector* outPosition);
    unsigned char IsAlwaysRendering();
    unsigned char IsAnimating();
    unsigned char IsForceAnimated();
    unsigned char IsInCutscene();
    unsigned char IsInViewVolume();
    int LoadAddInAnimations(char* modelName);
    void RemoveAddInAnimations();
    void RemoveAnimationFlag(int flag);
    void RemoveAttachment(Gob* attachment);
    void Render(bool cull);
    void RenderBlur();
    int RestoreTexture();
    void SetAsCharacter();
    int SetCanDownSample(bool canDownSample);
    void SetColorShifting(Vector color, float alpha, int propagate);
    int SetEnvironmentMap(char* envMapName);
    void SetIgnoreHitCheck(bool ignore);
    void SetIllumination(Vector color, int propagate);
    void SetLightPriority(int priority);
    void SetMaxBlurLength(float length);
    void SetMotionBlurred(bool motionBlurred);
    int SetObjectScale(float scale, bool inherit);
    Quaternion* SetOrientation(Quaternion* outOrientation, Quaternion orientation);
    void SetPosition(Vector* outPosition, Vector position);
    void SetProcessFlag();
    void SetRenderPersonalSpace(bool render);
    void SetScene(Scene* scene);
    void TurnOffShadows();
    void TurnOnShadows();

    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    typedef void (__thiscall* AddAnimationFlagFn)(void* thisPtr, unsigned long flag);
    typedef void (__thiscall* AddAttachmentFn)(void* thisPtr, void* attachment);
    typedef int (__thiscall* AmputateFn)(void* thisPtr, char* partName);
    typedef int (__thiscall* AppendFn)(void* thisPtr, char* modelName, char* partName);
    typedef int (__thiscall* AttachToSceneFn)(void* thisPtr, void* scene);
    typedef void (__thiscall* BehaviorMessageFn)(void* thisPtr, char* message);
    typedef void (__thiscall* DisableAlwaysRenderFn)(void* thisPtr);
    typedef void (__thiscall* DisableDistortionFn)(void* thisPtr);
    typedef void (__thiscall* DisableFogFn)(void* thisPtr);
    typedef void (__thiscall* DisableForceAnimationsFn)(void* thisPtr);
    typedef void (__thiscall* DisableInCutsceneFn)(void* thisPtr);
    typedef void (__thiscall* DisableInViewVolumeFn)(void* thisPtr);
    typedef void (__thiscall* DisableRenderBumpedOutFn)(void* thisPtr);
    typedef void (__thiscall* DisableRenderGobBBoxFn)(void* thisPtr);
    typedef void (__thiscall* EnableAlwaysRenderFn)(void* thisPtr);
    typedef void (__thiscall* EnableDistortionFn)(void* thisPtr);
    typedef void (__thiscall* EnableFogFn)(void* thisPtr);
    typedef void (__thiscall* EnableForceAnimationsFn)(void* thisPtr);
    typedef void (__thiscall* EnableInCutsceneFn)(void* thisPtr);
    typedef void (__thiscall* EnableInViewVolumeFn)(void* thisPtr);
    typedef void (__thiscall* EnableRenderBumpedOutFn)(void* thisPtr);
    typedef void (__thiscall* EnableRenderGobBBoxFn)(void* thisPtr, Vector* color);
    typedef void (__thiscall* EndLookAtAnimateFn)(void* thisPtr);
    typedef void (__thiscall* GetBoundingBoxFn)(void* thisPtr, Vector* min, Vector* max);
    typedef Vector* (__thiscall* GetCutsceneDummyPositionFn)(void* thisPtr, Vector* outPosition);
    typedef double (__thiscall* GetMaxBlurLengthFn)(void* thisPtr);
    typedef double (__thiscall* GetMaximumLightRadiusFn)(void* thisPtr);
    typedef void (__thiscall* GetMinimumBoundingBoxFn)(void* thisPtr, Vector* min, Vector* max);
    typedef void (__thiscall* GetMinimumBoundingSphereFn)(void* thisPtr, Vector* center, float* radius);
    typedef CResRef* (__thiscall* GetModelNameFn)(void* thisPtr);
    typedef unsigned char (__thiscall* GetMotionBlurredFn)(void* thisPtr);
    typedef CResRef* (__thiscall* GetNameFn)(void* thisPtr);
    typedef void (__thiscall* GetOrientationFn)(void* thisPtr, Quaternion* orientation);
    typedef int (__thiscall* GetPartLocalPositionFn)(void* thisPtr, Vector* outPosition, Quaternion* outOrientation);
    typedef void (__thiscall* GetPositionFn)(void* thisPtr, Vector* outPosition);
    typedef void (__thiscall* GetPreviousPositionFn)(void* thisPtr, Vector* outPosition);
    typedef unsigned char (__thiscall* IsAlwaysRenderingFn)(void* thisPtr);
    typedef unsigned char (__thiscall* IsAnimatingFn)(void* thisPtr);
    typedef unsigned char (__thiscall* IsForceAnimatedFn)(void* thisPtr);
    typedef unsigned char (__thiscall* IsInCutsceneFn)(void* thisPtr);
    typedef unsigned char (__thiscall* IsInViewVolumeFn)(void* thisPtr);
    typedef int (__thiscall* LoadAddInAnimationsFn)(void* thisPtr, char* modelName);
    typedef void (__thiscall* RemoveAddInAnimationsFn)(void* thisPtr);
    typedef void (__thiscall* RemoveAnimationFlagFn)(void* thisPtr, int flag);
    typedef void (__thiscall* RemoveAttachmentFn)(void* thisPtr, void* attachment);
    typedef void (__thiscall* RenderFn)(void* thisPtr, bool cull);
    typedef void (__thiscall* RenderBlurFn)(void* thisPtr);
    typedef int (__thiscall* RestoreTextureFn)(void* thisPtr);
    typedef void (__thiscall* SetAsCharacterFn)(void* thisPtr);
    typedef int (__thiscall* SetCanDownSampleFn)(void* thisPtr, bool canDownSample);
    typedef void (__thiscall* SetColorShiftingFn)(void* thisPtr, Vector color, float alpha, int propagate);
    typedef int (__thiscall* SetEnvironmentMapFn)(void* thisPtr, char* envMapName);
    typedef void (__thiscall* SetIgnoreHitCheckFn)(void* thisPtr, bool ignore);
    typedef void (__thiscall* SetIlluminationFn)(void* thisPtr, Vector color, int propagate);
    typedef void (__thiscall* SetLightPriorityFn)(void* thisPtr, int priority);
    typedef void (__thiscall* SetMaxBlurLengthFn)(void* thisPtr, float length);
    typedef void (__thiscall* SetMotionBlurredFn)(void* thisPtr, bool motionBlurred);
    typedef int (__thiscall* SetObjectScaleFn)(void* thisPtr, float scale, bool inherit);
    typedef Quaternion* (__thiscall* SetOrientationFn)(void* thisPtr, Quaternion* outOrientation, Quaternion orientation);
    typedef void (__thiscall* SetPositionFn)(void* thisPtr, Vector* outPosition, Vector position);
    typedef void (__thiscall* SetProcessFlagFn)(void* thisPtr);
    typedef void (__thiscall* SetRenderPersonalSpaceFn)(void* thisPtr, bool render);
    typedef void (__thiscall* SetSceneFn)(void* thisPtr, void* scene);
    typedef void (__thiscall* TurnOffShadowsFn)(void* thisPtr);
    typedef void (__thiscall* TurnOnShadowsFn)(void* thisPtr);

    static AddAnimationFlagFn addAnimationFlag;
    static AddAttachmentFn addAttachment;
    static AmputateFn amputate;
    static AppendFn append;
    static AttachToSceneFn attachToScene;
    static BehaviorMessageFn behaviorMessage;
    static DisableAlwaysRenderFn disableAlwaysRender;
    static DisableDistortionFn disableDistortion;
    static DisableFogFn disableFog;
    static DisableForceAnimationsFn disableForceAnimations;
    static DisableInCutsceneFn disableInCutscene;
    static DisableInViewVolumeFn disableInViewVolume;
    static DisableRenderBumpedOutFn disableRenderBumpedOut;
    static DisableRenderGobBBoxFn disableRenderGobBBox;
    static EnableAlwaysRenderFn enableAlwaysRender;
    static EnableDistortionFn enableDistortion;
    static EnableFogFn enableFog;
    static EnableForceAnimationsFn enableForceAnimations;
    static EnableInCutsceneFn enableInCutscene;
    static EnableInViewVolumeFn enableInViewVolume;
    static EnableRenderBumpedOutFn enableRenderBumpedOut;
    static EnableRenderGobBBoxFn enableRenderGobBBox;
    static EndLookAtAnimateFn endLookAtAnimate;
    static GetBoundingBoxFn getBoundingBox;
    static GetCutsceneDummyPositionFn getCutsceneDummyPosition;
    static GetMaxBlurLengthFn getMaxBlurLength;
    static GetMaximumLightRadiusFn getMaximumLightRadius;
    static GetMinimumBoundingBoxFn getMinimumBoundingBox;
    static GetMinimumBoundingSphereFn getMinimumBoundingSphere;
    static GetModelNameFn getModelName;
    static GetMotionBlurredFn getMotionBlurred;
    static GetNameFn getName;
    static GetOrientationFn getOrientation;
    static GetPartLocalPositionFn getPartLocalPosition;
    static GetPositionFn getPosition;
    static GetPreviousPositionFn getPreviousPosition;
    static IsAlwaysRenderingFn isAlwaysRendering;
    static IsAnimatingFn isAnimating;
    static IsForceAnimatedFn isForceAnimated;
    static IsInCutsceneFn isInCutscene;
    static IsInViewVolumeFn isInViewVolume;
    static LoadAddInAnimationsFn loadAddInAnimations;
    static RemoveAddInAnimationsFn removeAddInAnimations;
    static RemoveAnimationFlagFn removeAnimationFlag;
    static RemoveAttachmentFn removeAttachment;
    static RenderFn render;
    static RenderBlurFn renderBlur;
    static RestoreTextureFn restoreTexture;
    static SetAsCharacterFn setAsCharacter;
    static SetCanDownSampleFn setCanDownSample;
    static SetColorShiftingFn setColorShifting;
    static SetEnvironmentMapFn setEnvironmentMap;
    static SetIgnoreHitCheckFn setIgnoreHitCheck;
    static SetIlluminationFn setIllumination;
    static SetLightPriorityFn setLightPriority;
    static SetMaxBlurLengthFn setMaxBlurLength;
    static SetMotionBlurredFn setMotionBlurred;
    static SetObjectScaleFn setObjectScale;
    static SetOrientationFn setOrientation;
    static SetPositionFn setPosition;
    static SetProcessFlagFn setProcessFlag;
    static SetRenderPersonalSpaceFn setRenderPersonalSpace;
    static SetSceneFn setScene;
    static TurnOffShadowsFn turnOffShadows;
    static TurnOnShadowsFn turnOnShadows;

    static bool functionsInitialized;
    static bool offsetsInitialized;

    static int offsetScene;
};

// The game refers to this type as CAurObject in some places; accept either name.
typedef Gob CAurObject;
