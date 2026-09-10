#include "Common.h"
#include "GameAPI/CExoIni.h"
#include "GameAPI/CExoString.h"

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
    else if (key == "FullScreen") {

    }
    else if (key == "Disable Vertex Buffer Objects") {

    }
    else if (key == "AllowWindowedMode") {

    }
    else if (key == "DisableGUI") {

    }
    else if (key == "RenderLevel") {

    }
    else if (key == "Do3dGui") {

    }
    else if (key == "VisibilityGraph") {

    }
    else if (key == "DisableSaturation") {

    }
    else if (key == "ScanNoise") {

    }
    else if (key == "FilmNoise") {

    }
    else if (key == "Beams") {

    }
    else if (key == "DoGrassWind") {

    }
    else {
        debugLog("[ExpandedVanillaOptions] Unknown graphics key %s, with length %u", key, strlen(key));
    }
}

extern "C" void __cdecl AdditionalOptions_SoundHandler(const char* key, const char* value) {
    if (key == "Disable Sound") {

    }
    else if (key == "Number 3D Voices") {

    }
    else if (key == "Number 2D Voices") {

    }
    else if (key == "2D3D Bias") {

    }
    else if (key == "Environment Effects Nonstreaming") {

    }
    else if (key == "Environment Effects Streaming") {

    }
    else {
        debugLog("[ExpandedVanillaOptions] Unknown Sound key %s", key);
    }
}

extern "C" void __cdecl AdditionalOptions_GameHandler(const char* key, const char* value) {
    if (key == "GUIsInScreenShot") {

    }
    else if (key == "EnableScreenShot") {

    }
    else if (key == "Keyboard Camera Deceleration") {

    }
    else if (key == "Keyboard Camera Acceleration") {

    }
    else if (key == "Keyboard Camera DPS") {

    }
    else if (key == "Enable Mouse Teleporting To Buttons") {

    }
    else if (key == "TooltipDelay Sec") {

    }
    else if (key == "Disable Movies") {

    }
    else if (key == "EnableCheats") {

    }
    else {
        debugLog("[ExpandedVanillaOptions] Unknown Game key %s", key);
    }
}

extern "C" void __cdecl AdditionalOptions_DebugHandler(const char* key, const char* value) {
    if (key == "SavePlayerBIC") {

    }
    else if (key == "RenderingWalkmesh") {

    }
    else if (key == "RenderingWireFrame") {

    }
    else if (key == "RenderingTrigger") {

    }
    else if (key == "RenderPersonalSpace") {

    }
    else if (key == "RenderBoundingBoxes") {

    }
    else if (key == "RenderCollision") {

    }
    else {
        debugLog("[ExpandedVanillaOptions] Unknown Debug key %s", key);
    }
}

// Options Hooks
extern "C" void __cdecl LoadOptions_Hook(void* iniPtr) {
    CExoIni ini(iniPtr);

}

extern "C" void __cdecl ReadVideoModeSettings_Hook(void* iniPtr) {
    CExoIni ini(iniPtr);
    CExoString value;

    CExoString emitters("Emitters");
    if (ini.ReadIniEntry(&value, &SwkotorIni(), &GraphicsCat(), &emitters)) {
        AdditionalOptions_GraphicsHandler(emitters.GetCStrSafe(), value.GetCStrSafe());
    }

}

extern "C" void __cdecl InitializeSoundOptions_Hook(void* iniPtr) {
    CExoIni ini(iniPtr);

}

extern "C" void __cdecl StartServices_Hook(void* iniPtr) {
    CExoIni ini(iniPtr);

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