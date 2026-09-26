#include "AurGUI.h"
#include "GameVersion.h"

namespace {
    typedef void (__stdcall* StartLayerFn)();
    typedef void (__stdcall* StopLayerFn)();
    typedef int  (__cdecl*   SetupViewportFn)(int left, int top, int width, int height, Vector* color, int param6, float alpha);
    typedef void (__stdcall* CloseViewportFn)();

    StartLayerFn    startLayer    = nullptr;
    StopLayerFn     stopLayer     = nullptr;
    SetupViewportFn setupViewport = nullptr;
    CloseViewportFn closeViewport = nullptr;

    bool functionsInitialized = false;

    void InitializeFunctions() {
        if (functionsInitialized) {
            return;
        }

        if (!GameVersion::IsInitialized()) {
            OutputDebugStringA("[AurGUI] ERROR: GameVersion not initialized\n");
            return;
        }

        GameVersion::ResolveFunction(startLayer,    "Global", "AurGUIStartLayer");
        GameVersion::ResolveFunction(stopLayer,     "Global", "AurGUIStopLayer");
        GameVersion::ResolveFunction(setupViewport, "Global", "AurGUISetupViewport");
        GameVersion::ResolveFunction(closeViewport, "Global", "AurGUICloseViewport");

        functionsInitialized = true;
    }
}

void AurGUI::StartLayer() {
    InitializeFunctions();
    if (!startLayer) return;
    startLayer();
}

void AurGUI::StopLayer() {
    InitializeFunctions();
    if (!stopLayer) return;
    stopLayer();
}

int AurGUI::SetupViewport(int left, int top, int width, int height, Vector* color, bool param6, float alpha) {
    InitializeFunctions();
    if (!setupViewport) return 0;
    return setupViewport(left, top, width, height, color, param6 ? 1 : 0, alpha);
}

void AurGUI::CloseViewport() {
    InitializeFunctions();
    if (!closeViewport) return;
    closeViewport();
}
