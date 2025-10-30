#pragma once
#include <windows.h>

// Virtual Function Call (VFC) Template System

// Usage Example:
//   void* result = CallVirtualFunction<void*, int, float>(object, 0x30, argInt, argFloat);
template<typename RetType, typename... Args>
RetType callVirtualFunction(void* object, int vtableOffset, Args... args)
{
    if (!object) {
        return RetType();
    }

    // Dereference object to get vtable pointer
    void** vtable = *(void***)object;

    // Get function pointer at vtable offset
    auto func = reinterpret_cast<RetType(__thiscall*)(void*, Args...)>(vtable[vtableOffset]);

    // Call with this pointer and arguments
    return func(object, args...);
}

template<typename... Args>
void callVirtualFunctionVoid(void* object, int vtableOffset, Args... args)
{
    if (!object) {
        return;
    }

    // Dereference object to get vtable pointer
    void** vtable = *(void***)object;

    // Get function pointer at vtable offset
    auto func = reinterpret_cast<void(__thiscall*)(void*, Args...)>(vtable[vtableOffset]);

    // Call with this pointer and arguments
    func(object, args...);
}
