#include "CClientExoApp.h"
#include "CAppManager.h"
#include "GameVersion.h"
#include "../Common.h"
#include "CExoString.h"
#include "CResRef.h"
#include "CSWCCreature.h"
#include "CGameObject.h"
#include "CGuiInGame.h"

CClientExoApp::GetClientOptionsFn CClientExoApp::getClientOptions = nullptr;

CClientExoApp::AddMoveToModuleMovieFn CClientExoApp::addMoveToModuleMovie = nullptr;
CClientExoApp::AddMovieToMovieQueueFn CClientExoApp::addMovieToMovieQueue = nullptr;
CClientExoApp::CancelMovieFn CClientExoApp::cancelMovie = nullptr;
CClientExoApp::RemoveMoveToModuleMoviesFn CClientExoApp::removeMoveToModuleMovies = nullptr;

CClientExoApp::ChangeCharacterFn CClientExoApp::changeCharacter = nullptr;
CClientExoApp::CreatureAcquireItemFn CClientExoApp::creatureAcquireItem = nullptr;
CClientExoApp::GetCreatureByGameObjectIDFn CClientExoApp::getCreatureByGameObjectID = nullptr;
CClientExoApp::GetPlayerCreatureFn CClientExoApp::getPlayerCreature = nullptr;
CClientExoApp::SetPlayerCreatureFn CClientExoApp::setPlayerCreature = nullptr;
CClientExoApp::SetFutureLeaderFn CClientExoApp::setFutureLeader = nullptr;
CClientExoApp::GetCharacterChangeInProgressFn CClientExoApp::getCharacterChangeInProgress = nullptr;
CClientExoApp::PlayerFlourishWeaponsFn CClientExoApp::playerFlourishWeapons = nullptr;
CClientExoApp::RunDeathSequenceFn CClientExoApp::runDeathSequence = nullptr;

CClientExoApp::DisableInputFn CClientExoApp::disableInput = nullptr;
CClientExoApp::EnableInputFn CClientExoApp::enableInput = nullptr;
CClientExoApp::GetInputClassFn CClientExoApp::getInputClass = nullptr;
CClientExoApp::SetInputClassFn CClientExoApp::setInputClass = nullptr;
CClientExoApp::GetInFreeLookFn CClientExoApp::getInFreeLook = nullptr;
CClientExoApp::SetMouseModeFn CClientExoApp::setMouseMode = nullptr;

CClientExoApp::DisableVideoEffectFn CClientExoApp::disableVideoEffect = nullptr;
CClientExoApp::EnableVideoEffectFn CClientExoApp::enableVideoEffect = nullptr;
CClientExoApp::IsValidResolutionFn CClientExoApp::isValidResolution = nullptr;
CClientExoApp::SetVideoModeFn CClientExoApp::setVideoMode = nullptr;
CClientExoApp::SetTexturePackFn CClientExoApp::setTexturePack = nullptr;

CClientExoApp::DismissInGameGUIFn CClientExoApp::dismissInGameGUI = nullptr;
CClientExoApp::DisplayMainMenuFn CClientExoApp::displayMainMenu = nullptr;
CClientExoApp::ShutDownToMainMenuFn CClientExoApp::shutDownToMainMenu = nullptr;
CClientExoApp::GetGUIStringFn CClientExoApp::getGUIString = nullptr;
CClientExoApp::GetInGameGuiFn CClientExoApp::getInGameGui = nullptr;

CClientExoApp::ExitProgramFn CClientExoApp::exitProgram = nullptr;
CClientExoApp::QueryExitProgramFn CClientExoApp::queryExitProgram = nullptr;
CClientExoApp::GetGameOverFn CClientExoApp::getGameOver = nullptr;
CClientExoApp::SetGameOverFn CClientExoApp::setGameOver = nullptr;

CClientExoApp::GetActivePauseStateFn CClientExoApp::getActivePauseState = nullptr;
CClientExoApp::GetAutoPausedFn CClientExoApp::getAutoPaused = nullptr;
CClientExoApp::GetPausedByCombatFn CClientExoApp::getPausedByCombat = nullptr;
CClientExoApp::SetCombatModeFn CClientExoApp::setCombatMode = nullptr;

