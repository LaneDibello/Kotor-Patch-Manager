#include "CGuiInGame.h"
#include "GameVersion.h"
#include "CExoString.h"
#include "CResRef.h"

CGuiInGame::AddFloatyTextFn CGuiInGame::addFloatyText = nullptr;
CGuiInGame::AppendToMsgBufferFn CGuiInGame::appendToMsgBuffer = nullptr;
CGuiInGame::GetMessageBufferSizeFn CGuiInGame::getMessageBufferSize = nullptr;
CGuiInGame::UpdateMessageGUIFn CGuiInGame::updateMessageGUI = nullptr;

CGuiInGame::ChangeCharacterFn CGuiInGame::changeCharacter = nullptr;
CGuiInGame::DoQuickLoadFn CGuiInGame::doQuickLoad = nullptr;
CGuiInGame::GetCurrentEquipScreenNPCFn CGuiInGame::getCurrentEquipScreenNPC = nullptr;

CGuiInGame::FadeAndStopDialogAmbientTrackFn CGuiInGame::fadeAndStopDialogAmbientTrack = nullptr;
CGuiInGame::GetCanHeadFollowInDialogFn CGuiInGame::getCanHeadFollowInDialog = nullptr;
CGuiInGame::GetCanOrientInDialogFn CGuiInGame::getCanOrientInDialog = nullptr;
CGuiInGame::IsAnimationPlayingInDialogFn CGuiInGame::isAnimationPlayingInDialog = nullptr;
CGuiInGame::IsCameraDialogFn CGuiInGame::isCameraDialog = nullptr;
CGuiInGame::IsFadingInDialogFn CGuiInGame::isFadingInDialog = nullptr;
CGuiInGame::LoadAmbientTrackForDialogFn CGuiInGame::loadAmbientTrackForDialog = nullptr;
CGuiInGame::LoadStuntModelsForDialogFn CGuiInGame::loadStuntModelsForDialog = nullptr;
CGuiInGame::ResetDialogAnimationsFn CGuiInGame::resetDialogAnimations = nullptr;
CGuiInGame::ResetTalkAnimationsFn CGuiInGame::resetTalkAnimations = nullptr;
CGuiInGame::UpdateDialogFn CGuiInGame::updateDialog = nullptr;
CGuiInGame::UpdateDialogLipSyncFn CGuiInGame::updateDialogLipSync = nullptr;
CGuiInGame::GetCameraAnimationNameFn CGuiInGame::getCameraAnimationName = nullptr;

CGuiInGame::GetAreaTranstionPositionFn CGuiInGame::getAreaTranstionPosition = nullptr;
CGuiInGame::HideAreaTransitionFn CGuiInGame::hideAreaTransition = nullptr;
CGuiInGame::UpdateAreaTransitionGUIFn CGuiInGame::updateAreaTransitionGUI = nullptr;

CGuiInGame::GetFadeOverridePanelVisibleFn CGuiInGame::getFadeOverridePanelVisible = nullptr;
CGuiInGame::IsGlobalFadeObscuringFn CGuiInGame::isGlobalFadeObscuring = nullptr;
CGuiInGame::IsGlobalFadingFn CGuiInGame::isGlobalFading = nullptr;
CGuiInGame::StopGlobalFadeFn CGuiInGame::stopGlobalFade = nullptr;

CGuiInGame::GetBarkBubbleVisibleFn CGuiInGame::getBarkBubbleVisible = nullptr;
CGuiInGame::GetCanClickFn CGuiInGame::getCanClick = nullptr;
CGuiInGame::GetMiniMapVisibleFn CGuiInGame::getMiniMapVisible = nullptr;

CGuiInGame::GetPartyAccessPanelUpFn CGuiInGame::getPartyAccessPanelUp = nullptr;
CGuiInGame::GetPendingStatusSummaryFn CGuiInGame::getPendingStatusSummary = nullptr;
CGuiInGame::ShowPartySelectionFn CGuiInGame::showPartySelection = nullptr;
CGuiInGame::ShowStatusSummaryFn CGuiInGame::showStatusSummary = nullptr;

