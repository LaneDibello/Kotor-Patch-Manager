#include "CSWGuiListBox.h"
#include "GameVersion.h"
#include "CSWGuiBorder.h"
#include "CSWGuiScrollBar.h"
#include "CSWGuiControl.h"
#include "CExoArrayList.h"

CSWGuiListBox::ConstructorFn CSWGuiListBox::constructor = nullptr;
CSWGuiListBox::DestructorFn  CSWGuiListBox::destructor  = nullptr;
int CSWGuiListBox::classSize = -1;

CSWGuiListBox::AddControlsFn        CSWGuiListBox::addControls        = nullptr;
CSWGuiListBox::AddControls2Fn       CSWGuiListBox::addControls2       = nullptr;
CSWGuiListBox::ClearItemsFn         CSWGuiListBox::clearItems         = nullptr;
CSWGuiListBox::DisplayToolTipFn     CSWGuiListBox::displayToolTip     = nullptr;
CSWGuiListBox::GetControlFn         CSWGuiListBox::getControl         = nullptr;
CSWGuiListBox::GetIsSelectableFn    CSWGuiListBox::getIsSelectable    = nullptr;
CSWGuiListBox::GetSelectedControlFn CSWGuiListBox::getSelectedControl = nullptr;
CSWGuiListBox::OrganizeControlsFn   CSWGuiListBox::organizeControls   = nullptr;
CSWGuiListBox::OrganizeOversizedFn  CSWGuiListBox::organizeOversized  = nullptr;
CSWGuiListBox::OrganizeUnequalFn    CSWGuiListBox::organizeUnequal    = nullptr;
CSWGuiListBox::ReSetFontFn          CSWGuiListBox::reSetFont          = nullptr;
CSWGuiListBox::SetActiveFn          CSWGuiListBox::setActive          = nullptr;
CSWGuiListBox::SetActiveControlFn   CSWGuiListBox::setActiveControl   = nullptr;
CSWGuiListBox::SetExtentFn          CSWGuiListBox::setExtent          = nullptr;
CSWGuiListBox::SetPaddingFn         CSWGuiListBox::setPadding         = nullptr;
CSWGuiListBox::SetScrollBarExtentFn CSWGuiListBox::setScrollBarExtent = nullptr;
CSWGuiListBox::SetScrollBarOnLeftFn CSWGuiListBox::setScrollBarOnLeft = nullptr;
CSWGuiListBox::SetSelectedControlFn CSWGuiListBox::setSelectedControl = nullptr;
CSWGuiListBox::SetTopVisibleFn      CSWGuiListBox::setTopVisible      = nullptr;
CSWGuiListBox::ShouldScrollFn       CSWGuiListBox::shouldScrollFn     = nullptr;
CSWGuiListBox::GetPagesToScrollFn   CSWGuiListBox::getPagesToScrollFn = nullptr;

bool CSWGuiListBox::functionsInitialized = false;
bool CSWGuiListBox::offsetsInitialized = false;

