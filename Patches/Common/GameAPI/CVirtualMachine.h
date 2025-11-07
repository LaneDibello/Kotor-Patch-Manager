#pragma once

#include "GameVersion.h"
#include "../Common.h"

/// <summary>
/// Wrapper class for CVirtualMachine game object
///
/// Provides type-safe access to the game's virtual machine, which handles NWScript
/// execution and stack operations. This wrapper abstracts version-specific function
/// addresses and provides a clean API for stack manipulation.
///
/// Usage:
///   CVirtualMachine* vm = CVirtualMachine::GetInstance();
///   if (vm) {
///       int value = 0;
///       vm->StackPopInteger(&value);
///       // ... use value
///       delete vm;
///   }
/// </summary>
class CVirtualMachine {
public:
    /// <summary>
    /// Constructor - wraps an existing VM object pointer
    /// </summary>
    /// <param name="vmPtr">Pointer to game's CVirtualMachine object</param>
    explicit CVirtualMachine(void* vmPtr);

    /// <summary>
    /// Destructor - cleans up wrapper (does NOT delete game object)
    /// </summary>
    ~CVirtualMachine();

    /// <summary>
    /// Get the singleton virtual machine instance
    /// </summary>
    /// <returns>Wrapper around global VM instance, or nullptr if unavailable</returns>
    static CVirtualMachine* GetInstance();

    // ========================================================================
    // Stack Pop Operations (read from NWScript stack)
    // ========================================================================

    /// <summary>Pop integer value from VM stack</summary>
    /// <returns>true if successful, false if function unavailable or failed</returns>
    bool StackPopInteger(int* output);

    /// <summary>Pop float value from VM stack</summary>
    /// <returns>true if successful, false if function unavailable or failed</returns>
    bool StackPopFloat(float* output);

    /// <summary>Pop vector (3D position) from VM stack</summary>
    /// <returns>true if successful, false if function unavailable or failed</returns>
    bool StackPopVector(Vector* output);

    /// <summary>Pop string value from VM stack</summary>
    /// <returns>true if successful, false if function unavailable or failed</returns>
    bool StackPopString(CExoString* output);

    /// <summary>Pop engine structure (effect, location, talent, etc.) from VM stack</summary>
    /// <returns>true if successful, false if function unavailable or failed</returns>
    bool StackPopEngineStructure(VirtualMachineEngineStructureTypes type, void** output);

    /// <summary>Pop object ID from VM stack</summary>
    /// <returns>true if successful, false if function unavailable or failed</returns>
    bool StackPopObject(DWORD* output);

    /// <summary>Pop command (action) from VM stack</summary>
    /// <returns>true if successful, false if function unavailable or failed</returns>
    bool StackPopCommand(void** output);

    // ========================================================================
    // Stack Push Operations (write to NWScript stack)
    // ========================================================================

    /// <summary>Push integer value to VM stack</summary>
    /// <returns>true if successful, false if function unavailable or failed</returns>
    bool StackPushInteger(int value);

    /// <summary>Push float value to VM stack</summary>
    /// <returns>true if successful, false if function unavailable or failed</returns>
    bool StackPushFloat(float value);

    /// <summary>Push vector (3D position) to VM stack</summary>
    /// <returns>true if successful, false if function unavailable or failed</returns>
    bool StackPushVector(Vector value);

    /// <summary>Push string value to VM stack</summary>
    /// <returns>true if successful, false if function unavailable or failed</returns>
    bool StackPushString(CExoString* value);

    /// <summary>Push engine structure (effect, location, talent, etc.) to VM stack</summary>
    /// <returns>true if successful, false if function unavailable or failed</returns>
    bool StackPushEngineStructure(VirtualMachineEngineStructureTypes type, void* value);

    /// <summary>Push object ID to VM stack</summary>
    /// <returns>true if successful, false if function unavailable or failed</returns>
    bool StackPushObject(DWORD value);

    // ========================================================================
    // Script Execution
    // ========================================================================

