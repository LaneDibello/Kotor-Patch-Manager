#include "Common.h"
#include "Kotor1Functions.h"
#include "ConsoleFunc.h"

void __cdecl runscript(char* script) {
    CExoString scriptFile(script);

    DWORD playerId = serverExoAppGetPlayerCreatureId(getServerExoApp());

    virtualMachineRunScript(*VIRTUAL_MACHINE_PTR, &scriptFile, playerId, 1);
}

extern "C" void __cdecl InitializeAdditionalCommands()
{
    ConsoleFunc* command_runscript = new ConsoleFunc("runscript", (void*)&runscript, STRING_PARAM);
}

// DLL Entry Point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        break;

    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}