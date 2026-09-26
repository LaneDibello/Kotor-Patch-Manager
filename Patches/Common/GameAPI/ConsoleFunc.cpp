#include "ConsoleFunc.h"
#include "GameVersion.h"

ConsoleFunc::Constructor ConsoleFunc::noParamConstructor = nullptr;
ConsoleFunc::Constructor ConsoleFunc::intConstructor = nullptr;
ConsoleFunc::Constructor ConsoleFunc::stringConstructor = nullptr;
ConsoleFunc::Destructor ConsoleFunc::destructor = nullptr;

bool ConsoleFunc::functionsInitialized = false;
bool ConsoleFunc::offsetsInitialized = false;

int ConsoleFunc::offsetName = -1;
int ConsoleFunc::offsetFuncHolder = -1;

void ConsoleFunc::InitializeFunctions() {
    if (functionsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[ConsoleFunc] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        noParamConstructor = reinterpret_cast<Constructor>(
            GameVersion::GetFunctionAddress("ConsoleFunc", "NoParamConstructor")
            );
        intConstructor = reinterpret_cast<Constructor>(
            GameVersion::GetFunctionAddress("ConsoleFunc", "IntConstructor")
            );
        stringConstructor = reinterpret_cast<Constructor>(
            GameVersion::GetFunctionAddress("ConsoleFunc", "StringConstructor")
            );
        destructor = reinterpret_cast<Destructor>(
            GameVersion::GetFunctionAddress("ConsoleFunc", "Destructor")
            );
    }
    catch (const GameVersionException& e) {
        debugLog("[ConsoleFunc] ERROR: %s", e.what());
        return;
    }

    functionsInitialized = true;
}

void ConsoleFunc::InitializeOffsets() {
    if (offsetsInitialized) {
        return;
    }

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[ConsoleFunc] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        offsetName = GameVersion::GetOffset("ConsoleFunc", "name");
        offsetFuncHolder = GameVersion::GetOffset("ConsoleFunc", "funcholder");

        offsetsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[ConsoleFunc] ERROR: %s", e.what());
    }
}

ConsoleFunc::ConsoleFunc(void* ptr)
    : GameAPIObject(ptr, false) {  // false = don't free (wrapping existing)

    if (!functionsInitialized) {
        InitializeFunctions();
    }

    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

ConsoleFunc::ConsoleFunc(const char* name, void* function, funcTypes type)
    : GameAPIObject(nullptr, true) {  // true = will free (allocating new)

    if (!functionsInitialized) {
        InitializeFunctions();
    }

    if (!offsetsInitialized) {
        InitializeOffsets();
    }

    // The classes table carries no size for ConsoleFunc, so the local struct
    // defines the layout the game constructors expect.
    Constructor construct = nullptr;
    switch (type) {
    case INT_PARAM:
        construct = intConstructor;
        break;
    case STRING_PARAM:
        construct = stringConstructor;
        break;
    default:
    case NO_PARAMS:
        construct = noParamConstructor;
        break;
    }

    if (!construct) {
        return;
    }

    objectPtr = malloc(sizeof(ConsoleFunc_struct));
    if (!objectPtr) {
        return;
    }

    debugLog("[ConsoleFunc] Constructing %s at %p, function %p, type %i", name, objectPtr, function, type);

    construct(objectPtr, const_cast<char*>(name), function);
}

ConsoleFunc::~ConsoleFunc() {
    if (shouldFree && objectPtr) {
        if (destructor) {
            destructor(objectPtr);
        }
        free(objectPtr);
    }
    // Base class destructor handles setting objectPtr to nullptr
}