CGuiInGame::HideContainerGuiFn CGuiInGame::hideContainerGui = nullptr;
CGuiInGame::HideGalaxyMapGuiFn CGuiInGame::hideGalaxyMapGui = nullptr;
CGuiInGame::HideItemCreateMenuFn CGuiInGame::hideItemCreateMenu = nullptr;
CGuiInGame::HideLoadModuleDebugMenuFn CGuiInGame::hideLoadModuleDebugMenu = nullptr;
CGuiInGame::HidePowersFeatsSkillsDebugMenuFn CGuiInGame::hidePowersFeatsSkillsDebugMenu = nullptr;
CGuiInGame::HideSoloModeFn CGuiInGame::hideSoloMode = nullptr;
CGuiInGame::HideStoreGuiFn CGuiInGame::hideStoreGui = nullptr;
CGuiInGame::HideSubItemCreateMenuFn CGuiInGame::hideSubItemCreateMenu = nullptr;
CGuiInGame::HideUpgradeScreenFn CGuiInGame::hideUpgradeScreen = nullptr;
CGuiInGame::SetLevelUpModeFn CGuiInGame::setLevelUpMode = nullptr;
CGuiInGame::ShowGalaxyMapGuiFn CGuiInGame::showGalaxyMapGui = nullptr;
CGuiInGame::ShowItemCreateMenuFn CGuiInGame::showItemCreateMenu = nullptr;
CGuiInGame::ShowSoloModeQueryFn CGuiInGame::showSoloModeQuery = nullptr;
CGuiInGame::ShowUpgradeScreenFn CGuiInGame::showUpgradeScreen = nullptr;

CGuiInGame::NextSWInGameGuiFn CGuiInGame::nextSWInGameGui = nullptr;
CGuiInGame::PrevSWInGameGuiFn CGuiInGame::prevSWInGameGui = nullptr;
CGuiInGame::SwitchToSWInGameGuiFn CGuiInGame::switchToSWInGameGui = nullptr;

CGuiInGame::RemoveGUIPopUpsFn CGuiInGame::removeGUIPopUps = nullptr;
CGuiInGame::RePopulateMainInterfaceFn CGuiInGame::rePopulateMainInterface = nullptr;
CGuiInGame::ResetInterfaceForSizeFn CGuiInGame::resetInterfaceForSize = nullptr;

CGuiInGame::ShutDownFn CGuiInGame::shutDown = nullptr;
CGuiInGame::ShutDownGlobalFn CGuiInGame::shutDownGlobal = nullptr;

bool CGuiInGame::functionsInitialized = false;
bool CGuiInGame::offsetsInitialized = false;

