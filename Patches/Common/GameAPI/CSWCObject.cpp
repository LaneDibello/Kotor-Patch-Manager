#include "CSWCObject.h"
#include "GameVersion.h"
#include "CSWSObject.h"
#include "CExoString.h"
#include "CResRef.h"
#include "Scene.h"

CSWCObject::AnimationAttackFn CSWCObject::animationAttack = nullptr;
CSWCObject::AnimationFireAndForgetFn CSWCObject::animationFireAndForget = nullptr;
CSWCObject::AnimationHideEquippedItemsFn CSWCObject::animationHideEquippedItems = nullptr;
CSWCObject::AnimationLoopingFn CSWCObject::animationLooping = nullptr;
CSWCObject::AnimationOverlayFn CSWCObject::animationOverlay = nullptr;
CSWCObject::AnimationParryFn CSWCObject::animationParry = nullptr;
CSWCObject::AnimationPauseFn CSWCObject::animationPause = nullptr;
CSWCObject::AnimationPlayOutOfPlaceFn CSWCObject::animationPlayOutOfPlace = nullptr;
CSWCObject::AnimationRunningFn CSWCObject::animationRunning = nullptr;
CSWCObject::AnimationStationaryFn CSWCObject::animationStationary = nullptr;
CSWCObject::AnimationWalkingFn CSWCObject::animationWalking = nullptr;
CSWCObject::AnimationDialogFn CSWCObject::animationDialog = nullptr;
CSWCObject::GetCurrentAnimationFn CSWCObject::getCurrentAnimation = nullptr;
CSWCObject::GetLoopingAnimationFn CSWCObject::getLoopingAnimation = nullptr;
CSWCObject::SetLoopingAnimationFn CSWCObject::setLoopingAnimation = nullptr;
CSWCObject::AttachmentFromServerObjectFn CSWCObject::attachmentFromServerObject = nullptr;
CSWCObject::DetachFromServerObjectFn CSWCObject::detachFromServerObject = nullptr;
CSWCObject::GetServerObjectFn CSWCObject::getServerObject = nullptr;
CSWCObject::AttachToSceneFn CSWCObject::attachToScene = nullptr;
CSWCObject::FadeObjectFn CSWCObject::fadeObject = nullptr;
CSWCObject::ShowShadowBlobFn CSWCObject::showShadowBlob = nullptr;
CSWCObject::HideShadowBlobFn CSWCObject::hideShadowBlob = nullptr;
CSWCObject::TurnOnShadowsFn CSWCObject::turnOnShadows = nullptr;
CSWCObject::TurnOffShadowsFn CSWCObject::turnOffShadows = nullptr;
CSWCObject::ClearAllActionsFn CSWCObject::clearAllActions = nullptr;
CSWCObject::ClearAllQueuedCombatActionsFn CSWCObject::clearAllQueuedCombatActions = nullptr;
CSWCObject::GetNameFn CSWCObject::getName = nullptr;
CSWCObject::SetIdFn CSWCObject::setId = nullptr;
CSWCObject::SetFeedbackInfoFn CSWCObject::setFeedbackInfo = nullptr;
CSWCObject::GetPortraitFn CSWCObject::getPortrait = nullptr;
CSWCObject::SetPortraitFn CSWCObject::setPortrait = nullptr;
CSWCObject::GetPortraitIdFn CSWCObject::getPortraitId = nullptr;
CSWCObject::SetPortraitIdFn CSWCObject::setPortraitId = nullptr;

bool CSWCObject::functionsInitialized = false;
int CSWCObject::offsetPosition = -1;
int CSWCObject::offsetOrientation = -1;
int CSWCObject::offsetGroundNormal = -1;
bool CSWCObject::offsetsInitialized = false;

