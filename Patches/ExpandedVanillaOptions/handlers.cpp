#include "Common.h"
#include "GameAPI/CExoIni.h"
#include "GameAPI/CExoString.h"
#include "GameAPI/CClientOptions.h"
#include "GameAPI/CClientExoApp.h"
#include "GameAPI/CSWGuiManager.h"

static CExoString& SwkotorIni()  { static CExoString s("swkotor.ini"); return s; }
static CExoString& AddlIni()     { static CExoString s("additional-options.ini"); return s; }
static CExoString& GraphicsCat() { static CExoString s("Graphics Options"); return s; }
static CExoString& SoundCat()    { static CExoString s("Sound Options"); return s; }
static CExoString& GameCat()     { static CExoString s("Game Options"); return s; }
static CExoString& DebugCat()    { static CExoString s("Debug"); return s; }

// Menu Handlers
extern "C" void __cdecl AdditionalOptions_GraphicsHandler(const char* key, const char* value) {
    if (!strcmp(key, "Emitters")) {
        bool* enabled = static_cast<bool*>(GameVersion::GetGlobalPointer("enableEmitters"));
        *enabled = atoi(value) == 1;
    }
    else if (!strcmp(key, "FullScreen")) {
        CClientOptions clientOptions;
        clientOptions.SetFullScreenEnabled(atoi(value));
    }
    else if (!strcmp(key, "Disable Vertex Buffer Objects")) {
        BYTE* disabled = static_cast<BYTE*>(GameVersion::GetGlobalPointer("disableVertexBufferObjects"));
        *disabled = static_cast<BYTE>(atoi(value));
    }
    else if (!strcmp(key, "Disable Write-Only VBO")) {
        int* enabled = static_cast<int*>(GameVersion::GetGlobalPointer("vertexBufferObjectsWriteOnly"));
        *enabled = static_cast<int>(atoi(value) == 0);
    }
    else if (!strcmp(key, "AllowWindowedMode")) {
        int* allowed = static_cast<int*>(GameVersion::GetGlobalPointer("allowWindowedMode"));
        *allowed = atoi(value);
    }
    else if (!strcmp(key, "DisableGUI")) {
        int* disabled = static_cast<int*>(GameVersion::GetGlobalPointer("noGUIRender"));
        *disabled = atoi(value);
    }
    else if (!strcmp(key, "RenderLevel")) {
        int* level = static_cast<int*>(GameVersion::GetGlobalPointer("renderLevel"));
        *level = atoi(value);
    }
    else if (!strcmp(key, "Do3dGui")) {
        int* enabled = static_cast<int*>(GameVersion::GetGlobalPointer("do3DGUI"));
        *enabled = atoi(value);
    }
    else if (!strcmp(key, "VisibilityGraph")) {
        int* enabled = static_cast<int*>(GameVersion::GetGlobalPointer("enableVisibilityGraph"));
        *enabled = atoi(value);
    }
    else if (!strcmp(key, "Beams")) {
        int* enabled = static_cast<int*>(GameVersion::GetGlobalPointer("enableBeams"));
        *enabled = atoi(value);
    }
    else if (!strcmp(key, "DoGrassWind")) {
        int* enabled = static_cast<int*>(GameVersion::GetGlobalPointer("enableGrassWind"));
        *enabled = atoi(value);
    }
    else {
        debugLog("[ExpandedVanillaOptions] Unknown graphics key %s, with length %u", key, strlen(key));
    }
}

