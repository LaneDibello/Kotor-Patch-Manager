#include "ModOptions.h"

CSWGuiButton* modOptionsButton;

void __fastcall CreateModOptionsGui(void*, void*, void*) {
    CSWGuiManager* manager = new CSWGuiManager();

    ModOptions* gui = new ModOptions(manager);

    manager->AddPanel(gui, 3, 1);
}

extern "C" void __cdecl InjectOptionsButton(void* thisPanel) {
    modOptionsButton = new CSWGuiButton();
    CExoString buttonTag("BTN_MOD_OPTIONS");
    CSWGuiPanel panel(thisPanel);
    panel.InitControl(modOptionsButton, &buttonTag, 1);
}

extern "C" void __cdecl AddCreateEvent(void* thisPanel) {
    CSWGuiPanel panel(thisPanel);
    modOptionsButton->AddEvent(0x27, &panel, &CreateModOptionsGui);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        if (!GameVersion::Initialize()) {
            debugLog("[ModOptions] ERROR: GameVersion::Initialize() failed");
            return FALSE;
        }
        debugLog("[ModOptions] GameVersion initialized successfully");
        break;

    case DLL_PROCESS_DETACH:
        GameVersion::Reset();
        break;
    }
    return TRUE;
}