void CSWCObject::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CGameObject::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWCObject] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        animationAttack = reinterpret_cast<AnimationAttackFn>(
            GameVersion::GetFunctionAddress("CSWCObject", "AnimationAttack"));
        animationFireAndForget = reinterpret_cast<AnimationFireAndForgetFn>(
            GameVersion::GetFunctionAddress("CSWCObject", "AnimationFireAndForget"));
        animationHideEquippedItems = reinterpret_cast<AnimationHideEquippedItemsFn>(
            GameVersion::GetFunctionAddress("CSWCObject", "AnimationHideEquippedItems"));
        animationLooping = reinterpret_cast<AnimationLoopingFn>(
            GameVersion::GetFunctionAddress("CSWCObject", "AnimationLooping"));
        animationOverlay = reinterpret_cast<AnimationOverlayFn>(
            GameVersion::GetFunctionAddress("CSWCObject", "AnimationOverlay"));
        animationParry = reinterpret_cast<AnimationParryFn>(
            GameVersion::GetFunctionAddress("CSWCObject", "AnimationParry"));
        animationPause = reinterpret_cast<AnimationPauseFn>(
            GameVersion::GetFunctionAddress("CSWCObject", "AnimationPause"));
        animationPlayOutOfPlace = reinterpret_cast<AnimationPlayOutOfPlaceFn>(
            GameVersion::GetFunctionAddress("CSWCObject", "AnimationPlayOutOfPlace"));
        animationRunning = reinterpret_cast<AnimationRunningFn>(
            GameVersion::GetFunctionAddress("CSWCObject", "AnimationRunning"));
        animationStationary = reinterpret_cast<AnimationStationaryFn>(
            GameVersion::GetFunctionAddress("CSWCObject", "AnimationStationary"));
        animationWalking = reinterpret_cast<AnimationWalkingFn>(
            GameVersion::GetFunctionAddress("CSWCObject", "AnimationWalking"));
        animationDialog = reinterpret_cast<AnimationDialogFn>(
            GameVersion::GetFunctionAddress("CSWCObject", "AnimationDialog"));
        getCurrentAnimation = reinterpret_cast<GetCurrentAnimationFn>(
            GameVersion::GetFunctionAddress("CSWCObject", "GetCurrentAnimation"));
        getLoopingAnimation = reinterpret_cast<GetLoopingAnimationFn>(
            GameVersion::GetFunctionAddress("CSWCObject", "GetLoopingAnimation"));
        setLoopingAnimation = reinterpret_cast<SetLoopingAnimationFn>(
            GameVersion::GetFunctionAddress("CSWCObject", "SetLoopingAnimation"));
        attachmentFromServerObject = reinterpret_cast<AttachmentFromServerObjectFn>(
            GameVersion::GetFunctionAddress("CSWCObject", "AttachmentFromServerObject"));
        detachFromServerObject = reinterpret_cast<DetachFromServerObjectFn>(
            GameVersion::GetFunctionAddress("CSWCObject", "DetachFromServerObject"));
        getServerObject = reinterpret_cast<GetServerObjectFn>(
            GameVersion::GetFunctionAddress("CSWCObject", "GetServerObject"));
        attachToScene = reinterpret_cast<AttachToSceneFn>(
            GameVersion::GetFunctionAddress("CSWCObject", "AttachToScene"));
        fadeObject = reinterpret_cast<FadeObjectFn>(
            GameVersion::GetFunctionAddress("CSWCObject", "FadeObject"));
        showShadowBlob = reinterpret_cast<ShowShadowBlobFn>(
            GameVersion::GetFunctionAddress("CSWCObject", "ShowShadowBlob"));
        hideShadowBlob = reinterpret_cast<HideShadowBlobFn>(
            GameVersion::GetFunctionAddress("CSWCObject", "HideShadowBlob"));
        turnOnShadows = reinterpret_cast<TurnOnShadowsFn>(
            GameVersion::GetFunctionAddress("CSWCObject", "TurnOnShadows"));
        turnOffShadows = reinterpret_cast<TurnOffShadowsFn>(
            GameVersion::GetFunctionAddress("CSWCObject", "TurnOffShadows"));
        clearAllActions = reinterpret_cast<ClearAllActionsFn>(
            GameVersion::GetFunctionAddress("CSWCObject", "ClearAllActions"));
        clearAllQueuedCombatActions = reinterpret_cast<ClearAllQueuedCombatActionsFn>(
            GameVersion::GetFunctionAddress("CSWCObject", "ClearAllQueuedCombatActions"));
        getName = reinterpret_cast<GetNameFn>(
            GameVersion::GetFunctionAddress("CSWCObject", "GetName"));
        setId = reinterpret_cast<SetIdFn>(
            GameVersion::GetFunctionAddress("CSWCObject", "SetId"));
        setFeedbackInfo = reinterpret_cast<SetFeedbackInfoFn>(
            GameVersion::GetFunctionAddress("CSWCObject", "SetFeedbackInfo"));
        getPortrait = reinterpret_cast<GetPortraitFn>(
            GameVersion::GetFunctionAddress("CSWCObject", "GetPortrait"));
        setPortrait = reinterpret_cast<SetPortraitFn>(
            GameVersion::GetFunctionAddress("CSWCObject", "SetPortrait"));
        getPortraitId = reinterpret_cast<GetPortraitIdFn>(
            GameVersion::GetFunctionAddress("CSWCObject", "GetPortraitId"));
        setPortraitId = reinterpret_cast<SetPortraitIdFn>(
            GameVersion::GetFunctionAddress("CSWCObject", "SetPortraitId"));
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWCObject] ERROR: %s\n", e.what());
        return;
    }

    functionsInitialized = true;
}

