#pragma once
#include "../Common.h"
#include "GameAPIObject.h"

class CExoString;
class CResRef;

/// <summary>
/// Wraps the game's CGuiInGame, a stand-alone object that tracks and manages
/// state for the various in-game GUI screens (dialog, area transitions, fades,
/// the message buffer, galaxy map, party selection, upgrade/store screens, etc.).
/// Does not derive from CSWGuiObject. Instances are normally obtained via
/// CClientExoApp::GetInGameGui(); the explicit constructor wraps an existing
/// CGuiInGame pointer.
///
/// NOTE: CGuiInGame is only present in the KotOR 1 address database. On KotOR 2
/// the function lookups fail gracefully and every wrapper becomes a safe no-op.
/// </summary>
class CGuiInGame : public GameAPIObject {
public:
    // Wraps an existing CGuiInGame pointer (does not take ownership).
    explicit CGuiInGame(void* objectPtr);
    ~CGuiInGame();

    // Floaty text / message buffer
    void AddFloatyText(DWORD clientId, CExoString* text, Vector* color, float duration);
    void AppendToMsgBuffer(CExoString* messageString, DWORD messageType, BYTE messageColor);
    int GetMessageBufferSize();
    void UpdateMessageGUI();

    // Character / save-load
    void ChangeCharacter();
    void DoQuickLoad();
    int GetCurrentEquipScreenNPC();

    // Dialog
    void FadeAndStopDialogAmbientTrack();
    int GetCanHeadFollowInDialog(DWORD objectId);
    int GetCanOrientInDialog(DWORD clientId);
    int IsAnimationPlayingInDialog();
    int IsCameraDialog();
    int IsFadingInDialog();
    void LoadAmbientTrackForDialog(CResRef* track);
    int LoadStuntModelsForDialog();
    void ResetDialogAnimations();
    void ResetTalkAnimations();
    void UpdateDialog();
    void UpdateDialogLipSync();
    // Static game helper (__stdcall, no 'this'). Fills outName and returns it wrapped.
    CExoString* GetCameraAnimationName(CExoString* outName, short animationId);

    // Area transition
    // Writes the transition position into 'out' and returns it (or nullptr on failure).
    Vector* GetAreaTranstionPosition(Vector* out);
    void HideAreaTransition();
    void UpdateAreaTransitionGUI();

    // Fade
    int GetFadeOverridePanelVisible();
    int IsGlobalFadeObscuring();
    int IsGlobalFading();
    void StopGlobalFade();

    // Bark bubble / minimap / input
    // Returns whether the bark bubble is visible; writes its bottom Y into outBottomY.
    int GetBarkBubbleVisible(int* outBottomY);
    int GetCanClick();
    int GetMiniMapVisible();

    // Party / status summary
    int GetPartyAccessPanelUp(int checkInventory);
    int GetPendingStatusSummary();
    void ShowPartySelection(CExoString* exitScript, int requireAccept, int forcedNPC1, int forcedNPC2);
    void ShowStatusSummary();

    // Screens / menus
    void HideContainerGui();
    void HideGalaxyMapGui();
    void HideItemCreateMenu();
    void HideLoadModuleDebugMenu();
    void HidePowersFeatsSkillsDebugMenu();
    void HideSoloMode();
    void HideStoreGui();
    void HideSubItemCreateMenu();
    void HideUpgradeScreen();
    void SetLevelUpMode(int mode);
    void ShowGalaxyMapGui(int selectedPlanetId);
    void ShowItemCreateMenu();
    void ShowSoloModeQuery(int doStealth);
    void ShowUpgradeScreen(DWORD itemId);

    // SWInGameGui navigation
    void NextSWInGameGui();
    void PrevSWInGameGui();
    void SwitchToSWInGameGui(int guiId);

    // Main interface
    void RemoveGUIPopUps();
    void RePopulateMainInterface();
    void ResetInterfaceForSize();

    // Lifecycle
    void ShutDown();
    void ShutDownGlobal();

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

private:
    typedef void(__thiscall* AddFloatyTextFn)(void* thisPtr, DWORD clientId, void* text, Vector* color, float duration);
    typedef void(__thiscall* AppendToMsgBufferFn)(void* thisPtr, void* messageString, DWORD messageType, BYTE messageColor);
    typedef int(__thiscall* GetMessageBufferSizeFn)(void* thisPtr);
    typedef void(__thiscall* UpdateMessageGUIFn)(void* thisPtr);

