#pragma once

#include "../Common.h"
#include "CGameObject.h"

class CSWSObject;
class CExoString;
class CResRef;
class Scene;

/// <summary>
/// Wraps the client-side root of the game object hierarchy. Peer of CSWSObject;
/// holds the shared client state (position, orientation, portrait, model, visual
/// effects) and a back-pointer to its server counterpart.
/// </summary>
class CSWCObject : public CGameObject {
public:
    explicit CSWCObject(void* objectPtr);
    virtual ~CSWCObject();

    // Animation. Each takes a row index into animations.2da and returns
    // non-zero if the animation was accepted.
    int AnimationAttack(WORD animationRowIndex);
    int AnimationFireAndForget(WORD animationRowIndex);
    int AnimationHideEquippedItems(WORD animationRowIndex);
    int AnimationLooping(WORD animationRowIndex);
    int AnimationOverlay(WORD animationRowIndex);
    int AnimationParry(WORD animationRowIndex);
    int AnimationPause(WORD animationRowIndex);
    int AnimationPlayOutOfPlace(WORD animationRowIndex);
    int AnimationRunning(WORD animationRowIndex);
    int AnimationStationary(WORD animationRowIndex);
    int AnimationWalking(WORD animationRowIndex);
    // Takes a DWORD row index rather than a WORD, unlike the others.
    int AnimationDialog(DWORD animationRowIndex);

    WORD GetCurrentAnimation();
    WORD GetLoopingAnimation();
    void SetLoopingAnimation(WORD animation);

    // Server object linkage
    void AttachmentFromServerObject(CSWSObject* serverObject);
    void DetachFromServerObject();
    // Returns the paired server object as a heap-allocated wrapper; caller owns it.
    CSWSObject* GetServerObject();

    // Scene / rendering
    void AttachToScene(Scene* scene);
    int FadeObject();
    void ShowShadowBlob();
    void HideShadowBlob();
    void TurnOnShadows();
    void TurnOffShadows();

    // Actions
    void ClearAllActions();
    void ClearAllQueuedCombatActions();

    // Identity / presentation
    // NOTE: the address DB records this as __stdcall, but it is really __thiscall.
    // The function never touches ECX, so Ghidra could not see the 'this' pointer.
    // Either convention pushes one arg and returns with ret 4, so the two are
    // interchangeable here; __thiscall is used because it is what the game does.
    CExoString* GetName(CExoString* outString);
    void SetId(DWORD id);
    void SetFeedbackInfo(CExoString* info);
    // Fills outResRef and returns it wrapped; caller owns the returned wrapper.
    CResRef* GetPortrait(CResRef* outResRef);
    void SetPortrait(CResRef* portrait);
    WORD GetPortraitId();
    void SetPortraitId(WORD id);


    // ===== Offsets =====
    Vector GetPosition();
    void SetPosition(const Vector& value);
    Vector GetOrientation();
    void SetOrientation(const Vector& value);
    Vector GetGroundNormal();
    void SetGroundNormal(const Vector& value);

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    typedef int(__thiscall* AnimationAttackFn)(void* thisPtr, WORD animationRowIndex);
    typedef int(__thiscall* AnimationFireAndForgetFn)(void* thisPtr, WORD animationRowIndex);
    typedef int(__thiscall* AnimationHideEquippedItemsFn)(void* thisPtr, WORD animationRowIndex);
    typedef int(__thiscall* AnimationLoopingFn)(void* thisPtr, WORD animationRowIndex);
    typedef int(__thiscall* AnimationOverlayFn)(void* thisPtr, WORD animationRowIndex);
    typedef int(__thiscall* AnimationParryFn)(void* thisPtr, WORD animationRowIndex);
    typedef int(__thiscall* AnimationPauseFn)(void* thisPtr, WORD animationRowIndex);
    typedef int(__thiscall* AnimationPlayOutOfPlaceFn)(void* thisPtr, WORD animationRowIndex);
    typedef int(__thiscall* AnimationRunningFn)(void* thisPtr, WORD animationRowIndex);
    typedef int(__thiscall* AnimationStationaryFn)(void* thisPtr, WORD animationRowIndex);
    typedef int(__thiscall* AnimationWalkingFn)(void* thisPtr, WORD animationRowIndex);
    typedef int(__thiscall* AnimationDialogFn)(void* thisPtr, DWORD animationRowIndex);