void CSWCObject::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CGameObject::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWCObject] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        offsetPosition = GameVersion::GetOffset("CSWCObject", "position");
        offsetOrientation = GameVersion::GetOffset("CSWCObject", "orientation");
        offsetGroundNormal = GameVersion::GetOffset("CSWCObject", "ground_normal");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWCObject] ERROR: %s\n", e.what());
    }
}

CSWCObject::CSWCObject(void* objectPtr)
    : CGameObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWCObject::~CSWCObject() {
    // Base class destructor handles objectPtr cleanup (we don't own the instance)
}

// ===== Animation =====

int CSWCObject::AnimationAttack(WORD animationRowIndex) {
    if (!objectPtr || !animationAttack) {
        return 0;
    }
    return animationAttack(objectPtr, animationRowIndex);
}

int CSWCObject::AnimationFireAndForget(WORD animationRowIndex) {
    if (!objectPtr || !animationFireAndForget) {
        return 0;
    }
    return animationFireAndForget(objectPtr, animationRowIndex);
}

int CSWCObject::AnimationHideEquippedItems(WORD animationRowIndex) {
    if (!objectPtr || !animationHideEquippedItems) {
        return 0;
    }
    return animationHideEquippedItems(objectPtr, animationRowIndex);
}

int CSWCObject::AnimationLooping(WORD animationRowIndex) {
    if (!objectPtr || !animationLooping) {
        return 0;
    }
    return animationLooping(objectPtr, animationRowIndex);
}

int CSWCObject::AnimationOverlay(WORD animationRowIndex) {
    if (!objectPtr || !animationOverlay) {
        return 0;
    }
    return animationOverlay(objectPtr, animationRowIndex);
}

int CSWCObject::AnimationParry(WORD animationRowIndex) {
    if (!objectPtr || !animationParry) {
        return 0;
    }
    return animationParry(objectPtr, animationRowIndex);
}

int CSWCObject::AnimationPause(WORD animationRowIndex) {
    if (!objectPtr || !animationPause) {
        return 0;
    }
    return animationPause(objectPtr, animationRowIndex);
}

int CSWCObject::AnimationPlayOutOfPlace(WORD animationRowIndex) {
    if (!objectPtr || !animationPlayOutOfPlace) {
        return 0;
    }
    return animationPlayOutOfPlace(objectPtr, animationRowIndex);
}

int CSWCObject::AnimationRunning(WORD animationRowIndex) {
    if (!objectPtr || !animationRunning) {
        return 0;
    }
    return animationRunning(objectPtr, animationRowIndex);
}

int CSWCObject::AnimationStationary(WORD animationRowIndex) {
    if (!objectPtr || !animationStationary) {
        return 0;
    }
    return animationStationary(objectPtr, animationRowIndex);
}

int CSWCObject::AnimationWalking(WORD animationRowIndex) {
    if (!objectPtr || !animationWalking) {
        return 0;
    }
    return animationWalking(objectPtr, animationRowIndex);
}

int CSWCObject::AnimationDialog(DWORD animationRowIndex) {
    if (!objectPtr || !animationDialog) {
        return 0;
    }
    return animationDialog(objectPtr, animationRowIndex);
}

WORD CSWCObject::GetCurrentAnimation() {
    if (!objectPtr || !getCurrentAnimation) {
        return 0;
    }
    return getCurrentAnimation(objectPtr);
}

WORD CSWCObject::GetLoopingAnimation() {
    if (!objectPtr || !getLoopingAnimation) {
        return 0;
    }
    return getLoopingAnimation(objectPtr);
}

void CSWCObject::SetLoopingAnimation(WORD animation) {
    if (!objectPtr || !setLoopingAnimation) {
        return;
    }
    setLoopingAnimation(objectPtr, animation);
}

// ===== Server object linkage =====

void CSWCObject::AttachmentFromServerObject(CSWSObject* serverObject) {
    if (!objectPtr || !attachmentFromServerObject) {
        return;
    }
    attachmentFromServerObject(objectPtr, serverObject ? serverObject->GetPtr() : nullptr);
}

void CSWCObject::DetachFromServerObject() {
    if (!objectPtr || !detachFromServerObject) {
        return;
    }
    detachFromServerObject(objectPtr);
}

CSWSObject* CSWCObject::GetServerObject() {
    if (!objectPtr || !getServerObject) {
        return nullptr;
    }

    void* serverPtr = getServerObject(objectPtr);
    if (!serverPtr) {
        return nullptr;
    }

    return new CSWSObject(serverPtr);
}

