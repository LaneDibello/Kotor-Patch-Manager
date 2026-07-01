#pragma once
#include <windows.h>
#include "GameAPIObject.h"
#include "GameAPI/CClientOptions.h"

class CAppManager;
class CExoString;
class CResRef;
class CSWCCreature;
class CGameObject;
class CGuiInGame;

class CClientExoApp : public GameAPIObject {
public:
    static CClientExoApp* GetInstance();
    ~CClientExoApp();

    CClientOptions* GetClientOptions();

    // Movies
    void AddMoveToModuleMovie(CExoString* movie);
    void AddMovieToMovieQueue(CExoString* movie, int skippable);
    void CancelMovie();
    void RemoveMoveToModuleMovies();

    // Characters / creatures
    void ChangeCharacter(DWORD leader, int playSound);
    void CreatureAcquireItem(DWORD creatureId, CResRef* item);
    CSWCCreature* GetCreatureByGameObjectID(DWORD objectId);
    CSWCCreature* GetPlayerCreature();
    void SetPlayerCreature(DWORD creatureId);
    void SetFutureLeader(DWORD leader);
    int GetCharacterChangeInProgress();
    void PlayerFlourishWeapons();
    void RunDeathSequence();

    // Input
    void DisableInput();
    void EnableInput();
    int GetInputClass();
    void SetInputClass(int inputClass, int setGuiStatus);
    int GetInFreeLook();
    void SetMouseMode(BYTE mode);

    // Video / display
    void DisableVideoEffect();
    void EnableVideoEffect(int effect);
    int IsValidResolution(DWORD width, DWORD height);
    int SetVideoMode(DWORD modeNum);
    void SetTexturePack(BYTE pack);

    // GUI / menus
    int DismissInGameGUI();
    void DisplayMainMenu();
    void ShutDownToMainMenu();
    CExoString* GetGUIString(CExoString* outString, DWORD strRef);
    // Returns the in-game GUI manager (heap-allocated wrapper; caller owns it). K1 only.
    CGuiInGame* GetInGameGui();

    // Program lifecycle
    void ExitProgram();
    void QueryExitProgram();
    int GetGameOver();
    void SetGameOver(int gameOver);

    // Pause / combat state
    BYTE GetActivePauseState();
    int GetAutoPaused();
    int GetPausedByCombat();
    void SetCombatMode(int mode);

    // Objects / targeting
    CGameObject* GetGameObject(DWORD clientId);
    int GetObjectName(DWORD clientId, CExoString* outName);
    DWORD GetLastTarget();
    int GetTargetChanging();

    // Save / load
    int GetLoadingSaveGame();
    int SendLoadGameRequest(DWORD slot, CExoString* saveName, CExoString* module);
    int SendSaveGameRequest(DWORD slot, CExoString* saveName, CExoString* module);

    // Dialog / scripting
    int IsSoundPlayingInDialog();
    void SetCanSendDialog(int canSend);
    void SetRunScript(CExoString* script);

    // Pazaak minigame
    int GetPazaakIsRunning();
    void GetPazaakResults(int* won, int* wager);

    // Credits
    int GetCreditSequenceInProgress();
    void StopCreditSequence();

    // Misc
    int GetClientLanguage();

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

private:
    friend class CAppManager;
    explicit CClientExoApp(void* clientPtr);

    typedef void* (__thiscall* GetClientOptionsFn)(void* thisPtr);

    typedef void(__thiscall* AddMoveToModuleMovieFn)(void* thisPtr, void* movie);
    typedef void(__thiscall* AddMovieToMovieQueueFn)(void* thisPtr, void* movie, int skippable);
    typedef void(__thiscall* CancelMovieFn)(void* thisPtr);
    typedef void(__thiscall* RemoveMoveToModuleMoviesFn)(void* thisPtr);

    typedef void(__thiscall* ChangeCharacterFn)(void* thisPtr, DWORD leader, int playSound);
    typedef void(__stdcall* CreatureAcquireItemFn)(DWORD creatureId, void* item);
    typedef void* (__thiscall* GetCreatureByGameObjectIDFn)(void* thisPtr, DWORD objectId);
    typedef void* (__thiscall* GetPlayerCreatureFn)(void* thisPtr);
    typedef void(__thiscall* SetPlayerCreatureFn)(void* thisPtr, DWORD creatureId);
    typedef void(__thiscall* SetFutureLeaderFn)(void* thisPtr, DWORD leader);
    typedef int(__thiscall* GetCharacterChangeInProgressFn)(void* thisPtr);
    typedef void(__thiscall* PlayerFlourishWeaponsFn)(void* thisPtr);
    typedef void(__thiscall* RunDeathSequenceFn)(void* thisPtr);

    typedef void(__thiscall* DisableInputFn)(void* thisPtr);
    typedef void(__thiscall* EnableInputFn)(void* thisPtr);
    typedef int(__thiscall* GetInputClassFn)(void* thisPtr);
    typedef void(__thiscall* SetInputClassFn)(void* thisPtr, int inputClass, int setGuiStatus);
    typedef int(__thiscall* GetInFreeLookFn)(void* thisPtr);
    typedef void(__thiscall* SetMouseModeFn)(void* thisPtr, BYTE mode);

    typedef void(__thiscall* DisableVideoEffectFn)(void* thisPtr);
    typedef void(__thiscall* EnableVideoEffectFn)(void* thisPtr, int effect);
    typedef int(__stdcall* IsValidResolutionFn)(DWORD width, DWORD height);
    typedef int(__thiscall* SetVideoModeFn)(void* thisPtr, DWORD modeNum);
    typedef void(__thiscall* SetTexturePackFn)(void* thisPtr, BYTE pack);