    typedef WORD(__thiscall* GetCurrentAnimationFn)(void* thisPtr);
    typedef WORD(__thiscall* GetLoopingAnimationFn)(void* thisPtr);
    typedef void(__thiscall* SetLoopingAnimationFn)(void* thisPtr, WORD animation);

    typedef void(__thiscall* AttachmentFromServerObjectFn)(void* thisPtr, void* serverObject);
    typedef void(__thiscall* DetachFromServerObjectFn)(void* thisPtr);
    typedef void* (__thiscall* GetServerObjectFn)(void* thisPtr);

    typedef void(__thiscall* AttachToSceneFn)(void* thisPtr, void* scene);
    typedef int(__thiscall* FadeObjectFn)(void* thisPtr);
    typedef void(__thiscall* ShowShadowBlobFn)(void* thisPtr);
    typedef void(__thiscall* HideShadowBlobFn)(void* thisPtr);
    typedef void(__thiscall* TurnOnShadowsFn)(void* thisPtr);
    typedef void(__thiscall* TurnOffShadowsFn)(void* thisPtr);

    typedef void(__thiscall* ClearAllActionsFn)(void* thisPtr);
    typedef void(__thiscall* ClearAllQueuedCombatActionsFn)(void* thisPtr);

    typedef void* (__thiscall* GetNameFn)(void* thisPtr, void* outString);
    typedef void(__thiscall* SetIdFn)(void* thisPtr, DWORD id);
    typedef void(__thiscall* SetFeedbackInfoFn)(void* thisPtr, void* info);
    typedef void* (__thiscall* GetPortraitFn)(void* thisPtr, void* outResRef);
    typedef void(__thiscall* SetPortraitFn)(void* thisPtr, void* portrait);
    typedef WORD(__thiscall* GetPortraitIdFn)(void* thisPtr);
    typedef void(__thiscall* SetPortraitIdFn)(void* thisPtr, WORD id);

    static AnimationAttackFn animationAttack;
    static AnimationFireAndForgetFn animationFireAndForget;
    static AnimationHideEquippedItemsFn animationHideEquippedItems;
    static AnimationLoopingFn animationLooping;
    static AnimationOverlayFn animationOverlay;
    static AnimationParryFn animationParry;
    static AnimationPauseFn animationPause;
    static AnimationPlayOutOfPlaceFn animationPlayOutOfPlace;
    static AnimationRunningFn animationRunning;
    static AnimationStationaryFn animationStationary;
    static AnimationWalkingFn animationWalking;
    static AnimationDialogFn animationDialog;

    static GetCurrentAnimationFn getCurrentAnimation;
    static GetLoopingAnimationFn getLoopingAnimation;
    static SetLoopingAnimationFn setLoopingAnimation;

    static AttachmentFromServerObjectFn attachmentFromServerObject;
    static DetachFromServerObjectFn detachFromServerObject;
    static GetServerObjectFn getServerObject;

    static AttachToSceneFn attachToScene;
    static FadeObjectFn fadeObject;
    static ShowShadowBlobFn showShadowBlob;
    static HideShadowBlobFn hideShadowBlob;
    static TurnOnShadowsFn turnOnShadows;
    static TurnOffShadowsFn turnOffShadows;

    static ClearAllActionsFn clearAllActions;
    static ClearAllQueuedCombatActionsFn clearAllQueuedCombatActions;

    static GetNameFn getName;
    static SetIdFn setId;
    static SetFeedbackInfoFn setFeedbackInfo;
    static GetPortraitFn getPortrait;
    static SetPortraitFn setPortrait;
    static GetPortraitIdFn getPortraitId;
    static SetPortraitIdFn setPortraitId;

    static int offsetPosition;
    static int offsetOrientation;
    static int offsetGroundNormal;

    static bool functionsInitialized;
    static bool offsetsInitialized;
};
