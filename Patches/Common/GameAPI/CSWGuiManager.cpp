#include "CSWGuiManager.h"
#include "CSWGuiPanel.h"
#include "GameVersion.h"

CSWGuiManager::AddPanelFn CSWGuiManager::addPanel = nullptr;

bool CSWGuiManager::functionsInitialized = false;
bool CSWGuiManager::offsetsInitialized = false;

int CSWGuiManager::offsetViewportWidth  = -1;
int CSWGuiManager::offsetViewportHeight = -1;
int CSWGuiManager::offsetPanels         = -1;

void CSWGuiManager::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiManager] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        addPanel = reinterpret_cast<AddPanelFn>(GameVersion::GetFunctionAddress("CSWGuiManager", "AddPanel"));

        functionsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiManager] ERROR: %s\n", e.what());
        return;
    }
}

void CSWGuiManager::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiManager] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        offsetViewportWidth  = GameVersion::GetOffset("CSWGuiManager", "viewport_width");
        offsetViewportHeight = GameVersion::GetOffset("CSWGuiManager", "viewport_height");
        offsetPanels         = GameVersion::GetOffset("CSWGuiManager", "panels");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiManager] ERROR: %s\n", e.what());
    }
}

CSWGuiManager::CSWGuiManager(void* objectPtr)
    : GameAPIObject(objectPtr, false)  // false = don't free (singleton)
{
    InitializeFunctions();
    InitializeOffsets();
}

CSWGuiManager::CSWGuiManager()
    : GameAPIObject(nullptr, false)  // false = don't free (singleton)
{
    InitializeFunctions();
    InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiManager] ERROR: GameVersion not initialized\n");
        return;
    }

    // GUI_MANAGER_PTR holds the ADDRESS of the global that stores the manager
    // pointer, so dereference once to reach the actual instance.
    void** managerPtrAddr = static_cast<void**>(GameVersion::GetGlobalPointer("GUI_MANAGER_PTR"));
    if (managerPtrAddr && *managerPtrAddr) {
        objectPtr = *managerPtrAddr;
    } else {
        OutputDebugStringA("[CSWGuiManager] ERROR: GUI_MANAGER_PTR is null\n");
    }
}

CSWGuiManager::~CSWGuiManager()
{
    // Base class destructor handles objectPtr cleanup (we don't own the singleton)
}

short CSWGuiManager::GetViewportWidth() {
    if (!objectPtr || offsetViewportWidth < 0) {
        return 0;
    }
    return getObjectProperty<short>(objectPtr, offsetViewportWidth);
}

void CSWGuiManager::SetViewportWidth(short width) {
    if (!objectPtr || offsetViewportWidth < 0) {
        return;
    }
    setObjectProperty<short>(objectPtr, offsetViewportWidth, width);
}

short CSWGuiManager::GetViewportHeight() {
    if (!objectPtr || offsetViewportHeight < 0) {
        return 0;
    }
    return getObjectProperty<short>(objectPtr, offsetViewportHeight);
}

void CSWGuiManager::SetViewportHeight(short height) {
    if (!objectPtr || offsetViewportHeight < 0) {
        return;
    }
    setObjectProperty<short>(objectPtr, offsetViewportHeight, height);
}

CExoArrayList<CSWGuiPanel*>* CSWGuiManager::GetPanels() {
    if (!objectPtr || offsetPanels < 0) {
        return nullptr;
    }
    // Inline CExoArrayList member: wrap its in-place address.
    return new CExoArrayList<CSWGuiPanel*>((char*)objectPtr + offsetPanels);
}

void CSWGuiManager::AddPanel(CSWGuiPanel* panel, int flags, int playSound) {
    if (!objectPtr || !addPanel) return;
    addPanel(objectPtr, panel ? panel->GetPtr() : nullptr, flags, playSound);
}
