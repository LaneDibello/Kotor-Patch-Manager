#include "TestGUI.h"

CSWGuiButton* testButton;

//Params are unused, but need to match call convention
void __fastcall CreateTestGui(void*, void*, void*) {
    debugLog("Creating Test GUI");
    CSWGuiManager* manager = new CSWGuiManager();

    TestGUI* gui = new TestGUI(manager);

    manager->AddPanel(gui, 3, 1);
    debugLog("Test GUI Created");
}

extern "C" void __cdecl InjectOptionsButton(void* thisPanel) {
    debugLog("Injecting Test Objects Button");
    testButton = new CSWGuiButton();
    CExoString buttonTag("BTN_TEST");
    CSWGuiPanel panel(thisPanel);
    panel.InitControl(testButton, &buttonTag, 1);
    debugLog("Test Objects Button Injected");
}

extern "C" void __cdecl AddCreateEvent(void* thisPanel) {
    debugLog("Adding Event to Test Options Button");
    CSWGuiPanel panel(thisPanel);
    testButton->AddEvent(0x27, &panel, &CreateTestGui);
    debugLog("Added Event to Test Options Button");
}

// DLL Entry Point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        if (!GameVersion::Initialize()) {
            OutputDebugStringA("[AdditionalConsoleCommands] ERROR: GameVersion::Initialize() failed\n");
            return FALSE;
        }
        OutputDebugStringA("[AdditionalConsoleCommands] GameVersion initialized successfully\n");
        break;

    case DLL_PROCESS_DETACH:
        GameVersion::Reset();
        break;
    }
    return TRUE;
}