    typedef int(__thiscall* DismissInGameGUIFn)(void* thisPtr);
    typedef void(__thiscall* DisplayMainMenuFn)(void* thisPtr);
    typedef void(__thiscall* ShutDownToMainMenuFn)(void* thisPtr);
    typedef void* (__thiscall* GetGUIStringFn)(void* thisPtr, void* outString, DWORD strRef);
    typedef void* (__thiscall* GetInGameGuiFn)(void* thisPtr);

    typedef void(__stdcall* ExitProgramFn)();
    typedef void(__thiscall* QueryExitProgramFn)(void* thisPtr);
    typedef int(__thiscall* GetGameOverFn)(void* thisPtr);
    typedef void(__thiscall* SetGameOverFn)(void* thisPtr, int gameOver);

    typedef BYTE(__thiscall* GetActivePauseStateFn)(void* thisPtr);
    typedef int(__thiscall* GetAutoPausedFn)(void* thisPtr);
    typedef int(__stdcall* GetPausedByCombatFn)();
    typedef void(__thiscall* SetCombatModeFn)(void* thisPtr, int mode);

    typedef void* (__thiscall* GetGameObjectFn)(void* thisPtr, DWORD clientId);
    typedef int(__thiscall* GetObjectNameFn)(void* thisPtr, DWORD clientId, void* outName);
    typedef DWORD(__thiscall* GetLastTargetFn)(void* thisPtr);
    typedef int(__thiscall* GetTargetChangingFn)(void* thisPtr);

    typedef int(__thiscall* GetLoadingSaveGameFn)(void* thisPtr);
    typedef int(__thiscall* SendLoadGameRequestFn)(void* thisPtr, DWORD slot, void* saveName, void* module);
    typedef int(__thiscall* SendSaveGameRequestFn)(void* thisPtr, DWORD slot, void* saveName, void* module);

    typedef int(__thiscall* IsSoundPlayingInDialogFn)(void* thisPtr);
    typedef void(__thiscall* SetCanSendDialogFn)(void* thisPtr, int canSend);
    typedef void(__thiscall* SetRunScriptFn)(void* thisPtr, void* script);

    typedef int(__thiscall* GetPazaakIsRunningFn)(void* thisPtr);
    typedef void(__thiscall* GetPazaakResultsFn)(void* thisPtr, int* won, int* wager);

    typedef int(__thiscall* GetCreditSequenceInProgressFn)(void* thisPtr);
    typedef void(__thiscall* StopCreditSequenceFn)(void* thisPtr);

    typedef int(__thiscall* GetClientLanguageFn)(void* thisPtr);

    static GetClientOptionsFn getClientOptions;

    static AddMoveToModuleMovieFn addMoveToModuleMovie;
    static AddMovieToMovieQueueFn addMovieToMovieQueue;
    static CancelMovieFn cancelMovie;
    static RemoveMoveToModuleMoviesFn removeMoveToModuleMovies;

    static ChangeCharacterFn changeCharacter;
    static CreatureAcquireItemFn creatureAcquireItem;
    static GetCreatureByGameObjectIDFn getCreatureByGameObjectID;
    static GetPlayerCreatureFn getPlayerCreature;
    static SetPlayerCreatureFn setPlayerCreature;
    static SetFutureLeaderFn setFutureLeader;
    static GetCharacterChangeInProgressFn getCharacterChangeInProgress;
    static PlayerFlourishWeaponsFn playerFlourishWeapons;
    static RunDeathSequenceFn runDeathSequence;

    static DisableInputFn disableInput;
    static EnableInputFn enableInput;
    static GetInputClassFn getInputClass;
    static SetInputClassFn setInputClass;
    static GetInFreeLookFn getInFreeLook;
    static SetMouseModeFn setMouseMode;

    static DisableVideoEffectFn disableVideoEffect;
    static EnableVideoEffectFn enableVideoEffect;
    static IsValidResolutionFn isValidResolution;
    static SetVideoModeFn setVideoMode;
    static SetTexturePackFn setTexturePack;

    static DismissInGameGUIFn dismissInGameGUI;
    static DisplayMainMenuFn displayMainMenu;
    static ShutDownToMainMenuFn shutDownToMainMenu;
    static GetGUIStringFn getGUIString;
    static GetInGameGuiFn getInGameGui;

    static ExitProgramFn exitProgram;
    static QueryExitProgramFn queryExitProgram;
    static GetGameOverFn getGameOver;
    static SetGameOverFn setGameOver;

    static GetActivePauseStateFn getActivePauseState;
    static GetAutoPausedFn getAutoPaused;
    static GetPausedByCombatFn getPausedByCombat;
    static SetCombatModeFn setCombatMode;

    static GetGameObjectFn getGameObject;
    static GetObjectNameFn getObjectName;
    static GetLastTargetFn getLastTarget;
    static GetTargetChangingFn getTargetChanging;

    static GetLoadingSaveGameFn getLoadingSaveGame;
    static SendLoadGameRequestFn sendLoadGameRequest;
    static SendSaveGameRequestFn sendSaveGameRequest;

    static IsSoundPlayingInDialogFn isSoundPlayingInDialog;
    static SetCanSendDialogFn setCanSendDialog;
    static SetRunScriptFn setRunScript;

    static GetPazaakIsRunningFn getPazaakIsRunning;
    static GetPazaakResultsFn getPazaakResults;

    static GetCreditSequenceInProgressFn getCreditSequenceInProgress;
    static StopCreditSequenceFn stopCreditSequence;

    static GetClientLanguageFn getClientLanguage;

    static bool functionsInitialized;
    static bool offsetsInitialized;
};
