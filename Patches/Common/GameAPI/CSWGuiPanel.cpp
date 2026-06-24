#include "CSWGuiPanel.h"
#include "GameVersion.h"
#include "CSWGuiControl.h"
#include "CSWGuiBorder.h"
#include "CExoArrayList.h"

bool CSWGuiPanel::functionsInitialized = false;
bool CSWGuiPanel::offsetsInitialized = false;

int CSWGuiPanel::offsetActiveControl = -1;
int CSWGuiPanel::offsetControls = -1;
int CSWGuiPanel::offsetAlpha = -1;
int CSWGuiPanel::offsetColor = -1;
int CSWGuiPanel::offsetBorder = -1;

void CSWGuiPanel::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWGuiObject::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiPanel] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Functions Here

        functionsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiPanel] ERROR: %s\n", e.what());
        return;
    }
}

void CSWGuiPanel::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWGuiObject::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiPanel] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        offsetActiveControl = GameVersion::GetOffset("CSWGuiPanel", "active_control");
        offsetControls      = GameVersion::GetOffset("CSWGuiPanel", "controls");
        offsetAlpha         = GameVersion::GetOffset("CSWGuiPanel", "alpha");
        offsetColor         = GameVersion::GetOffset("CSWGuiPanel", "color");
        offsetBorder        = GameVersion::GetOffset("CSWGuiPanel", "border");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiPanel] ERROR: %s\n", e.what());
    }
}

CSWGuiPanel::CSWGuiPanel(void* objectPtr)
    : CSWGuiObject(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWGuiPanel::~CSWGuiPanel()
{
    // Base class destructor handles objectPtr cleanup
}

CSWGuiControl* CSWGuiPanel::GetActiveControl() {
    if (!objectPtr || offsetActiveControl < 0) {
        return nullptr;
    }
    void* ctrlPtr = getObjectProperty<void*>(objectPtr, offsetActiveControl);
    if (!ctrlPtr) {
        return nullptr;
    }
    return new CSWGuiControl(ctrlPtr);
}

CExoArrayList<CSWGuiControl*>* CSWGuiPanel::GetControls() {
    if (!objectPtr || offsetControls < 0) {
        return nullptr;
    }
    return new CExoArrayList<CSWGuiControl*>((char*)objectPtr + offsetControls);
}

float CSWGuiPanel::GetAlpha() {
    if (!objectPtr || offsetAlpha < 0) {
        return 0.0f;
    }
    return getObjectProperty<float>(objectPtr, offsetAlpha);
}

void CSWGuiPanel::SetAlpha(float alpha) {
    if (!objectPtr || offsetAlpha < 0) {
        return;
    }
    setObjectProperty<float>(objectPtr, offsetAlpha, alpha);
}

Vector CSWGuiPanel::GetColor() {
    Vector result = {0.0f, 0.0f, 0.0f};
    if (!objectPtr || offsetColor < 0) {
        return result;
    }
    return getObjectProperty<Vector>(objectPtr, offsetColor);
}

void CSWGuiPanel::SetColor(const Vector& color) {
    if (!objectPtr || offsetColor < 0) {
        return;
    }
    setObjectProperty<Vector>(objectPtr, offsetColor, color);
}

CSWGuiBorder* CSWGuiPanel::GetBorder() {
    if (!objectPtr || offsetBorder < 0) {
        return nullptr;
    }
    void* borderPtr = getObjectProperty<void*>(objectPtr, offsetBorder);
    if (!borderPtr) {
        return nullptr;
    }
    return new CSWGuiBorder(borderPtr);
}