CClientExoApp::GetGameObjectFn CClientExoApp::getGameObject = nullptr;
CClientExoApp::GetObjectNameFn CClientExoApp::getObjectName = nullptr;
CClientExoApp::GetLastTargetFn CClientExoApp::getLastTarget = nullptr;
CClientExoApp::GetTargetChangingFn CClientExoApp::getTargetChanging = nullptr;

CClientExoApp::GetLoadingSaveGameFn CClientExoApp::getLoadingSaveGame = nullptr;
CClientExoApp::SendLoadGameRequestFn CClientExoApp::sendLoadGameRequest = nullptr;
CClientExoApp::SendSaveGameRequestFn CClientExoApp::sendSaveGameRequest = nullptr;

CClientExoApp::IsSoundPlayingInDialogFn CClientExoApp::isSoundPlayingInDialog = nullptr;
CClientExoApp::SetCanSendDialogFn CClientExoApp::setCanSendDialog = nullptr;
CClientExoApp::SetRunScriptFn CClientExoApp::setRunScript = nullptr;

CClientExoApp::GetPazaakIsRunningFn CClientExoApp::getPazaakIsRunning = nullptr;
CClientExoApp::GetPazaakResultsFn CClientExoApp::getPazaakResults = nullptr;

CClientExoApp::GetCreditSequenceInProgressFn CClientExoApp::getCreditSequenceInProgress = nullptr;
CClientExoApp::StopCreditSequenceFn CClientExoApp::stopCreditSequence = nullptr;

CClientExoApp::GetClientLanguageFn CClientExoApp::getClientLanguage = nullptr;

bool CClientExoApp::functionsInitialized = false;
bool CClientExoApp::offsetsInitialized = false;