extern "C" void __cdecl AdditionalOptions_GameHandler(const char* key, const char* value) {
    CClientOptions options;
    CClientExoApp client;

    if (!strcmp(key, "Keyboard Camera Deceleration")) {
        options.LoadOptions();
    }
    else if (!strcmp(key, "Keyboard Camera Acceleration")) {
        options.LoadOptions();
    }
    else if (!strcmp(key, "Keyboard Camera DPS")) {
        options.LoadOptions();
    }
    else if (!strcmp(key, "TooltipDelay Sec")) {
        options.SetTooltipDelay((float)atof(value));
        CSWGuiManager manager;
        manager.SetTooltipAppearTime((float)atof(value));
    }
    else if (!strcmp(key, "Disable Movies")) {
        client.SetDisableMovies(atoi(value));
    }
    else if (!strcmp(key, "EnableCheats")) {
        options.LoadOptions();
    }
    else {
        debugLog("[ExpandedVanillaOptions] Unknown Game key %s", key);
    }
}

extern "C" void __cdecl AdditionalOptions_DebugHandler(const char* key, const char* value) {
    if (!strcmp(key, "SavePlayerBIC")) {
        int* enabled = static_cast<int*>(GameVersion::GetGlobalPointer("savePlayerBIC"));
        *enabled = atoi(value);
    }
    else if (!strcmp(key, "RenderWalkmesh")) {
        int* enabled = static_cast<int*>(GameVersion::GetGlobalPointer("renderAABBs"));
        *enabled = atoi(value);
    }
    else if (!strcmp(key, "RenderWireFrame")) {
        int* enabled = static_cast<int*>(GameVersion::GetGlobalPointer("renderWireFrame"));
        *enabled = atoi(value);
    }
    else if (!strcmp(key, "RenderTriggers")) {
        int* enabled = static_cast<int*>(GameVersion::GetGlobalPointer("enableRenderTriggers"));
        *enabled = atoi(value);
        enabled = static_cast<int*>(GameVersion::GetGlobalPointer("renderQAtriggers"));
        *enabled = atoi(value);
    }
    else if (!strcmp(key, "RenderPersonalSpace")) {
        int* enabled = static_cast<int*>(GameVersion::GetGlobalPointer("renderPersonalSpace"));
        *enabled = atoi(value);
    }
    else if (!strcmp(key, "RenderBoundingBoxes")) {
        int* enabled = static_cast<int*>(GameVersion::GetGlobalPointer("renderGobBBox"));
        *enabled = atoi(value);
    }
    else if (!strcmp(key, "RenderCollision")) {
        int* enabled = static_cast<int*>(GameVersion::GetGlobalPointer("renderCollisionBoxes"));
        *enabled = atoi(value);
    }
    else {
        debugLog("[ExpandedVanillaOptions] Unknown Debug key %s", key);
    }
}

// Options Hooks
extern "C" void __cdecl LoadOptions_Hook(void* iniPtr) {
    CExoIni ini(iniPtr);
    CExoString value;

    CExoString tooltipDelay("TooltipDelay Sec");
    if (ini.ReadIniEntry(&value, &SwkotorIni(), &GameCat(), &tooltipDelay)) {
        AdditionalOptions_GameHandler(tooltipDelay.GetCStrSafe(), value.GetCStrSafe());
    }
}

extern "C" void __cdecl ReadVideoModeSettings_Hook(void* iniPtr) {
    CExoIni ini(iniPtr);
    CExoString value;

    CExoString emitters("Emitters");
    if (ini.ReadIniEntry(&value, &SwkotorIni(), &GraphicsCat(), &emitters)) {
        AdditionalOptions_GraphicsHandler(emitters.GetCStrSafe(), value.GetCStrSafe());
    }
    CExoString disableGUI("DisableGUI");
    if (ini.ReadIniEntry(&value, &AddlIni(), &GraphicsCat(), &disableGUI)) {
        AdditionalOptions_GraphicsHandler(disableGUI.GetCStrSafe(), value.GetCStrSafe());
    }
    CExoString renderLevel("RenderLevel");
    if (ini.ReadIniEntry(&value, &AddlIni(), &GraphicsCat(), &renderLevel)) {
        AdditionalOptions_GraphicsHandler(renderLevel.GetCStrSafe(), value.GetCStrSafe());
    }
    CExoString do3dGui("Do3dGui");
    if (ini.ReadIniEntry(&value, &AddlIni(), &GraphicsCat(), &do3dGui)) {
        AdditionalOptions_GraphicsHandler(do3dGui.GetCStrSafe(), value.GetCStrSafe());
    }
    CExoString visibilityGraph("VisibilityGraph");
    if (ini.ReadIniEntry(&value, &AddlIni(), &GraphicsCat(), &visibilityGraph)) {
        AdditionalOptions_GraphicsHandler(visibilityGraph.GetCStrSafe(), value.GetCStrSafe());
    }
    CExoString beams("Beams");
    if (ini.ReadIniEntry(&value, &AddlIni(), &GraphicsCat(), &beams)) {
        AdditionalOptions_GraphicsHandler(beams.GetCStrSafe(), value.GetCStrSafe());
    }
    CExoString doGrassWind("DoGrassWind");
    if (ini.ReadIniEntry(&value, &AddlIni(), &GraphicsCat(), &doGrassWind)) {
        AdditionalOptions_GraphicsHandler(doGrassWind.GetCStrSafe(), value.GetCStrSafe());
    }
}

