#include "snakePanel.h"

#include "GameAPI/ConsoleFunc.h"
#include "GameAPI/CSWGuiManager.h"

void __cdecl snake() {
    CSWGuiManager manager;
    manager.AddPanel(new SnakePanel(&manager), 2, 1);
}

void addConsoleCommand() {
    new ConsoleFunc("snake", &snake, NO_PARAMS);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        if (!GameVersion::Initialize()) {
            debugLog("[ScriptExtender] ERROR: GameVersion::Initialize() failed");
            return FALSE;
        }
        debugLog("[ScriptExtender] GameVersion initialized successfully");
        addConsoleCommand();
        break;

    case DLL_PROCESS_DETACH:
        GameVersion::Reset();
        break;
    }
    return TRUE;
}