    typedef void(__thiscall* ChangeCharacterFn)(void* thisPtr);
    typedef void(__thiscall* DoQuickLoadFn)(void* thisPtr);
    typedef int(__thiscall* GetCurrentEquipScreenNPCFn)(void* thisPtr);

    typedef void(__thiscall* FadeAndStopDialogAmbientTrackFn)(void* thisPtr);
    typedef int(__thiscall* GetCanHeadFollowInDialogFn)(void* thisPtr, DWORD objectId);
    typedef int(__thiscall* GetCanOrientInDialogFn)(void* thisPtr, DWORD clientId);
    typedef int(__thiscall* IsAnimationPlayingInDialogFn)(void* thisPtr);
    typedef int(__thiscall* IsCameraDialogFn)(void* thisPtr);
    typedef int(__thiscall* IsFadingInDialogFn)(void* thisPtr);
    typedef void(__thiscall* LoadAmbientTrackForDialogFn)(void* thisPtr, void* track);
    typedef int(__thiscall* LoadStuntModelsForDialogFn)(void* thisPtr);
    typedef void(__thiscall* ResetDialogAnimationsFn)(void* thisPtr);
    typedef void(__thiscall* ResetTalkAnimationsFn)(void* thisPtr);
    typedef void(__thiscall* UpdateDialogFn)(void* thisPtr);
    typedef void(__thiscall* UpdateDialogLipSyncFn)(void* thisPtr);
    typedef void* (__stdcall* GetCameraAnimationNameFn)(void* outName, short animationId);

    typedef Vector* (__thiscall* GetAreaTranstionPositionFn)(void* thisPtr, Vector* out);
    typedef void(__thiscall* HideAreaTransitionFn)(void* thisPtr);
    typedef void(__thiscall* UpdateAreaTransitionGUIFn)(void* thisPtr);

    typedef int(__thiscall* GetFadeOverridePanelVisibleFn)(void* thisPtr);
    typedef int(__thiscall* IsGlobalFadeObscuringFn)(void* thisPtr);
    typedef int(__thiscall* IsGlobalFadingFn)(void* thisPtr);
    typedef void(__thiscall* StopGlobalFadeFn)(void* thisPtr);

    typedef int(__thiscall* GetBarkBubbleVisibleFn)(void* thisPtr, int* outBottomY);
    typedef int(__thiscall* GetCanClickFn)(void* thisPtr);
    typedef int(__thiscall* GetMiniMapVisibleFn)(void* thisPtr);

    typedef int(__thiscall* GetPartyAccessPanelUpFn)(void* thisPtr, int checkInventory);
    typedef int(__thiscall* GetPendingStatusSummaryFn)(void* thisPtr);
    typedef void(__thiscall* ShowPartySelectionFn)(void* thisPtr, void* exitScript, int requireAccept, int forcedNPC1, int forcedNPC2);
    typedef void(__thiscall* ShowStatusSummaryFn)(void* thisPtr);

    typedef void(__thiscall* HideContainerGuiFn)(void* thisPtr);
    typedef void(__thiscall* HideGalaxyMapGuiFn)(void* thisPtr);
    typedef void(__thiscall* HideItemCreateMenuFn)(void* thisPtr);
    typedef void(__thiscall* HideLoadModuleDebugMenuFn)(void* thisPtr);
    typedef void(__thiscall* HidePowersFeatsSkillsDebugMenuFn)(void* thisPtr);
    typedef void(__thiscall* HideSoloModeFn)(void* thisPtr);
    typedef void(__thiscall* HideStoreGuiFn)(void* thisPtr);
    typedef void(__thiscall* HideSubItemCreateMenuFn)(void* thisPtr);
    typedef void(__thiscall* HideUpgradeScreenFn)(void* thisPtr);
    typedef void(__thiscall* SetLevelUpModeFn)(void* thisPtr, int mode);
    typedef void(__thiscall* ShowGalaxyMapGuiFn)(void* thisPtr, int selectedPlanetId);
    typedef void(__thiscall* ShowItemCreateMenuFn)(void* thisPtr);
    typedef void(__thiscall* ShowSoloModeQueryFn)(void* thisPtr, int doStealth);
    typedef void(__thiscall* ShowUpgradeScreenFn)(void* thisPtr, DWORD itemId);