void CClientExoApp::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CClientExoApp] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        getClientOptions = reinterpret_cast<GetClientOptionsFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "GetClientOptions"));

        addMoveToModuleMovie = reinterpret_cast<AddMoveToModuleMovieFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "AddMoveToModuleMovie"));
        addMovieToMovieQueue = reinterpret_cast<AddMovieToMovieQueueFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "AddMovieToMovieQueue"));
        cancelMovie = reinterpret_cast<CancelMovieFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "CancelMovie"));
        removeMoveToModuleMovies = reinterpret_cast<RemoveMoveToModuleMoviesFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "RemoveMoveToModuleMovies"));

        changeCharacter = reinterpret_cast<ChangeCharacterFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "ChangeCharacter"));
        creatureAcquireItem = reinterpret_cast<CreatureAcquireItemFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "CreatureAcquireItem"));
        getCreatureByGameObjectID = reinterpret_cast<GetCreatureByGameObjectIDFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "GetCreatureByGameObjectID"));
        getPlayerCreature = reinterpret_cast<GetPlayerCreatureFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "GetPlayerCreature"));
        setPlayerCreature = reinterpret_cast<SetPlayerCreatureFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "SetPlayerCreature"));
        setFutureLeader = reinterpret_cast<SetFutureLeaderFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "SetFutureLeader"));
        getCharacterChangeInProgress = reinterpret_cast<GetCharacterChangeInProgressFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "GetCharacterChangeInProgress"));
        playerFlourishWeapons = reinterpret_cast<PlayerFlourishWeaponsFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "PlayerFlourishWeapons"));
        runDeathSequence = reinterpret_cast<RunDeathSequenceFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "RunDeathSequence"));

        disableInput = reinterpret_cast<DisableInputFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "DisableInput"));
        enableInput = reinterpret_cast<EnableInputFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "EnableInput"));
        getInputClass = reinterpret_cast<GetInputClassFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "GetInputClass"));
        setInputClass = reinterpret_cast<SetInputClassFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "SetInputClass"));
        getInFreeLook = reinterpret_cast<GetInFreeLookFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "GetInFreeLook"));
        setMouseMode = reinterpret_cast<SetMouseModeFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "SetMouseMode"));

        disableVideoEffect = reinterpret_cast<DisableVideoEffectFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "DisableVideoEffect"));
        enableVideoEffect = reinterpret_cast<EnableVideoEffectFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "EnableVideoEffect"));
        isValidResolution = reinterpret_cast<IsValidResolutionFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "IsValidResolution"));
        setVideoMode = reinterpret_cast<SetVideoModeFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "SetVideoMode"));
        setTexturePack = reinterpret_cast<SetTexturePackFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "SetTexturePack"));

        dismissInGameGUI = reinterpret_cast<DismissInGameGUIFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "DismissInGameGUI"));
        displayMainMenu = reinterpret_cast<DisplayMainMenuFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "DisplayMainMenu"));
        shutDownToMainMenu = reinterpret_cast<ShutDownToMainMenuFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "ShutDownToMainMenu"));
        getGUIString = reinterpret_cast<GetGUIStringFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "GetGUIString"));
        getInGameGui = reinterpret_cast<GetInGameGuiFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "GetInGameGui"));

        exitProgram = reinterpret_cast<ExitProgramFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "ExitProgram"));
        queryExitProgram = reinterpret_cast<QueryExitProgramFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "QueryExitProgram"));
        getGameOver = reinterpret_cast<GetGameOverFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "GetGameOver"));
        setGameOver = reinterpret_cast<SetGameOverFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "SetGameOver"));

        getActivePauseState = reinterpret_cast<GetActivePauseStateFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "GetActivePauseState"));
        getAutoPaused = reinterpret_cast<GetAutoPausedFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "GetAutoPaused"));
        getPausedByCombat = reinterpret_cast<GetPausedByCombatFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "GetPausedByCombat"));
        setCombatMode = reinterpret_cast<SetCombatModeFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "SetCombatMode"));

        getGameObject = reinterpret_cast<GetGameObjectFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "GetGameObject"));
        getObjectName = reinterpret_cast<GetObjectNameFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "GetObjectName"));
        getLastTarget = reinterpret_cast<GetLastTargetFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "GetLastTarget"));
        getTargetChanging = reinterpret_cast<GetTargetChangingFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "GetTargetChanging"));

        getLoadingSaveGame = reinterpret_cast<GetLoadingSaveGameFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "GetLoadingSaveGame"));
        sendLoadGameRequest = reinterpret_cast<SendLoadGameRequestFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "SendLoadGameRequest"));
        sendSaveGameRequest = reinterpret_cast<SendSaveGameRequestFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "SendSaveGameRequest"));

        isSoundPlayingInDialog = reinterpret_cast<IsSoundPlayingInDialogFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "IsSoundPlayingInDialog"));
        setCanSendDialog = reinterpret_cast<SetCanSendDialogFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "SetCanSendDialog"));
        setRunScript = reinterpret_cast<SetRunScriptFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "SetRunScript"));

        getPazaakIsRunning = reinterpret_cast<GetPazaakIsRunningFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "GetPazaakIsRunning"));
        getPazaakResults = reinterpret_cast<GetPazaakResultsFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "GetPazaakResults"));

        getCreditSequenceInProgress = reinterpret_cast<GetCreditSequenceInProgressFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "GetCreditSequenceInProgress"));
        stopCreditSequence = reinterpret_cast<StopCreditSequenceFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "StopCreditSequence"));

        getClientLanguage = reinterpret_cast<GetClientLanguageFn>(
            GameVersion::GetFunctionAddress("CClientExoApp", "GetClientLanguage"));
    }
    catch (const GameVersionException& e) {
        debugLog("[CClientExoApp] ERROR: %s\n", e.what());
        return;
    }

    functionsInitialized = true;
}

void CClientExoApp::InitializeOffsets() {
    // CClientExoApp has no offsets
    offsetsInitialized = true;
}

CClientExoApp* CClientExoApp::GetInstance() {
    CAppManager* appManager = CAppManager::GetInstance();
    if (!appManager) {
        OutputDebugStringA("[CClientExoApp] ERROR: Failed to get CAppManager instance\n");
        return nullptr;
    }

    CClientExoApp* client = appManager->GetClient();
    delete appManager;  // Clean up the temporary CAppManager instance

    return client;
}

CClientExoApp::CClientExoApp(void* clientPtr)
    : GameAPIObject(clientPtr, false)  // false = don't free (singleton)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CClientExoApp::~CClientExoApp() {
    // Base class destructor handles objectPtr cleanup
}

