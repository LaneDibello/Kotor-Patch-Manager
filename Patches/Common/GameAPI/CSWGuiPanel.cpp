#include "CSWGuiPanel.h"
#include "GameVersion.h"
#include "CSWGuiControl.h"
#include "CSWGuiBorder.h"
#include "CSWGuiManager.h"
#include "CExoArrayList.h"
#include "CExoString.h"
#include "CResRef.h"

CSWGuiPanel::AddControlFn                        CSWGuiPanel::addControl                        = nullptr;
CSWGuiPanel::CenterPanelFn                       CSWGuiPanel::centerPanel                       = nullptr;
CSWGuiPanel::HandleInputEventFn                  CSWGuiPanel::handleInputEvent                  = nullptr;
CSWGuiPanel::DrawFn                              CSWGuiPanel::draw                              = nullptr;
CSWGuiPanel::OnPanelAddedFn                      CSWGuiPanel::onPanelAdded                      = nullptr;
CSWGuiPanel::OnPanelRemovedFn                    CSWGuiPanel::onPanelRemoved                    = nullptr;
CSWGuiPanel::GetControlFn                        CSWGuiPanel::getControl                        = nullptr;
CSWGuiPanel::GetExtentAccountingForPanelOffsetFn CSWGuiPanel::getExtentAccountingForPanelOffset = nullptr;
CSWGuiPanel::GetFullScreenBGFn                   CSWGuiPanel::getFullScreenBG                   = nullptr;
CSWGuiPanel::GetLocalMouseCoordsFn               CSWGuiPanel::getLocalMouseCoords               = nullptr;
CSWGuiPanel::HitCheckMouseFn                     CSWGuiPanel::hitCheckMouse                     = nullptr;
CSWGuiPanel::InitControlFn                       CSWGuiPanel::initControl                       = nullptr;
CSWGuiPanel::ResetFontFn                         CSWGuiPanel::resetFont                         = nullptr;
CSWGuiPanel::SetActiveControlFn                  CSWGuiPanel::setActiveControl                  = nullptr;
CSWGuiPanel::SetBackgroundFn                     CSWGuiPanel::setBackground                     = nullptr;
CSWGuiPanel::SetVisibleFn                        CSWGuiPanel::setVisible                        = nullptr;
CSWGuiPanel::StartLoadFromLayoutFn               CSWGuiPanel::startLoadFromLayout               = nullptr;
CSWGuiPanel::StopLoadFromLayoutFn                CSWGuiPanel::stopLoadFromLayout                = nullptr;
CSWGuiPanel::ConstructorFn                       CSWGuiPanel::constructor                       = nullptr;
CSWGuiPanel::DestructorFn                        CSWGuiPanel::destructor                        = nullptr;

bool CSWGuiPanel::functionsInitialized = false;
bool CSWGuiPanel::offsetsInitialized = false;

