#pragma once
#include "../Common.h"
#include "CSWGuiText.h"

class CExoString;

class CSWGuiEditText : public CSWGuiText {
public:
    explicit CSWGuiEditText(void* objectPtr);
    CSWGuiEditText();
    ~CSWGuiEditText();

    // Accessors. Returned wrapper is heap allocated; caller owns it.
    CExoString* GetString();
    short GetMaxLength();
    void SetMaxLength(short maxLength);

    // Functions
    void AddNewChar(char* character);
    void RemoveLastChar();
    void SetCaretVisible(int visible);
    void SetText(CExoString* text);

    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    typedef void (__thiscall* AddNewCharFn)(void* thisPtr, char* character);
    typedef void (__thiscall* RemoveLastCharFn)(void* thisPtr);
    typedef void (__thiscall* SetCaretVisibleFn)(void* thisPtr, int visible);
    typedef void (__thiscall* SetTextFn)(void* thisPtr, void* text);
    typedef void* (__thiscall* ConstructorFn)(void* thisPtr);
    typedef void* (__thiscall* DestructorFn)(void* thisPtr);

    static AddNewCharFn addNewChar;
    static RemoveLastCharFn removeLastChar;
    static SetCaretVisibleFn setCaretVisible;
    static SetTextFn setText;
    static ConstructorFn constructor;
    static DestructorFn  destructor;
    static int classSize;

    static bool functionsInitialized;
    static bool offsetsInitialized;

    static int offsetMaxLength;
    static int offsetString;
};