CClientOptions* CClientExoApp::GetClientOptions() {
    if (!objectPtr || !getClientOptions) {
        return nullptr;
    }

    void* clientOptionsPtr = getClientOptions(objectPtr);

    if (clientOptionsPtr)
        return new CClientOptions(clientOptionsPtr);

    debugLog("[CClientExoApp] ERROR: Failed to get CClientOptions");
    return nullptr;
}

// ===== Movies =====

void CClientExoApp::AddMoveToModuleMovie(CExoString* movie) {
    if (!objectPtr || !addMoveToModuleMovie) {
        return;
    }
    addMoveToModuleMovie(objectPtr, movie ? movie->GetPtr() : nullptr);
}

void CClientExoApp::AddMovieToMovieQueue(CExoString* movie, int skippable) {
    if (!objectPtr || !addMovieToMovieQueue) {
        return;
    }
    addMovieToMovieQueue(objectPtr, movie ? movie->GetPtr() : nullptr, skippable);
}

void CClientExoApp::CancelMovie() {
    if (!objectPtr || !cancelMovie) {
        return;
    }
    cancelMovie(objectPtr);
}

void CClientExoApp::RemoveMoveToModuleMovies() {
    if (!objectPtr || !removeMoveToModuleMovies) {
        return;
    }
    removeMoveToModuleMovies(objectPtr);
}

// ===== Characters / creatures =====

void CClientExoApp::ChangeCharacter(DWORD leader, int playSound) {
    if (!objectPtr || !changeCharacter) {
        return;
    }
    changeCharacter(objectPtr, leader, playSound);
}

void CClientExoApp::CreatureAcquireItem(DWORD creatureId, CResRef* item) {
    // __stdcall global helper: does not take a 'this' pointer.
    if (!objectPtr || !creatureAcquireItem) {
        return;
    }
    creatureAcquireItem(creatureId, item ? item->GetPtr() : nullptr);
}

CSWCCreature* CClientExoApp::GetCreatureByGameObjectID(DWORD objectId) {
    if (!objectPtr || !getCreatureByGameObjectID) {
        debugLog("[CClientExoApp] Error: no objectPtr or no getCreatureByGameObjectID");
        return nullptr;
    }

    void* creaturePtr = getCreatureByGameObjectID(objectPtr, objectId);
    if (!creaturePtr) {
        debugLog("[CClientExoApp] Error: Bad creaturePtr");
        return nullptr;
    }

    return new CSWCCreature(creaturePtr);
}

CSWCCreature* CClientExoApp::GetPlayerCreature() {
    if (!objectPtr || !getPlayerCreature) {
        debugLog("[CClientExoApp] Error: no objectPtr or no getPlayerCreature");
        return nullptr;
    }

    void* creaturePtr = getPlayerCreature(objectPtr);
    if (!creaturePtr) {
        debugLog("[CClientExoApp] Error: Bad creaturePtr");
        return nullptr;
    }

    return new CSWCCreature(creaturePtr);
}

void CClientExoApp::SetPlayerCreature(DWORD creatureId) {
    if (!objectPtr || !setPlayerCreature) {
        return;
    }
    setPlayerCreature(objectPtr, creatureId);
}

void CClientExoApp::SetFutureLeader(DWORD leader) {
    if (!objectPtr || !setFutureLeader) {
        return;
    }
    setFutureLeader(objectPtr, leader);
}

int CClientExoApp::GetCharacterChangeInProgress() {
    if (!objectPtr || !getCharacterChangeInProgress) {
        return 0;
    }
    return getCharacterChangeInProgress(objectPtr);
}

void CClientExoApp::PlayerFlourishWeapons() {
    if (!objectPtr || !playerFlourishWeapons) {
        return;
    }
    playerFlourishWeapons(objectPtr);
}

void CClientExoApp::RunDeathSequence() {
    if (!objectPtr || !runDeathSequence) {
        return;
    }
    runDeathSequence(objectPtr);
}

// ===== Input =====

void CClientExoApp::DisableInput() {
    if (!objectPtr || !disableInput) {
        return;
    }
    disableInput(objectPtr);
}