int CSWGuiPanel::offsetActiveControl = -1;
int CSWGuiPanel::offsetControls = -1;
int CSWGuiPanel::offsetAlpha = -1;
int CSWGuiPanel::offsetColor = -1;
int CSWGuiPanel::offsetBorder = -1;
int CSWGuiPanel::classSize = -1;

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
        addControl                        = reinterpret_cast<AddControlFn>                       (GameVersion::GetFunctionAddress("CSWGuiPanel", "AddControl"));
        centerPanel                       = reinterpret_cast<CenterPanelFn>                      (GameVersion::GetFunctionAddress("CSWGuiPanel", "CenterPanel"));
        handleInputEvent                  = reinterpret_cast<HandleInputEventFn>                 (GameVersion::GetFunctionAddress("CSWGuiPanel", "HandleInputEvent"));
        draw                              = reinterpret_cast<DrawFn>                             (GameVersion::GetFunctionAddress("CSWGuiPanel", "Draw"));
        onPanelAdded                      = reinterpret_cast<OnPanelAddedFn>                     (GameVersion::GetFunctionAddress("CSWGuiPanel", "OnPanelAdded"));
        onPanelRemoved                    = reinterpret_cast<OnPanelRemovedFn>                   (GameVersion::GetFunctionAddress("CSWGuiPanel", "OnPanelRemoved"));
        getControl                        = reinterpret_cast<GetControlFn>                       (GameVersion::GetFunctionAddress("CSWGuiPanel", "GetControl"));
        getExtentAccountingForPanelOffset = reinterpret_cast<GetExtentAccountingForPanelOffsetFn>(GameVersion::GetFunctionAddress("CSWGuiPanel", "GetExtentAccountingForPanelOffset"));
        getFullScreenBG                   = reinterpret_cast<GetFullScreenBGFn>                  (GameVersion::GetFunctionAddress("CSWGuiPanel", "GetFullScreenBG"));
        getLocalMouseCoords               = reinterpret_cast<GetLocalMouseCoordsFn>              (GameVersion::GetFunctionAddress("CSWGuiPanel", "GetLocalMouseCoords"));
        hitCheckMouse                     = reinterpret_cast<HitCheckMouseFn>                    (GameVersion::GetFunctionAddress("CSWGuiPanel", "HitCheckMouse"));
        initControl                       = reinterpret_cast<InitControlFn>                      (GameVersion::GetFunctionAddress("CSWGuiPanel", "InitControl"));
        resetFont                         = reinterpret_cast<ResetFontFn>                        (GameVersion::GetFunctionAddress("CSWGuiPanel", "ResetFont"));
        setActiveControl                  = reinterpret_cast<SetActiveControlFn>                 (GameVersion::GetFunctionAddress("CSWGuiPanel", "SetActiveControl"));
        setBackground                     = reinterpret_cast<SetBackgroundFn>                    (GameVersion::GetFunctionAddress("CSWGuiPanel", "SetBackground"));
        setVisible                        = reinterpret_cast<SetVisibleFn>                       (GameVersion::GetFunctionAddress("CSWGuiPanel", "SetVisible"));
        startLoadFromLayout               = reinterpret_cast<StartLoadFromLayoutFn>              (GameVersion::GetFunctionAddress("CSWGuiPanel", "StartLoadFromLayout"));
        stopLoadFromLayout                = reinterpret_cast<StopLoadFromLayoutFn>               (GameVersion::GetFunctionAddress("CSWGuiPanel", "StopLoadFromLayout"));
        constructor                       = reinterpret_cast<ConstructorFn>                      (GameVersion::GetFunctionAddress("CSWGuiPanel", "Constructor"));
        destructor                        = reinterpret_cast<DestructorFn>                       (GameVersion::GetFunctionAddress("CSWGuiPanel", "Destructor"));

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
        classSize           = GameVersion::GetClassSize("CSWGuiPanel");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiPanel] ERROR: %s\n", e.what());
    }
}

// Direct wrappers for the game functions (called by address, like the other panel
// functions). Used to invoke the original behavior.
void CSWGuiPanel::HandleInputEvent(int event, int param2) {
    if (!objectPtr || !handleInputEvent) return;
    handleInputEvent(objectPtr, event, param2);
}

void CSWGuiPanel::Draw(float param1) {
    if (!objectPtr || !draw) return;
    draw(objectPtr, param1);
}

void CSWGuiPanel::OnPanelAdded() {
    if (!objectPtr || !onPanelAdded) return;
    onPanelAdded(objectPtr);
}

void CSWGuiPanel::OnPanelRemoved() {
    if (!objectPtr || !onPanelRemoved) return;
    onPanelRemoved(objectPtr);
}

// Installed into the HandleInputEvent vtable slot. The game invokes it as
// __thiscall (game object in ECX, (event, param2) on the stack); __fastcall is the
// stand-in (gameObj -> ECX, dummy -> EDX, args off the stack -- same trick as
// CreateTestGui in exports.cpp). We recover the wrapper via the override's
// back-pointer and forward to the registered handler with the wrapper as `this`,
// so handler member functions resolve their members correctly.
void __fastcall CSWGuiPanel::HandleInputEventThunk(void* gameObj, void* /*edx*/, int event, int param2) {
    CSWGuiPanel* self = static_cast<CSWGuiPanel*>(VTableOverride::GetOwner(gameObj));
    if (!self || !self->inputEventHandler) return;

    auto handler = reinterpret_cast<void(__thiscall*)(void*, int, int)>(self->inputEventHandler);
    handler(self, event, param2);
}

void __fastcall CSWGuiPanel::DrawThunk(void* gameObj, void* /*edx*/, float param1) {
    CSWGuiPanel* self = static_cast<CSWGuiPanel*>(VTableOverride::GetOwner(gameObj));
    if (!self || !self->drawHandler) return;

    auto handler = reinterpret_cast<void(__thiscall*)(void*, float)>(self->drawHandler);
    handler(self, param1);
}

void __fastcall CSWGuiPanel::OnPanelAddedThunk(void* gameObj, void* /*edx*/) {
    CSWGuiPanel* self = static_cast<CSWGuiPanel*>(VTableOverride::GetOwner(gameObj));
    if (!self || !self->onPanelAddedHandler) return;

    auto handler = reinterpret_cast<void(__thiscall*)(void*)>(self->onPanelAddedHandler);
    handler(self);
}

