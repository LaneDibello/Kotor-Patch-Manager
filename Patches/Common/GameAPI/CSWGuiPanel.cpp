#include "CSWGuiPanel.h"
#include "GameVersion.h"
#include "CSWGuiControl.h"
#include "CSWGuiBorder.h"
#include "CExoArrayList.h"
#include "CExoString.h"
#include "CResRef.h"

CSWGuiPanel::AddControlFn                       CSWGuiPanel::addControl                       = nullptr;
CSWGuiPanel::CenterPanelFn                      CSWGuiPanel::centerPanel                      = nullptr;
CSWGuiPanel::GetControlFn                       CSWGuiPanel::getControl                       = nullptr;
CSWGuiPanel::GetExtentAccountingForPanelOffsetFn CSWGuiPanel::getExtentAccountingForPanelOffset = nullptr;
CSWGuiPanel::GetFullScreenBGFn                  CSWGuiPanel::getFullScreenBG                  = nullptr;
CSWGuiPanel::GetLocalMouseCoordsFn              CSWGuiPanel::getLocalMouseCoords              = nullptr;
CSWGuiPanel::HitCheckMouseFn                    CSWGuiPanel::hitCheckMouse                    = nullptr;
CSWGuiPanel::InitControlFn                      CSWGuiPanel::initControl                      = nullptr;
CSWGuiPanel::ResetFontFn                        CSWGuiPanel::resetFont                        = nullptr;
CSWGuiPanel::SetActiveControlFn                 CSWGuiPanel::setActiveControl                 = nullptr;
CSWGuiPanel::SetBackgroundFn                    CSWGuiPanel::setBackground                    = nullptr;
CSWGuiPanel::SetVisibleFn                       CSWGuiPanel::setVisible                       = nullptr;
CSWGuiPanel::StartLoadFromLayoutFn              CSWGuiPanel::startLoadFromLayout              = nullptr;
CSWGuiPanel::StopLoadFromLayoutFn               CSWGuiPanel::stopLoadFromLayout               = nullptr;

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
        addControl                       = reinterpret_cast<AddControlFn>                      (GameVersion::GetFunctionAddress("CSWGuiPanel", "AddControl"));
        centerPanel                      = reinterpret_cast<CenterPanelFn>                     (GameVersion::GetFunctionAddress("CSWGuiPanel", "CenterPanel"));
        getControl                       = reinterpret_cast<GetControlFn>                      (GameVersion::GetFunctionAddress("CSWGuiPanel", "GetControl"));
        getExtentAccountingForPanelOffset = reinterpret_cast<GetExtentAccountingForPanelOffsetFn>(GameVersion::GetFunctionAddress("CSWGuiPanel", "GetExtentAccountingForPanelOffset"));
        getFullScreenBG                  = reinterpret_cast<GetFullScreenBGFn>                 (GameVersion::GetFunctionAddress("CSWGuiPanel", "GetFullScreenBG"));
        getLocalMouseCoords              = reinterpret_cast<GetLocalMouseCoordsFn>             (GameVersion::GetFunctionAddress("CSWGuiPanel", "GetLocalMouseCoords"));
        hitCheckMouse                    = reinterpret_cast<HitCheckMouseFn>                   (GameVersion::GetFunctionAddress("CSWGuiPanel", "HitCheckMouse"));
        initControl                      = reinterpret_cast<InitControlFn>                     (GameVersion::GetFunctionAddress("CSWGuiPanel", "InitControl"));
        resetFont                        = reinterpret_cast<ResetFontFn>                       (GameVersion::GetFunctionAddress("CSWGuiPanel", "ResetFont"));
        setActiveControl                 = reinterpret_cast<SetActiveControlFn>                (GameVersion::GetFunctionAddress("CSWGuiPanel", "SetActiveControl"));
        setBackground                    = reinterpret_cast<SetBackgroundFn>                   (GameVersion::GetFunctionAddress("CSWGuiPanel", "SetBackground"));
        setVisible                       = reinterpret_cast<SetVisibleFn>                      (GameVersion::GetFunctionAddress("CSWGuiPanel", "SetVisible"));
        startLoadFromLayout              = reinterpret_cast<StartLoadFromLayoutFn>             (GameVersion::GetFunctionAddress("CSWGuiPanel", "StartLoadFromLayout"));
        stopLoadFromLayout               = reinterpret_cast<StopLoadFromLayoutFn>              (GameVersion::GetFunctionAddress("CSWGuiPanel", "StopLoadFromLayout"));

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

void CSWGuiPanel::AddControl(CSWGuiControl* control) {
    if (!objectPtr || !addControl) return;
    addControl(objectPtr, control ? control->GetPtr() : nullptr);
}

void CSWGuiPanel::CenterPanel() {
    if (!objectPtr || !centerPanel) return;
    centerPanel(objectPtr);
}

CSWGuiControl* CSWGuiPanel::GetControl(int index) {
    if (!objectPtr || !getControl) return nullptr;
    void* ctrlPtr = getControl(objectPtr, index);
    if (!ctrlPtr) return nullptr;
    return new CSWGuiControl(ctrlPtr);
}

void CSWGuiPanel::GetExtentAccountingForPanelOffset(CSWGuiExtent* outExtent) {
    if (!objectPtr || !getExtentAccountingForPanelOffset || !outExtent) return;
    getExtentAccountingForPanelOffset(objectPtr, outExtent);
}

void CSWGuiPanel::GetFullScreenBG(CExoString* outBGString) {
    if (!objectPtr || !getFullScreenBG || !outBGString) return;
    getFullScreenBG(objectPtr, outBGString->GetPtr());
}

void CSWGuiPanel::GetLocalMouseCoords(int* outX, int* outY) {
    if (!objectPtr || !getLocalMouseCoords) return;
    getLocalMouseCoords(objectPtr, outX, outY);
}

bool CSWGuiPanel::HitCheckMouse(int mouseX, int mouseY) {
    if (!objectPtr || !hitCheckMouse) return false;
    return hitCheckMouse(objectPtr, mouseX, mouseY);
}

void CSWGuiPanel::InitControl(CSWGuiControl* controlToInit, CExoString* label, int activate) {
    if (!objectPtr || !initControl) return;
    initControl(objectPtr,
                controlToInit ? controlToInit->GetPtr() : nullptr,
                label ? label->GetPtr() : nullptr,
                activate);
}

void CSWGuiPanel::ResetFont() {
    if (!objectPtr || !resetFont) return;
    resetFont(objectPtr);
}

void CSWGuiPanel::SetActiveControl(CSWGuiControl* controlToActivate, int playSound) {
    if (!objectPtr || !setActiveControl) return;
    setActiveControl(objectPtr,
                     controlToActivate ? controlToActivate->GetPtr() : nullptr,
                     playSound);
}

void CSWGuiPanel::SetBackground(CResRef* image) {
    if (!objectPtr || !setBackground) return;
    setBackground(objectPtr, image ? image->GetPtr() : nullptr);
}

void CSWGuiPanel::SetVisible(int isVisible) {
    if (!objectPtr || !setVisible) return;
    setVisible(objectPtr, isVisible);
}

void CSWGuiPanel::StartLoadFromLayout(CResRef* guiResref) {
    if (!objectPtr || !startLoadFromLayout) return;
    startLoadFromLayout(objectPtr, guiResref ? guiResref->GetPtr() : nullptr);
}

void CSWGuiPanel::StopLoadFromLayout() {
    if (!objectPtr || !stopLoadFromLayout) return;
    stopLoadFromLayout(objectPtr);
}