void CClientExoApp::EnableInput() {
    if (!objectPtr || !enableInput) {
        return;
    }
    enableInput(objectPtr);
}

int CClientExoApp::GetInputClass() {
    if (!objectPtr || !getInputClass) {
        return 0;
    }
    return getInputClass(objectPtr);
}

void CClientExoApp::SetInputClass(int inputClass, int setGuiStatus) {
    if (!objectPtr || !setInputClass) {
        return;
    }
    setInputClass(objectPtr, inputClass, setGuiStatus);
}

int CClientExoApp::GetInFreeLook() {
    if (!objectPtr || !getInFreeLook) {
        return 0;
    }
    return getInFreeLook(objectPtr);
}

void CClientExoApp::SetMouseMode(BYTE mode) {
    if (!objectPtr || !setMouseMode) {
        return;
    }
    setMouseMode(objectPtr, mode);
}

// ===== Video / display =====

void CClientExoApp::DisableVideoEffect() {
    if (!objectPtr || !disableVideoEffect) {
        return;
    }
    disableVideoEffect(objectPtr);
}

void CClientExoApp::EnableVideoEffect(int effect) {
    if (!objectPtr || !enableVideoEffect) {
        return;
    }
    enableVideoEffect(objectPtr, effect);
}

int CClientExoApp::IsValidResolution(DWORD width, DWORD height) {
    // __stdcall global helper: does not take a 'this' pointer.
    if (!objectPtr || !isValidResolution) {
        return 0;
    }
    return isValidResolution(width, height);
}

int CClientExoApp::SetVideoMode(DWORD modeNum) {
    if (!objectPtr || !setVideoMode) {
        return 0;
    }
    return setVideoMode(objectPtr, modeNum);
}

void CClientExoApp::SetTexturePack(BYTE pack) {
    if (!objectPtr || !setTexturePack) {
        return;
    }
    setTexturePack(objectPtr, pack);
}

// ===== GUI / menus =====

int CClientExoApp::DismissInGameGUI() {
    if (!objectPtr || !dismissInGameGUI) {
        return 0;
    }
    return dismissInGameGUI(objectPtr);
}

void CClientExoApp::DisplayMainMenu() {
    if (!objectPtr || !displayMainMenu) {
        return;
    }
    displayMainMenu(objectPtr);
}

void CClientExoApp::ShutDownToMainMenu() {
    if (!objectPtr || !shutDownToMainMenu) {
        return;
    }
    shutDownToMainMenu(objectPtr);
}

CExoString* CClientExoApp::GetGUIString(CExoString* outString, DWORD strRef) {
    if (!objectPtr || !getGUIString || !outString) {
        return nullptr;
    }

    void* resultPtr = getGUIString(objectPtr, outString->GetPtr(), strRef);
    if (!resultPtr) {
        return nullptr;
    }

    return new CExoString(resultPtr);
}

CGuiInGame* CClientExoApp::GetInGameGui() {
    if (!objectPtr || !getInGameGui) {
        return nullptr;
    }

    void* guiInGamePtr = getInGameGui(objectPtr);
    if (!guiInGamePtr) {
        return nullptr;
    }

    return new CGuiInGame(guiInGamePtr);
}

// ===== Program lifecycle =====

void CClientExoApp::ExitProgram() {
    // __stdcall global helper: does not take a 'this' pointer.
    if (!objectPtr || !exitProgram) {
        return;
    }
    exitProgram();
}

void CClientExoApp::QueryExitProgram() {
    if (!objectPtr || !queryExitProgram) {
        return;
    }
    queryExitProgram(objectPtr);
}

int CClientExoApp::GetGameOver() {
    if (!objectPtr || !getGameOver) {
        return 0;
    }
    return getGameOver(objectPtr);
}

void CClientExoApp::SetGameOver(int gameOver) {
    if (!objectPtr || !setGameOver) {
        return;
    }
    setGameOver(objectPtr, gameOver);
}

// ===== Pause / combat state =====

BYTE CClientExoApp::GetActivePauseState() {
    if (!objectPtr || !getActivePauseState) {
        return 0;
    }
    return getActivePauseState(objectPtr);
}