// ===== Scene / rendering =====

void CSWCObject::AttachToScene(Scene* scene) {
    if (!objectPtr || !attachToScene) {
        return;
    }
    attachToScene(objectPtr, scene ? scene->GetPtr() : nullptr);
}

int CSWCObject::FadeObject() {
    if (!objectPtr || !fadeObject) {
        return 0;
    }
    return fadeObject(objectPtr);
}

void CSWCObject::ShowShadowBlob() {
    if (!objectPtr || !showShadowBlob) {
        return;
    }
    showShadowBlob(objectPtr);
}

void CSWCObject::HideShadowBlob() {
    if (!objectPtr || !hideShadowBlob) {
        return;
    }
    hideShadowBlob(objectPtr);
}

void CSWCObject::TurnOnShadows() {
    if (!objectPtr || !turnOnShadows) {
        return;
    }
    turnOnShadows(objectPtr);
}

void CSWCObject::TurnOffShadows() {
    if (!objectPtr || !turnOffShadows) {
        return;
    }
    turnOffShadows(objectPtr);
}

// ===== Actions =====

void CSWCObject::ClearAllActions() {
    if (!objectPtr || !clearAllActions) {
        return;
    }
    clearAllActions(objectPtr);
}

void CSWCObject::ClearAllQueuedCombatActions() {
    if (!objectPtr || !clearAllQueuedCombatActions) {
        return;
    }
    clearAllQueuedCombatActions(objectPtr);
}

// ===== Identity / presentation =====

CExoString* CSWCObject::GetName(CExoString* outString) {
    if (!objectPtr || !getName) {
        return nullptr;
    }

    void* resultPtr = getName(objectPtr, outString ? outString->GetPtr() : nullptr);
    if (!resultPtr) {
        return nullptr;
    }

    return new CExoString(resultPtr);
}

void CSWCObject::SetId(DWORD id) {
    if (!objectPtr || !setId) {
        return;
    }
    setId(objectPtr, id);
}

void CSWCObject::SetFeedbackInfo(CExoString* info) {
    if (!objectPtr || !setFeedbackInfo) {
        return;
    }
    setFeedbackInfo(objectPtr, info ? info->GetPtr() : nullptr);
}

CResRef* CSWCObject::GetPortrait(CResRef* outResRef) {
    if (!objectPtr || !getPortrait) {
        return nullptr;
    }

    void* resultPtr = getPortrait(objectPtr, outResRef ? outResRef->GetPtr() : nullptr);
    if (!resultPtr) {
        return nullptr;
    }

    return new CResRef(resultPtr);
}

void CSWCObject::SetPortrait(CResRef* portrait) {
    if (!objectPtr || !setPortrait) {
        return;
    }
    setPortrait(objectPtr, portrait ? portrait->GetPtr() : nullptr);
}

WORD CSWCObject::GetPortraitId() {
    if (!objectPtr || !getPortraitId) {
        return 0;
    }
    return getPortraitId(objectPtr);
}

void CSWCObject::SetPortraitId(WORD id) {
    if (!objectPtr || !setPortraitId) {
        return;
    }
    setPortraitId(objectPtr, id);
}

// ===== Offsets =====

Vector CSWCObject::GetPosition() {
    Vector result = { 0.0f, 0.0f, 0.0f };

    if (!objectPtr || offsetPosition < 0) {
        return result;
    }

    return getObjectProperty<Vector>(objectPtr, offsetPosition);
}

void CSWCObject::SetPosition(const Vector& value) {
    if (!objectPtr || offsetPosition < 0) {
        return;
    }
    setObjectProperty<Vector>(objectPtr, offsetPosition, value);
}

Vector CSWCObject::GetOrientation() {
    Vector result = { 0.0f, 0.0f, 0.0f };

    if (!objectPtr || offsetOrientation < 0) {
        return result;
    }

    return getObjectProperty<Vector>(objectPtr, offsetOrientation);
}

void CSWCObject::SetOrientation(const Vector& value) {
    if (!objectPtr || offsetOrientation < 0) {
        return;
    }
    setObjectProperty<Vector>(objectPtr, offsetOrientation, value);
}

Vector CSWCObject::GetGroundNormal() {
    Vector result = { 0.0f, 0.0f, 0.0f };

    if (!objectPtr || offsetGroundNormal < 0) {
        return result;
    }

    return getObjectProperty<Vector>(objectPtr, offsetGroundNormal);
}

void CSWCObject::SetGroundNormal(const Vector& value) {
    if (!objectPtr || offsetGroundNormal < 0) {
        return;
    }
    setObjectProperty<Vector>(objectPtr, offsetGroundNormal, value);
}