int CSWGuiListBox::offsetBorder = -1;
int CSWGuiListBox::offsetScrollbar = -1;
int CSWGuiListBox::offsetControls = -1;
int CSWGuiListBox::offsetControlExtents = -1;
int CSWGuiListBox::offsetHoveredControl = -1;
int CSWGuiListBox::offsetProtoItem = -1;
int CSWGuiListBox::offsetColor = -1;
int CSWGuiListBox::offsetViewportX = -1;
int CSWGuiListBox::offsetViewportY = -1;
int CSWGuiListBox::offsetViewportWidth = -1;
int CSWGuiListBox::offsetViewportHeight = -1;
int CSWGuiListBox::offsetPadding = -1;

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
        constructor = reinterpret_cast<ConstructorFn>(GameVersion::GetFunctionAddress("CSWGuiListBox", "Constructor"));
        destructor  = reinterpret_cast<DestructorFn> (GameVersion::GetFunctionAddress("CSWGuiListBox", "Destructor"));

        addControls        = reinterpret_cast<AddControlsFn>       (GameVersion::GetFunctionAddress("CSWGuiListBox", "AddControls"));
        addControls2       = reinterpret_cast<AddControls2Fn>      (GameVersion::GetFunctionAddress("CSWGuiListBox", "AddControls_2"));
        clearItems         = reinterpret_cast<ClearItemsFn>        (GameVersion::GetFunctionAddress("CSWGuiListBox", "ClearItems"));
        displayToolTip     = reinterpret_cast<DisplayToolTipFn>    (GameVersion::GetFunctionAddress("CSWGuiListBox", "DisplayToolTip"));
        getControl         = reinterpret_cast<GetControlFn>        (GameVersion::GetFunctionAddress("CSWGuiListBox", "GetControl"));
        getIsSelectable    = reinterpret_cast<GetIsSelectableFn>   (GameVersion::GetFunctionAddress("CSWGuiListBox", "GetIsSelectable"));
        getSelectedControl = reinterpret_cast<GetSelectedControlFn>(GameVersion::GetFunctionAddress("CSWGuiListBox", "GetSelectedControl"));
        organizeControls   = reinterpret_cast<OrganizeControlsFn>  (GameVersion::GetFunctionAddress("CSWGuiListBox", "OrganizeControls"));
        organizeOversized  = reinterpret_cast<OrganizeOversizedFn> (GameVersion::GetFunctionAddress("CSWGuiListBox", "OrganizeOversized"));
        organizeUnequal    = reinterpret_cast<OrganizeUnequalFn>   (GameVersion::GetFunctionAddress("CSWGuiListBox", "OrganizeUnequal"));
        reSetFont          = reinterpret_cast<ReSetFontFn>         (GameVersion::GetFunctionAddress("CSWGuiListBox", "ReSetFont"));
        setActive          = reinterpret_cast<SetActiveFn>         (GameVersion::GetFunctionAddress("CSWGuiListBox", "SetActive"));
        setActiveControl   = reinterpret_cast<SetActiveControlFn>  (GameVersion::GetFunctionAddress("CSWGuiListBox", "SetActiveControl"));
        setExtent          = reinterpret_cast<SetExtentFn>         (GameVersion::GetFunctionAddress("CSWGuiListBox", "SetExtent"));
        setPadding         = reinterpret_cast<SetPaddingFn>        (GameVersion::GetFunctionAddress("CSWGuiListBox", "SetPadding"));
        setScrollBarExtent = reinterpret_cast<SetScrollBarExtentFn>(GameVersion::GetFunctionAddress("CSWGuiListBox", "SetScrollBarExtent"));
        setScrollBarOnLeft = reinterpret_cast<SetScrollBarOnLeftFn>(GameVersion::GetFunctionAddress("CSWGuiListBox", "SetScrollBarOnLeft"));
        setSelectedControl = reinterpret_cast<SetSelectedControlFn>(GameVersion::GetFunctionAddress("CSWGuiListBox", "SetSelectedControl"));
        setTopVisible      = reinterpret_cast<SetTopVisibleFn>     (GameVersion::GetFunctionAddress("CSWGuiListBox", "SetTopVisible"));
        shouldScrollFn     = reinterpret_cast<ShouldScrollFn>      (GameVersion::GetFunctionAddress("CSWGuiListBox", "shouldScroll"));
        getPagesToScrollFn = reinterpret_cast<GetPagesToScrollFn>  (GameVersion::GetFunctionAddress("CSWGuiListBox", "getPagesToScroll"));

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
        offsetProtoItem      = GameVersion::GetOffset("CSWGuiListBox", "proto_item");
        offsetColor          = GameVersion::GetOffset("CSWGuiListBox", "color");
        offsetViewportX      = GameVersion::GetOffset("CSWGuiListBox", "viewport_x");
        offsetViewportY      = GameVersion::GetOffset("CSWGuiListBox", "viewport_y");
        offsetViewportWidth  = GameVersion::GetOffset("CSWGuiListBox", "viewport_width");
        offsetViewportHeight = GameVersion::GetOffset("CSWGuiListBox", "viewport_height");
        offsetPadding        = GameVersion::GetOffset("CSWGuiListBox", "padding");
        classSize = GameVersion::GetClassSize("CSWGuiListBox");

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

CSWGuiListBox::CSWGuiListBox()
    : CSWGuiNavigable(nullptr)
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
            constructor(objectPtr);
            shouldFree = true;
        }
    }
}