    typedef void(__thiscall* NextSWInGameGuiFn)(void* thisPtr);
    typedef void(__thiscall* PrevSWInGameGuiFn)(void* thisPtr);
    typedef void(__thiscall* SwitchToSWInGameGuiFn)(void* thisPtr, int guiId);

    typedef void(__thiscall* RemoveGUIPopUpsFn)(void* thisPtr);
    typedef void(__thiscall* RePopulateMainInterfaceFn)(void* thisPtr);
    typedef void(__thiscall* ResetInterfaceForSizeFn)(void* thisPtr);

    typedef void(__thiscall* ShutDownFn)(void* thisPtr);
    typedef void(__thiscall* ShutDownGlobalFn)(void* thisPtr);

    static AddFloatyTextFn addFloatyText;
    static AppendToMsgBufferFn appendToMsgBuffer;
    static GetMessageBufferSizeFn getMessageBufferSize;
    static UpdateMessageGUIFn updateMessageGUI;

    static ChangeCharacterFn changeCharacter;
    static DoQuickLoadFn doQuickLoad;
    static GetCurrentEquipScreenNPCFn getCurrentEquipScreenNPC;

    static FadeAndStopDialogAmbientTrackFn fadeAndStopDialogAmbientTrack;
    static GetCanHeadFollowInDialogFn getCanHeadFollowInDialog;
    static GetCanOrientInDialogFn getCanOrientInDialog;
    static IsAnimationPlayingInDialogFn isAnimationPlayingInDialog;
    static IsCameraDialogFn isCameraDialog;
    static IsFadingInDialogFn isFadingInDialog;
    static LoadAmbientTrackForDialogFn loadAmbientTrackForDialog;
    static LoadStuntModelsForDialogFn loadStuntModelsForDialog;
    static ResetDialogAnimationsFn resetDialogAnimations;
    static ResetTalkAnimationsFn resetTalkAnimations;
    static UpdateDialogFn updateDialog;
    static UpdateDialogLipSyncFn updateDialogLipSync;
    static GetCameraAnimationNameFn getCameraAnimationName;

    static GetAreaTranstionPositionFn getAreaTranstionPosition;
    static HideAreaTransitionFn hideAreaTransition;
    static UpdateAreaTransitionGUIFn updateAreaTransitionGUI;

    static GetFadeOverridePanelVisibleFn getFadeOverridePanelVisible;
    static IsGlobalFadeObscuringFn isGlobalFadeObscuring;
    static IsGlobalFadingFn isGlobalFading;
    static StopGlobalFadeFn stopGlobalFade;

    static GetBarkBubbleVisibleFn getBarkBubbleVisible;
    static GetCanClickFn getCanClick;
    static GetMiniMapVisibleFn getMiniMapVisible;

    static GetPartyAccessPanelUpFn getPartyAccessPanelUp;
    static GetPendingStatusSummaryFn getPendingStatusSummary;
    static ShowPartySelectionFn showPartySelection;
    static ShowStatusSummaryFn showStatusSummary;

    static HideContainerGuiFn hideContainerGui;
    static HideGalaxyMapGuiFn hideGalaxyMapGui;
    static HideItemCreateMenuFn hideItemCreateMenu;
    static HideLoadModuleDebugMenuFn hideLoadModuleDebugMenu;
    static HidePowersFeatsSkillsDebugMenuFn hidePowersFeatsSkillsDebugMenu;
    static HideSoloModeFn hideSoloMode;
    static HideStoreGuiFn hideStoreGui;
    static HideSubItemCreateMenuFn hideSubItemCreateMenu;
    static HideUpgradeScreenFn hideUpgradeScreen;
    static SetLevelUpModeFn setLevelUpMode;
    static ShowGalaxyMapGuiFn showGalaxyMapGui;
    static ShowItemCreateMenuFn showItemCreateMenu;
    static ShowSoloModeQueryFn showSoloModeQuery;
    static ShowUpgradeScreenFn showUpgradeScreen;

    static NextSWInGameGuiFn nextSWInGameGui;
    static PrevSWInGameGuiFn prevSWInGameGui;
    static SwitchToSWInGameGuiFn switchToSWInGameGui;

    static RemoveGUIPopUpsFn removeGUIPopUps;
    static RePopulateMainInterfaceFn rePopulateMainInterface;
    static ResetInterfaceForSizeFn resetInterfaceForSize;

    static ShutDownFn shutDown;
    static ShutDownGlobalFn shutDownGlobal;

    static bool functionsInitialized;
    static bool offsetsInitialized;
};