    /// <summary>
    /// Execute a NWScript script
    /// </summary>
    /// <param name="scriptName">Name of script to run</param>
    /// <param name="objectSelf">Object ID to use as OBJECT_SELF</param>
    /// <param name="usually1">Usually set to 1 (purpose unclear)</param>
    /// <returns>true if successful, false if function unavailable or failed</returns>
    bool RunScript(CExoString* scriptName, DWORD objectSelf, int usually1 = 1);

    // ========================================================================
    // Capability Checks
    // ========================================================================

    /// <summary>Check if StackPopInteger is available for this version</summary>
    bool CanStackPopInteger() const { return m_stackPopInt != nullptr; }

    /// <summary>Check if StackPopFloat is available for this version</summary>
    bool CanStackPopFloat() const { return m_stackPopFloat != nullptr; }

    /// <summary>Check if StackPopVector is available for this version</summary>
    bool CanStackPopVector() const { return m_stackPopVector != nullptr; }

    /// <summary>Check if StackPopString is available for this version</summary>
    bool CanStackPopString() const { return m_stackPopString != nullptr; }

    /// <summary>Check if StackPushInteger is available for this version</summary>
    bool CanStackPushInteger() const { return m_stackPushInt != nullptr; }

    /// <summary>Check if StackPushString is available for this version</summary>
    bool CanStackPushString() const { return m_stackPushString != nullptr; }

    /// <summary>Check if RunScript is available for this version</summary>
    bool CanRunScript() const { return m_runScript != nullptr; }

    /// <summary>Get raw VM pointer (for advanced usage)</summary>
    void* GetRawPointer() const { return m_vmPtr; }

private:
    // Raw game object pointer
    void* m_vmPtr;

    // Function pointer typedefs (matching game's calling convention)
    typedef int(__thiscall* StackPopIntFn)(void*, int*);
    typedef int(__thiscall* StackPopFloatFn)(void*, float*);
    typedef int(__thiscall* StackPopVectorFn)(void*, Vector*);
    typedef int(__thiscall* StackPopStringFn)(void*, CExoString*);
    typedef int(__thiscall* StackPopEngineStructureFn)(void*, VirtualMachineEngineStructureTypes, void**);
    typedef int(__thiscall* StackPopObjectFn)(void*, DWORD*);
    typedef int(__thiscall* StackPopCommandFn)(void*, void**);
    typedef int(__thiscall* StackPushIntFn)(void*, int);
    typedef int(__thiscall* StackPushFloatFn)(void*, float);
    typedef int(__thiscall* StackPushVectorFn)(void*, Vector);
    typedef int(__thiscall* StackPushStringFn)(void*, CExoString*);
    typedef int(__thiscall* StackPushEngineStructureFn)(void*, VirtualMachineEngineStructureTypes, void*);
    typedef int(__thiscall* StackPushObjectFn)(void*, DWORD);
    typedef int(__thiscall* RunScriptFn)(void*, CExoString*, DWORD, int);

    // Cached function pointers (loaded once per class, shared across instances)
    static StackPopIntFn m_stackPopInt;
    static StackPopFloatFn m_stackPopFloat;
    static StackPopVectorFn m_stackPopVector;
    static StackPopStringFn m_stackPopString;
    static StackPopEngineStructureFn m_stackPopEngineStructure;
    static StackPopObjectFn m_stackPopObject;
    static StackPopCommandFn m_stackPopCommand;
    static StackPushIntFn m_stackPushInt;
    static StackPushFloatFn m_stackPushFloat;
    static StackPushVectorFn m_stackPushVector;
    static StackPushStringFn m_stackPushString;
    static StackPushEngineStructureFn m_stackPushEngineStructure;
    static StackPushObjectFn m_stackPushObject;
    static RunScriptFn m_runScript;

    // Initialization flag
    static bool m_functionsInitialized;

    /// <summary>
    /// Initialize function pointers from GameVersion (called once)
    /// </summary>
    static void InitializeFunctions();
};