void CGuiInGame::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CGuiInGame] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        addFloatyText = reinterpret_cast<AddFloatyTextFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "AddFloatyText"));
        appendToMsgBuffer = reinterpret_cast<AppendToMsgBufferFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "AppendToMsgBuffer"));
        getMessageBufferSize = reinterpret_cast<GetMessageBufferSizeFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "GetMessageBufferSize"));
        updateMessageGUI = reinterpret_cast<UpdateMessageGUIFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "UpdateMessageGUI"));

        changeCharacter = reinterpret_cast<ChangeCharacterFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "ChangeCharacter"));
        doQuickLoad = reinterpret_cast<DoQuickLoadFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "DoQuickLoad"));
        getCurrentEquipScreenNPC = reinterpret_cast<GetCurrentEquipScreenNPCFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "GetCurrentEquipScreenNPC"));

        fadeAndStopDialogAmbientTrack = reinterpret_cast<FadeAndStopDialogAmbientTrackFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "FadeAndStopDialogAmbientTrack"));
        getCanHeadFollowInDialog = reinterpret_cast<GetCanHeadFollowInDialogFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "GetCanHeadFollowInDialog"));
        getCanOrientInDialog = reinterpret_cast<GetCanOrientInDialogFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "GetCanOrientInDialog"));
        isAnimationPlayingInDialog = reinterpret_cast<IsAnimationPlayingInDialogFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "IsAnimationPlayingInDialog"));
        isCameraDialog = reinterpret_cast<IsCameraDialogFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "IsCameraDialog"));
        isFadingInDialog = reinterpret_cast<IsFadingInDialogFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "IsFadingInDialog"));
        loadAmbientTrackForDialog = reinterpret_cast<LoadAmbientTrackForDialogFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "LoadAmbientTrackForDialog"));
        loadStuntModelsForDialog = reinterpret_cast<LoadStuntModelsForDialogFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "LoadStuntModelsForDialog"));
        resetDialogAnimations = reinterpret_cast<ResetDialogAnimationsFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "ResetDialogAnimations"));
        resetTalkAnimations = reinterpret_cast<ResetTalkAnimationsFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "ResetTalkAnimations"));
        updateDialog = reinterpret_cast<UpdateDialogFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "UpdateDialog"));
        updateDialogLipSync = reinterpret_cast<UpdateDialogLipSyncFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "UpdateDialogLipSync"));
        getCameraAnimationName = reinterpret_cast<GetCameraAnimationNameFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "GetCameraAnimationName"));

        getAreaTranstionPosition = reinterpret_cast<GetAreaTranstionPositionFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "GetAreaTranstionPosition"));
        hideAreaTransition = reinterpret_cast<HideAreaTransitionFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "HideAreaTransition"));
        updateAreaTransitionGUI = reinterpret_cast<UpdateAreaTransitionGUIFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "UpdateAreaTransitionGUI"));

        getFadeOverridePanelVisible = reinterpret_cast<GetFadeOverridePanelVisibleFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "GetFadeOverridePanelVisible"));
        isGlobalFadeObscuring = reinterpret_cast<IsGlobalFadeObscuringFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "IsGlobalFadeObscuring"));
        isGlobalFading = reinterpret_cast<IsGlobalFadingFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "IsGlobalFading"));
        stopGlobalFade = reinterpret_cast<StopGlobalFadeFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "StopGlobalFade"));

        getBarkBubbleVisible = reinterpret_cast<GetBarkBubbleVisibleFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "GetBarkBubbleVisible"));
        getCanClick = reinterpret_cast<GetCanClickFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "GetCanClick"));
        getMiniMapVisible = reinterpret_cast<GetMiniMapVisibleFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "GetMiniMapVisible"));

        getPartyAccessPanelUp = reinterpret_cast<GetPartyAccessPanelUpFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "GetPartyAccessPanelUp"));
        getPendingStatusSummary = reinterpret_cast<GetPendingStatusSummaryFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "GetPendingStatusSummary"));
        showPartySelection = reinterpret_cast<ShowPartySelectionFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "ShowPartySelection"));
        showStatusSummary = reinterpret_cast<ShowStatusSummaryFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "ShowStatusSummary"));

        hideContainerGui = reinterpret_cast<HideContainerGuiFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "HideContainerGui"));
        hideGalaxyMapGui = reinterpret_cast<HideGalaxyMapGuiFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "HideGalaxyMapGui"));
        hideItemCreateMenu = reinterpret_cast<HideItemCreateMenuFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "HideItemCreateMenu"));
        hideLoadModuleDebugMenu = reinterpret_cast<HideLoadModuleDebugMenuFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "HideLoadModuleDebugMenu"));
        hidePowersFeatsSkillsDebugMenu = reinterpret_cast<HidePowersFeatsSkillsDebugMenuFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "HidePowersFeatsSkillsDebugMenu"));
        hideSoloMode = reinterpret_cast<HideSoloModeFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "HideSoloMode"));
        hideStoreGui = reinterpret_cast<HideStoreGuiFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "HideStoreGui"));
        hideSubItemCreateMenu = reinterpret_cast<HideSubItemCreateMenuFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "HideSubItemCreateMenu"));
        hideUpgradeScreen = reinterpret_cast<HideUpgradeScreenFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "HideUpgradeScreen"));
        setLevelUpMode = reinterpret_cast<SetLevelUpModeFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "SetLevelUpMode"));
        showGalaxyMapGui = reinterpret_cast<ShowGalaxyMapGuiFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "ShowGalaxyMapGui"));
        showItemCreateMenu = reinterpret_cast<ShowItemCreateMenuFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "ShowItemCreateMenu"));
        showSoloModeQuery = reinterpret_cast<ShowSoloModeQueryFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "ShowSoloModeQuery"));
        showUpgradeScreen = reinterpret_cast<ShowUpgradeScreenFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "ShowUpgradeScreen"));

        nextSWInGameGui = reinterpret_cast<NextSWInGameGuiFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "NextSWInGameGui"));
        prevSWInGameGui = reinterpret_cast<PrevSWInGameGuiFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "PrevSWInGameGui"));
        switchToSWInGameGui = reinterpret_cast<SwitchToSWInGameGuiFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "SwitchToSWInGameGui"));

        removeGUIPopUps = reinterpret_cast<RemoveGUIPopUpsFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "RemoveGUIPopUps"));
        rePopulateMainInterface = reinterpret_cast<RePopulateMainInterfaceFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "RePopulateMainInterface"));
        resetInterfaceForSize = reinterpret_cast<ResetInterfaceForSizeFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "ResetInterfaceForSize"));

        shutDown = reinterpret_cast<ShutDownFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "ShutDown"));
        shutDownGlobal = reinterpret_cast<ShutDownGlobalFn>(
            GameVersion::GetFunctionAddress("CGuiInGame", "ShutDownGlobal"));
    }
    catch (const GameVersionException& e) {
        debugLog("[CGuiInGame] ERROR: %s\n", e.what());
        return;
    }

    functionsInitialized = true;
}