CSWGuiListBox::~CSWGuiListBox()
{
    if (shouldFree && objectPtr) {
        if (destructor) {
            destructor(objectPtr);
        }
        free(objectPtr);
        objectPtr = nullptr;
        shouldFree = false;
    }
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

CSWGuiControl* CSWGuiListBox::GetProtoItem() {
    if (!objectPtr || offsetProtoItem < 0) {
        return nullptr;
    }
    void* protoPtr = getObjectProperty<void*>(objectPtr, offsetProtoItem);
    if (!protoPtr) {
        return nullptr;
    }
    return new CSWGuiControl(protoPtr);
}

int CSWGuiListBox::GetViewportX() {
    if (!objectPtr || offsetViewportX < 0) return 0;
    return getObjectProperty<int>(objectPtr, offsetViewportX);
}

int CSWGuiListBox::GetViewportY() {
    if (!objectPtr || offsetViewportY < 0) return 0;
    return getObjectProperty<int>(objectPtr, offsetViewportY);
}

int CSWGuiListBox::GetViewportWidth() {
    if (!objectPtr || offsetViewportWidth < 0) return 0;
    return getObjectProperty<int>(objectPtr, offsetViewportWidth);
}

int CSWGuiListBox::GetViewportHeight() {
    if (!objectPtr || offsetViewportHeight < 0) return 0;
    return getObjectProperty<int>(objectPtr, offsetViewportHeight);
}

int CSWGuiListBox::GetPadding() {
    if (!objectPtr || offsetPadding < 0) return 0;
    return getObjectProperty<unsigned char>(objectPtr, offsetPadding); // padding is a byte
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

void CSWGuiListBox::AddControls(CExoArrayList<CSWGuiControl*>* controls, int suppressCleanUp, int restoreExtents, int varyItemHeights) {
    if (!objectPtr || !addControls) return;
    addControls(objectPtr, controls ? controls->GetPtr() : nullptr, suppressCleanUp, restoreExtents, varyItemHeights);
}

void CSWGuiListBox::AddControls(CSWGuiControl** controls, int count, int suppressCleanUp, int restoreExtents, int variableItemHeights) {
    if (!objectPtr || !addControls2) return;
    addControls2(objectPtr, controls, count, suppressCleanUp, restoreExtents, variableItemHeights);
}

void CSWGuiListBox::ClearItems() {
    if (!objectPtr || !clearItems) return;
    clearItems(objectPtr);
}

int CSWGuiListBox::DisplayToolTip() {
    if (!objectPtr || !displayToolTip) return 0;
    return displayToolTip(objectPtr);
}

CSWGuiControl* CSWGuiListBox::GetControl(int controlId) {
    if (!objectPtr || !getControl) return nullptr;
    void* ctrlPtr = getControl(objectPtr, controlId);
    if (!ctrlPtr) return nullptr;
    return new CSWGuiControl(ctrlPtr);
}

int CSWGuiListBox::GetIsSelectable() {
    if (!objectPtr || !getIsSelectable) return 0;
    return getIsSelectable(objectPtr);
}

CSWGuiControl* CSWGuiListBox::GetSelectedControl() {
    if (!objectPtr || !getSelectedControl) return nullptr;
    void* ctrlPtr = getSelectedControl(objectPtr);
    if (!ctrlPtr) return nullptr;
    return new CSWGuiControl(ctrlPtr);
}

void CSWGuiListBox::OrganizeControls() {
    if (!objectPtr || !organizeControls) return;
    organizeControls(objectPtr);
}

void CSWGuiListBox::OrganizeOversized() {
    if (!objectPtr || !organizeOversized) return;
    organizeOversized(objectPtr);
}

void CSWGuiListBox::OrganizeUnequal() {
    if (!objectPtr || !organizeUnequal) return;
    organizeUnequal(objectPtr);
}

void CSWGuiListBox::ReSetFont() {
    if (!objectPtr || !reSetFont) return;
    reSetFont(objectPtr);
}

void CSWGuiListBox::SetActive(int active) {
    if (!objectPtr || !setActive) return;
    setActive(objectPtr, active);
}

void CSWGuiListBox::SetActiveControl(CSWGuiControl* control, int active) {
    if (!objectPtr || !setActiveControl) return;
    setActiveControl(objectPtr, control ? control->GetPtr() : nullptr, active);
}

void CSWGuiListBox::SetExtent(CSWGuiExtent* extent) {
    if (!objectPtr || !setExtent) return;
    setExtent(objectPtr, extent);
}

void CSWGuiListBox::SetPadding(int padding) {
    if (!objectPtr || !setPadding) return;
    setPadding(objectPtr, padding);
}

void CSWGuiListBox::SetScrollBarExtent(int width, int updateListBoxExtent) {
    if (!objectPtr || !setScrollBarExtent) return;
    setScrollBarExtent(objectPtr, width, updateListBoxExtent);
}

void CSWGuiListBox::SetScrollBarOnLeft(int onLeft) {
    if (!objectPtr || !setScrollBarOnLeft) return;
    setScrollBarOnLeft(objectPtr, onLeft);
}

void CSWGuiListBox::SetSelectedControl(int controlId, int playSound) {
    if (!objectPtr || !setSelectedControl) return;
    setSelectedControl(objectPtr, controlId, playSound);
}

void CSWGuiListBox::SetTopVisible(short newTopIndex) {
    if (!objectPtr || !setTopVisible) return;
    setTopVisible(objectPtr, newTopIndex);
}

int CSWGuiListBox::shouldScroll() {
    if (!objectPtr || !shouldScrollFn) return 0;
    return shouldScrollFn(objectPtr);
}

int CSWGuiListBox::getPagesToScroll() {
    if (!objectPtr || !getPagesToScrollFn) return 0;
    return getPagesToScrollFn(objectPtr);
}
