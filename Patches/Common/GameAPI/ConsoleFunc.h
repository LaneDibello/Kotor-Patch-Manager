#pragma once
#include "../Common.h"
#include "GameAPIObject.h"

enum funcTypes {
    NO_PARAMS,
    INT_PARAM,
    STRING_PARAM,
};

class ConsoleFunc : public GameAPIObject {
public:
    explicit ConsoleFunc(void* ptr);

    // Allocates and registers a new console command. The game keeps the pointer
    // in its command list, so a registered ConsoleFunc must outlive the session.
    ConsoleFunc(const char* name, void* function, funcTypes type);

    ~ConsoleFunc();

    // Override virtual methods from GameAPIObject
    void InitializeFunctions() override;
    void InitializeOffsets() override;

private:
    typedef void* (__thiscall* Constructor)(void* thisPtr, char* name, void* function);
    typedef void* (__thiscall* Destructor)(void* thisPtr);

    static Constructor noParamConstructor;
    static Constructor intConstructor;
    static Constructor stringConstructor;
    static Destructor destructor;

    static bool functionsInitialized;
    static bool offsetsInitialized;

    static int offsetName;
    static int offsetFuncHolder;
};

#pragma pack(push, 4)
struct ConsoleFunc_struct {
    char name[80];
    void* funcholder;
};
#pragma pack(pop)