void CGuiInGame::InitializeOffsets() {
    // CGuiInGame exposes no offsets through this wrapper (functions only)
    offsetsInitialized = true;
}

CGuiInGame::CGuiInGame(void* objectPtr)
    : GameAPIObject(objectPtr, false)  // false = don't free (owned by the game)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CGuiInGame::~CGuiInGame() {
    // Base class destructor handles objectPtr cleanup (we don't own the instance)
}

// ===== Floaty text / message buffer =====

void CGuiInGame::AddFloatyText(DWORD clientId, CExoString* text, Vector* color, float duration) {
    if (!objectPtr || !addFloatyText) {
        return;
    }
    addFloatyText(objectPtr, clientId, text ? text->GetPtr() : nullptr, color, duration);
}

void CGuiInGame::AppendToMsgBuffer(CExoString* messageString, DWORD messageType, BYTE messageColor) {
    if (!objectPtr || !appendToMsgBuffer) {
        return;
    }
    appendToMsgBuffer(objectPtr, messageString ? messageString->GetPtr() : nullptr, messageType, messageColor);
}

int CGuiInGame::GetMessageBufferSize() {
    if (!objectPtr || !getMessageBufferSize) {
        return 0;
    }
    return getMessageBufferSize(objectPtr);
}

void CGuiInGame::UpdateMessageGUI() {
    if (!objectPtr || !updateMessageGUI) {
        return;
    }
    updateMessageGUI(objectPtr);
}

// ===== Character / save-load =====

void CGuiInGame::ChangeCharacter() {
    if (!objectPtr || !changeCharacter) {
        return;
    }
    changeCharacter(objectPtr);
}

void CGuiInGame::DoQuickLoad() {
    if (!objectPtr || !doQuickLoad) {
        return;
    }
    doQuickLoad(objectPtr);
}

int CGuiInGame::GetCurrentEquipScreenNPC() {
    if (!objectPtr || !getCurrentEquipScreenNPC) {
        return 0;
    }
    return getCurrentEquipScreenNPC(objectPtr);
}

// ===== Dialog =====

void CGuiInGame::FadeAndStopDialogAmbientTrack() {
    if (!objectPtr || !fadeAndStopDialogAmbientTrack) {
        return;
    }
    fadeAndStopDialogAmbientTrack(objectPtr);
}

int CGuiInGame::GetCanHeadFollowInDialog(DWORD objectId) {
    if (!objectPtr || !getCanHeadFollowInDialog) {
        return 0;
    }
    return getCanHeadFollowInDialog(objectPtr, objectId);
}

int CGuiInGame::GetCanOrientInDialog(DWORD clientId) {
    if (!objectPtr || !getCanOrientInDialog) {
        return 0;
    }
    return getCanOrientInDialog(objectPtr, clientId);
}

int CGuiInGame::IsAnimationPlayingInDialog() {
    if (!objectPtr || !isAnimationPlayingInDialog) {
        return 0;
    }
    return isAnimationPlayingInDialog(objectPtr);
}

int CGuiInGame::IsCameraDialog() {
    if (!objectPtr || !isCameraDialog) {
        return 0;
    }
    return isCameraDialog(objectPtr);
}

int CGuiInGame::IsFadingInDialog() {
    if (!objectPtr || !isFadingInDialog) {
        return 0;
    }
    return isFadingInDialog(objectPtr);
}

void CGuiInGame::LoadAmbientTrackForDialog(CResRef* track) {
    if (!objectPtr || !loadAmbientTrackForDialog) {
        return;
    }
    loadAmbientTrackForDialog(objectPtr, track ? track->GetPtr() : nullptr);
}