extern "C" void __cdecl InitializeSoundOptions_Hook(void* iniPtr) {
    CExoIni ini(iniPtr);
    // Nothing for now
    // I may add sound settings in the future
}

extern "C" void __cdecl StartServices_Hook(void* iniPtr) {
    CExoIni ini(iniPtr);
    CExoString value;

    CExoString savePlayerBIC("SavePlayerBIC");
    if (ini.ReadIniEntry(&value, &AddlIni(), &DebugCat(), &savePlayerBIC)) {
        AdditionalOptions_DebugHandler(savePlayerBIC.GetCStrSafe(), value.GetCStrSafe());
    }
    CExoString renderWalkmesh("RenderWalkmesh");
    if (ini.ReadIniEntry(&value, &AddlIni(), &DebugCat(), &renderWalkmesh)) {
        AdditionalOptions_DebugHandler(renderWalkmesh.GetCStrSafe(), value.GetCStrSafe());
    }
    CExoString renderWireFrame("RenderWireFrame");
    if (ini.ReadIniEntry(&value, &AddlIni(), &DebugCat(), &renderWireFrame)) {
        AdditionalOptions_DebugHandler(renderWireFrame.GetCStrSafe(), value.GetCStrSafe());
    }
    CExoString renderTrigger("RenderTrigger");
    if (ini.ReadIniEntry(&value, &AddlIni(), &DebugCat(), &renderTrigger)) {
        AdditionalOptions_DebugHandler(renderTrigger.GetCStrSafe(), value.GetCStrSafe());
    }
    CExoString renderPersonalSpace("RenderPersonalSpace");
    if (ini.ReadIniEntry(&value, &AddlIni(), &DebugCat(), &renderPersonalSpace)) {
        AdditionalOptions_DebugHandler(renderPersonalSpace.GetCStrSafe(), value.GetCStrSafe());
    }
    CExoString renderBoundingBoxes("RenderBoundingBoxes");
    if (ini.ReadIniEntry(&value, &AddlIni(), &DebugCat(), &renderBoundingBoxes)) {
        AdditionalOptions_DebugHandler(renderBoundingBoxes.GetCStrSafe(), value.GetCStrSafe());
    }
    CExoString renderCollision("RenderCollision");
    if (ini.ReadIniEntry(&value, &AddlIni(), &DebugCat(), &renderCollision)) {
        AdditionalOptions_DebugHandler(renderCollision.GetCStrSafe(), value.GetCStrSafe());
    }
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        if (!GameVersion::Initialize()) {
            debugLog("[ExpandedVanillaOptions] ERROR: GameVersion::Initialize() failed");
            return FALSE;
        }
        debugLog("[ExpandedVanillaOptions] GameVersion initialized successfully");
        break;

    case DLL_PROCESS_DETACH:
        GameVersion::Reset();
        break;
    }
    return TRUE;
}