int CClientExoApp::GetAutoPaused() {
    if (!objectPtr || !getAutoPaused) {
        return 0;
    }
    return getAutoPaused(objectPtr);
}

int CClientExoApp::GetPausedByCombat() {
    // __stdcall global helper: does not take a 'this' pointer.
    if (!objectPtr || !getPausedByCombat) {
        return 0;
    }
    return getPausedByCombat();
}

void CClientExoApp::SetCombatMode(int mode) {
    if (!objectPtr || !setCombatMode) {
        return;
    }
    setCombatMode(objectPtr, mode);
}

// ===== Objects / targeting =====

CGameObject* CClientExoApp::GetGameObject(DWORD clientId) {
    if (!objectPtr || !getGameObject) {
        return nullptr;
    }

    void* gameObjectPtr = getGameObject(objectPtr, clientId);
    if (!gameObjectPtr) {
        return nullptr;
    }

    return new CGameObject(gameObjectPtr);
}

int CClientExoApp::GetObjectName(DWORD clientId, CExoString* outName) {
    if (!objectPtr || !getObjectName || !outName) {
        return 0;
    }
    return getObjectName(objectPtr, clientId, outName->GetPtr());
}

DWORD CClientExoApp::GetLastTarget() {
    if (!objectPtr || !getLastTarget) {
        return OBJECT_DEFAULT;
    }
    return getLastTarget(objectPtr);
}

int CClientExoApp::GetTargetChanging() {
    if (!objectPtr || !getTargetChanging) {
        return 0;
    }
    return getTargetChanging(objectPtr);
}

// ===== Save / load =====

int CClientExoApp::GetLoadingSaveGame() {
    if (!objectPtr || !getLoadingSaveGame) {
        return 0;
    }
    return getLoadingSaveGame(objectPtr);
}

int CClientExoApp::SendLoadGameRequest(DWORD slot, CExoString* saveName, CExoString* module) {
    if (!objectPtr || !sendLoadGameRequest) {
        return 0;
    }
    return sendLoadGameRequest(objectPtr, slot,
        saveName ? saveName->GetPtr() : nullptr,
        module ? module->GetPtr() : nullptr);
}

int CClientExoApp::SendSaveGameRequest(DWORD slot, CExoString* saveName, CExoString* module) {
    if (!objectPtr || !sendSaveGameRequest) {
        return 0;
    }
    return sendSaveGameRequest(objectPtr, slot,
        saveName ? saveName->GetPtr() : nullptr,
        module ? module->GetPtr() : nullptr);
}

// ===== Dialog / scripting =====

int CClientExoApp::IsSoundPlayingInDialog() {
    if (!objectPtr || !isSoundPlayingInDialog) {
        return 0;
    }
    return isSoundPlayingInDialog(objectPtr);
}

void CClientExoApp::SetCanSendDialog(int canSend) {
    if (!objectPtr || !setCanSendDialog) {
        return;
    }
    setCanSendDialog(objectPtr, canSend);
}

void CClientExoApp::SetRunScript(CExoString* script) {
    if (!objectPtr || !setRunScript) {
        return;
    }
    setRunScript(objectPtr, script ? script->GetPtr() : nullptr);
}

// ===== Pazaak minigame =====

int CClientExoApp::GetPazaakIsRunning() {
    if (!objectPtr || !getPazaakIsRunning) {
        return 0;
    }
    return getPazaakIsRunning(objectPtr);
}

void CClientExoApp::GetPazaakResults(int* won, int* wager) {
    if (!objectPtr || !getPazaakResults) {
        return;
    }
    getPazaakResults(objectPtr, won, wager);
}

// ===== Credits =====

int CClientExoApp::GetCreditSequenceInProgress() {
    if (!objectPtr || !getCreditSequenceInProgress) {
        return 0;
    }
    return getCreditSequenceInProgress(objectPtr);
}

void CClientExoApp::StopCreditSequence() {
    if (!objectPtr || !stopCreditSequence) {
        return;
    }
    stopCreditSequence(objectPtr);
}

// ===== Misc =====

int CClientExoApp::GetClientLanguage() {
    if (!objectPtr || !getClientLanguage) {
        return 0;
    }
    return getClientLanguage(objectPtr);
}