int CGuiInGame::LoadStuntModelsForDialog() {
    if (!objectPtr || !loadStuntModelsForDialog) {
        return 0;
    }
    return loadStuntModelsForDialog(objectPtr);
}

void CGuiInGame::ResetDialogAnimations() {
    if (!objectPtr || !resetDialogAnimations) {
        return;
    }
    resetDialogAnimations(objectPtr);
}

void CGuiInGame::ResetTalkAnimations() {
    if (!objectPtr || !resetTalkAnimations) {
        return;
    }
    resetTalkAnimations(objectPtr);
}

void CGuiInGame::UpdateDialog() {
    if (!objectPtr || !updateDialog) {
        return;
    }
    updateDialog(objectPtr);
}

void CGuiInGame::UpdateDialogLipSync() {
    if (!objectPtr || !updateDialogLipSync) {
        return;
    }
    updateDialogLipSync(objectPtr);
}

CExoString* CGuiInGame::GetCameraAnimationName(CExoString* outName, short animationId) {
    // Static game helper (__stdcall): does not take a 'this' pointer.
    if (!objectPtr || !getCameraAnimationName || !outName) {
        return nullptr;
    }
    void* resultPtr = getCameraAnimationName(outName->GetPtr(), animationId);
    if (!resultPtr) {
        return nullptr;
    }
    return new CExoString(resultPtr);
}

// ===== Area transition =====

Vector* CGuiInGame::GetAreaTranstionPosition(Vector* out) {
    if (!objectPtr || !getAreaTranstionPosition || !out) {
        return nullptr;
    }
    return getAreaTranstionPosition(objectPtr, out);
}

void CGuiInGame::HideAreaTransition() {
    if (!objectPtr || !hideAreaTransition) {
        return;
    }
    hideAreaTransition(objectPtr);
}

void CGuiInGame::UpdateAreaTransitionGUI() {
    if (!objectPtr || !updateAreaTransitionGUI) {
        return;
    }
    updateAreaTransitionGUI(objectPtr);
}

// ===== Fade =====

int CGuiInGame::GetFadeOverridePanelVisible() {
    if (!objectPtr || !getFadeOverridePanelVisible) {
        return 0;
    }
    return getFadeOverridePanelVisible(objectPtr);
}

int CGuiInGame::IsGlobalFadeObscuring() {
    if (!objectPtr || !isGlobalFadeObscuring) {
        return 0;
    }
    return isGlobalFadeObscuring(objectPtr);
}

int CGuiInGame::IsGlobalFading() {
    if (!objectPtr || !isGlobalFading) {
        return 0;
    }
    return isGlobalFading(objectPtr);
}

void CGuiInGame::StopGlobalFade() {
    if (!objectPtr || !stopGlobalFade) {
        return;
    }
    stopGlobalFade(objectPtr);
}

// ===== Bark bubble / minimap / input =====

int CGuiInGame::GetBarkBubbleVisible(int* outBottomY) {
    if (!objectPtr || !getBarkBubbleVisible) {
        return 0;
    }
    return getBarkBubbleVisible(objectPtr, outBottomY);
}

int CGuiInGame::GetCanClick() {
    if (!objectPtr || !getCanClick) {
        return 0;
    }
    return getCanClick(objectPtr);
}

int CGuiInGame::GetMiniMapVisible() {
    if (!objectPtr || !getMiniMapVisible) {
        return 0;
    }
    return getMiniMapVisible(objectPtr);
}

// ===== Party / status summary =====

int CGuiInGame::GetPartyAccessPanelUp(int checkInventory) {
    if (!objectPtr || !getPartyAccessPanelUp) {
        return 0;
    }
    return getPartyAccessPanelUp(objectPtr, checkInventory);
}

int CGuiInGame::GetPendingStatusSummary() {
    if (!objectPtr || !getPendingStatusSummary) {
        return 0;
    }
    return getPendingStatusSummary(objectPtr);
}

void CGuiInGame::ShowPartySelection(CExoString* exitScript, int requireAccept, int forcedNPC1, int forcedNPC2) {
    if (!objectPtr || !showPartySelection) {
        return;
    }
    showPartySelection(objectPtr, exitScript ? exitScript->GetPtr() : nullptr, requireAccept, forcedNPC1, forcedNPC2);
}

void CGuiInGame::ShowStatusSummary() {
    if (!objectPtr || !showStatusSummary) {
        return;
    }
    showStatusSummary(objectPtr);
}

