#include "CSWGuiListBox.h"
#include "GameVersion.h"
#include "CSWGuiBorder.h"
#include "CSWGuiScrollBar.h"
#include "CSWGuiControl.h"
#include "CExoArrayList.h"

bool CSWGuiListBox::functionsInitialized = false;
bool CSWGuiListBox::offsetsInitialized = false;

int CSWGuiListBox::offsetBorder = -1;
int CSWGuiListBox::offsetScrollbar = -1;
int CSWGuiListBox::offsetControls = -1;
int CSWGuiListBox::offsetControlExtents = -1;
int CSWGuiListBox::offsetHoveredControl = -1;
int CSWGuiListBox::offsetColor = -1;

void CSWGuiListBox::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWGuiNavigable::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiListBox] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Functions Here

        functionsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiListBox] ERROR: %s\n", e.what());
        return;
    }
}

void CSWGuiListBox::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWGuiNavigable::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiListBox] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        offsetBorder         = GameVersion::GetOffset("CSWGuiListBox", "border");
        offsetScrollbar      = GameVersion::GetOffset("CSWGuiListBox", "scrollbar");
        offsetControls       = GameVersion::GetOffset("CSWGuiListBox", "controls");
        offsetControlExtents = GameVersion::GetOffset("CSWGuiListBox", "control_extents");
        offsetHoveredControl = GameVersion::GetOffset("CSWGuiListBox", "hovered_control");
        offsetColor          = GameVersion::GetOffset("CSWGuiListBox", "color");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiListBox] ERROR: %s\n", e.what());
    }
}

CSWGuiListBox::CSWGuiListBox(void* objectPtr)
    : CSWGuiNavigable(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWGuiListBox::~CSWGuiListBox()
{
    // Base class destructor handles objectPtr cleanup
}

CSWGuiBorder* CSWGuiListBox::GetBorder() {
    if (!objectPtr || offsetBorder < 0) {
        return nullptr;
    }
    return new CSWGuiBorder((char*)objectPtr + offsetBorder);
}

CSWGuiScrollBar* CSWGuiListBox::GetScrollbar() {
    if (!objectPtr || offsetScrollbar < 0) {
        return nullptr;
    }
    return new CSWGuiScrollBar((char*)objectPtr + offsetScrollbar);
}

CExoArrayList<CSWGuiControl*>* CSWGuiListBox::GetControls() {
    if (!objectPtr || offsetControls < 0) {
        return nullptr;
    }
    return new CExoArrayList<CSWGuiControl*>((char*)objectPtr + offsetControls);
}

CExoArrayList<CSWGuiExtent>* CSWGuiListBox::GetControlExtents() {
    if (!objectPtr || offsetControlExtents < 0) {
        return nullptr;
    }
    return new CExoArrayList<CSWGuiExtent>((char*)objectPtr + offsetControlExtents);
}

CSWGuiControl* CSWGuiListBox::GetHoveredControl() {
    if (!objectPtr || offsetHoveredControl < 0) {
        return nullptr;
    }
    void* ctrlPtr = getObjectProperty<void*>(objectPtr, offsetHoveredControl);
    if (!ctrlPtr) {
        return nullptr;
    }
    return new CSWGuiControl(ctrlPtr);
}

Vector CSWGuiListBox::GetColor() {
    Vector result = {0.0f, 0.0f, 0.0f};
    if (!objectPtr || offsetColor < 0) {
        return result;
    }
    return getObjectProperty<Vector>(objectPtr, offsetColor);
}

void CSWGuiListBox::SetColor(const Vector& color) {
    if (!objectPtr || offsetColor < 0) {
        return;
    }
    setObjectProperty<Vector>(objectPtr, offsetColor, color);
}
