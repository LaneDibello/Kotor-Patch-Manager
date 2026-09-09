#include "Common.h"

extern "C" void __cdecl AdditionalOptions_GraphicsHandler(const char* key, const char* value) {
    if (key == "Emitters") {

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
        debugLog("[ExpandedVanillaOptions] Unknown graphics key %s", key);
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