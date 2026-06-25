#include "CSWGuiProgressBar.h"
#include "CSWGuiBorder.h"
#include "GameVersion.h"

CSWGuiProgressBar::ConstructorFn CSWGuiProgressBar::constructor = nullptr;
CSWGuiProgressBar::DestructorFn  CSWGuiProgressBar::destructor  = nullptr;
CSWGuiProgressBar::SetCurValueFn      CSWGuiProgressBar::setCurValue      = nullptr;
CSWGuiProgressBar::SetMaxValueFn      CSWGuiProgressBar::setMaxValue      = nullptr;
CSWGuiProgressBar::SetStartFromLeftFn CSWGuiProgressBar::setStartFromLeft = nullptr;
int CSWGuiProgressBar::classSize = -1;
int CSWGuiProgressBar::offsetMax     = -1;
int CSWGuiProgressBar::offsetBorder1 = -1;
int CSWGuiProgressBar::offsetBorder2 = -1;

bool CSWGuiProgressBar::functionsInitialized = false;
bool CSWGuiProgressBar::offsetsInitialized = false;

void CSWGuiProgressBar::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    CSWGuiControl::InitializeFunctions();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiProgressBar] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Functions Here
        constructor = reinterpret_cast<ConstructorFn>(GameVersion::GetFunctionAddress("CSWGuiProgressBar", "Constructor"));
        destructor  = reinterpret_cast<DestructorFn> (GameVersion::GetFunctionAddress("CSWGuiProgressBar", "Destructor_2"));
        setCurValue      = reinterpret_cast<SetCurValueFn>     (GameVersion::GetFunctionAddress("CSWGuiProgressBar", "SetCurValue"));
        setMaxValue      = reinterpret_cast<SetMaxValueFn>     (GameVersion::GetFunctionAddress("CSWGuiProgressBar", "SetMaxValue"));
        setStartFromLeft = reinterpret_cast<SetStartFromLeftFn>(GameVersion::GetFunctionAddress("CSWGuiProgressBar", "SetStartFromLeft"));

        functionsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiProgressBar] ERROR: %s\n", e.what());
        return;
    }
}

void CSWGuiProgressBar::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    CSWGuiControl::InitializeOffsets();

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CSWGuiProgressBar] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        // Offsets Here
        offsetMax     = GameVersion::GetOffset("CSWGuiProgressBar", "max");
        offsetBorder1 = GameVersion::GetOffset("CSWGuiProgressBar", "border_1");
        offsetBorder2 = GameVersion::GetOffset("CSWGuiProgressBar", "border_2");
        classSize = GameVersion::GetClassSize("CSWGuiProgressBar");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CSWGuiProgressBar] ERROR: %s\n", e.what());
    }
}

CSWGuiProgressBar::CSWGuiProgressBar(void* objectPtr)
    : CSWGuiControl(objectPtr)
{
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

CSWGuiProgressBar::CSWGuiProgressBar()
    : CSWGuiControl(nullptr)
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

CSWGuiProgressBar::~CSWGuiProgressBar()
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

int CSWGuiProgressBar::GetMax() {
    if (!objectPtr || offsetMax < 0) {
        return 0;
    }
    return getObjectProperty<int>(objectPtr, offsetMax);
}

CSWGuiBorder* CSWGuiProgressBar::GetBorder1() {
    if (!objectPtr || offsetBorder1 < 0) {
        return nullptr;
    }
    // Inline CSWGuiBorder member: wrap its in-place address.
    return new CSWGuiBorder((char*)objectPtr + offsetBorder1);
}

CSWGuiBorder* CSWGuiProgressBar::GetBorder2() {
    if (!objectPtr || offsetBorder2 < 0) {
        return nullptr;
    }
    // Inline CSWGuiBorder member: wrap its in-place address.
    return new CSWGuiBorder((char*)objectPtr + offsetBorder2);
}

void CSWGuiProgressBar::SetCurValue(int value) {
    if (!objectPtr || !setCurValue) return;
    setCurValue(objectPtr, value);
}

void CSWGuiProgressBar::SetMaxValue(int value) {
    if (!objectPtr || !setMaxValue) return;
    setMaxValue(objectPtr, value);
}

void CSWGuiProgressBar::SetStartFromLeft(UINT startFromLeft) {
    if (!objectPtr || !setStartFromLeft) return;
    setStartFromLeft(objectPtr, startFromLeft);
}