// ===== Screens / menus =====

void CGuiInGame::HideContainerGui() {
    if (!objectPtr || !hideContainerGui) {
        return;
    }
    hideContainerGui(objectPtr);
}

void CGuiInGame::HideGalaxyMapGui() {
    if (!objectPtr || !hideGalaxyMapGui) {
        return;
    }
    hideGalaxyMapGui(objectPtr);
}

void CGuiInGame::HideItemCreateMenu() {
    if (!objectPtr || !hideItemCreateMenu) {
        return;
    }
    hideItemCreateMenu(objectPtr);
}

void CGuiInGame::HideLoadModuleDebugMenu() {
    if (!objectPtr || !hideLoadModuleDebugMenu) {
        return;
    }
    hideLoadModuleDebugMenu(objectPtr);
}

void CGuiInGame::HidePowersFeatsSkillsDebugMenu() {
    if (!objectPtr || !hidePowersFeatsSkillsDebugMenu) {
        return;
    }
    hidePowersFeatsSkillsDebugMenu(objectPtr);
}

void CGuiInGame::HideSoloMode() {
    if (!objectPtr || !hideSoloMode) {
        return;
    }
    hideSoloMode(objectPtr);
}

void CGuiInGame::HideStoreGui() {
    if (!objectPtr || !hideStoreGui) {
        return;
    }
    hideStoreGui(objectPtr);
}

void CGuiInGame::HideSubItemCreateMenu() {
    if (!objectPtr || !hideSubItemCreateMenu) {
        return;
    }
    hideSubItemCreateMenu(objectPtr);
}

void CGuiInGame::HideUpgradeScreen() {
    if (!objectPtr || !hideUpgradeScreen) {
        return;
    }
    hideUpgradeScreen(objectPtr);
}

void CGuiInGame::SetLevelUpMode(int mode) {
    if (!objectPtr || !setLevelUpMode) {
        return;
    }
    setLevelUpMode(objectPtr, mode);
}

void CGuiInGame::ShowGalaxyMapGui(int selectedPlanetId) {
    if (!objectPtr || !showGalaxyMapGui) {
        return;
    }
    showGalaxyMapGui(objectPtr, selectedPlanetId);
}

void CGuiInGame::ShowItemCreateMenu() {
    if (!objectPtr || !showItemCreateMenu) {
        return;
    }
    showItemCreateMenu(objectPtr);
}

void CGuiInGame::ShowSoloModeQuery(int doStealth) {
    if (!objectPtr || !showSoloModeQuery) {
        return;
    }
    showSoloModeQuery(objectPtr, doStealth);
}

void CGuiInGame::ShowUpgradeScreen(DWORD itemId) {
    if (!objectPtr || !showUpgradeScreen) {
        return;
    }
    showUpgradeScreen(objectPtr, itemId);
}

// ===== SWInGameGui navigation =====

void CGuiInGame::NextSWInGameGui() {
    if (!objectPtr || !nextSWInGameGui) {
        return;
    }
    nextSWInGameGui(objectPtr);
}

void CGuiInGame::PrevSWInGameGui() {
    if (!objectPtr || !prevSWInGameGui) {
        return;
    }
    prevSWInGameGui(objectPtr);
}

void CGuiInGame::SwitchToSWInGameGui(int guiId) {
    if (!objectPtr || !switchToSWInGameGui) {
        return;
    }
    switchToSWInGameGui(objectPtr, guiId);
}

// ===== Main interface =====

void CGuiInGame::RemoveGUIPopUps() {
    if (!objectPtr || !removeGUIPopUps) {
        return;
    }
    removeGUIPopUps(objectPtr);
}

void CGuiInGame::RePopulateMainInterface() {
    if (!objectPtr || !rePopulateMainInterface) {
        return;
    }
    rePopulateMainInterface(objectPtr);
}

void CGuiInGame::ResetInterfaceForSize() {
    if (!objectPtr || !resetInterfaceForSize) {
        return;
    }
    resetInterfaceForSize(objectPtr);
}

// ===== Lifecycle =====

void CGuiInGame::ShutDown() {
    if (!objectPtr || !shutDown) {
        return;
    }
    shutDown(objectPtr);
}

void CGuiInGame::ShutDownGlobal() {
    if (!objectPtr || !shutDownGlobal) {
        return;
    }
    shutDownGlobal(objectPtr);
}