void __fastcall CSWGuiPanel::OnPanelRemovedThunk(void* gameObj, void* /*edx*/) {
    CSWGuiPanel* self = static_cast<CSWGuiPanel*>(VTableOverride::GetOwner(gameObj));
    if (!self || !self->onPanelRemovedHandler) return;

    auto handler = reinterpret_cast<void(__thiscall*)(void*)>(self->onPanelRemovedHandler);
    handler(self);
}

void __fastcall CSWGuiPanel::UpdateThunk(void* gameObj, void* /*edx*/, float param1) {
    CSWGuiPanel* self = static_cast<CSWGuiPanel*>(VTableOverride::GetOwner(gameObj));
    if (!self || !self->updateHandler) return;

    auto handler = reinterpret_cast<void(__thiscall*)(void*, float)>(self->updateHandler);
    handler(self, param1);
}

bool CSWGuiPanel::EnsureVTableOverride() {
    if (vtableOverride) {
        return vtableOverride->IsActive();
    }
    if (!objectPtr) {
        return false;
    }

    int count = PanelVTableCount();
    if (count < 0) {
        return false;  // Unsupported version; overriding disabled (already logged).
    }

    vtableOverride = new VTableOverride(objectPtr, this, count);
    if (!vtableOverride->IsActive()) {
        debugLog("[CSWGuiPanel] ERROR: failed to install vtable override\n");
        delete vtableOverride;
        vtableOverride = nullptr;
        return false;
    }
    return true;
}

void CSWGuiPanel::OverrideHandleInputEvent(void* handler) {
    if (!EnsureVTableOverride()) {
        return;
    }
    inputEventHandler = handler;
    vtableOverride->Override(static_cast<int>(PanelVTableSlot::HandleInputEvent),
                             reinterpret_cast<void*>(&CSWGuiPanel::HandleInputEventThunk));
}

void CSWGuiPanel::OverrideDraw(void* handler) {
    if (!EnsureVTableOverride()) {
        return;
    }
    drawHandler = handler;
    vtableOverride->Override(static_cast<int>(PanelVTableSlot::Draw),
                             reinterpret_cast<void*>(&CSWGuiPanel::DrawThunk));
}

void CSWGuiPanel::OverrideOnPanelAdded(void* handler) {
    if (!EnsureVTableOverride()) {
        return;
    }
    onPanelAddedHandler = handler;
    vtableOverride->Override(static_cast<int>(PanelVTableSlot::OnPanelAdded),
                             reinterpret_cast<void*>(&CSWGuiPanel::OnPanelAddedThunk));
}

void CSWGuiPanel::OverrideOnPanelRemoved(void* handler) {
    if (!EnsureVTableOverride()) {
        return;
    }
    onPanelRemovedHandler = handler;
    vtableOverride->Override(static_cast<int>(PanelVTableSlot::OnPanelRemoved),
                             reinterpret_cast<void*>(&CSWGuiPanel::OnPanelRemovedThunk));
}

void CSWGuiPanel::OverrideUpdate(void* handler) {
    if (!EnsureVTableOverride()) {
        return;
    }
    updateHandler = handler;
    vtableOverride->Override(static_cast<int>(PanelVTableSlot::Update),
                             reinterpret_cast<void*>(&CSWGuiPanel::UpdateThunk));
}

int CSWGuiPanel::PanelVTableCount() {
    // Only KotOR 1 on Windows is supported for now. Other versions/platforms
    // return -1 so callers disable vtable overriding rather than corrupting a
    // mismatched layout.
    if (GameVersion::GetTitle() == GameTitle::KOTOR1 &&
        GameVersion::GetPlatform() == GamePlatform::Windows) {
        return PANEL_VTABLE_SLOT_COUNT;
    }

    debugLog("[CSWGuiPanel] WARNING: panel vtable layout unknown for this game version; vtable overriding disabled\n");
    return -1;
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

CSWGuiPanel::CSWGuiPanel(CSWGuiManager* manager)
    : CSWGuiObject(nullptr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }

    if (classSize > 0 && constructor) {
        objectPtr = malloc(classSize);
        if (objectPtr) {
            constructor(objectPtr, manager ? manager->GetPtr() : nullptr);
            shouldFree = true;
        }
    }
}

CSWGuiPanel::~CSWGuiPanel()
{
    // Restore the game's original vtable (and free our copy) before the game's
    // destructor runs, so it dispatches against its own vtable.
    if (vtableOverride) {
        delete vtableOverride;
        vtableOverride = nullptr;
    }

    if (shouldFree && objectPtr) {
        if (destructor) {
            destructor(objectPtr);
        }
        free(objectPtr);
        objectPtr = nullptr;
        shouldFree = false;
    }
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
