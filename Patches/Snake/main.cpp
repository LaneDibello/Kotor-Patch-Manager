#include "snakePanel.h"

#include "GameAPI/ConsoleFunc.h"
#include "GameAPI/CSWGuiManager.h"

void __cdecl snake() {
    CSWGuiManager manager;
    manager.AddPanel(new SnakePanel(&manager), 2, 1);
}

extern "C" void __cdecl addConsoleCommand() {
    new ConsoleFunc("snake", &snake, NO_PARAMS);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        if (!GameVersion::Initialize()) {
            debugLog("[Snake] ERROR: GameVersion::Initialize() failed");
            return FALSE;
        }
        debugLog("[Snake] GameVersion initialized successfully");
        break;

    case DLL_PROCESS_DETACH:
        GameVersion::Reset();
        break;
    }
    return TRUE;
}