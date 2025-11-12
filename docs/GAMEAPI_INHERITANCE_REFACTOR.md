# GameAPI Inheritance & Organization Refactor

## Executive Summary

This document outlines a comprehensive refactoring plan for the GameAPI system to establish consistent inheritance patterns, improve code organization, and enable sustainable growth as we expand coverage of the game's class hierarchies.

### Current Issues

1. **Inconsistent Patterns**: Each GameAPI class implements similar functionality (pointer storage, initialization, GetPtr) independently with slight variations
2. **Code Duplication**: Constructor boilerplate, initialization checks, and GetPtr implementations repeated across 16+ classes
3. **Flat Directory Structure**: All 31 GameAPI files in a single directory, becoming difficult to navigate
4. **No Enforced Interface**: No compile-time guarantees that classes follow expected patterns
5. **Scalability Concerns**: As we add hundreds of GUI classes and additional CGameObject derivatives, inconsistencies will multiply

### Goals

1. **Establish Abstract Base Class**: Create `GameAPIObject` that all wrapper classes inherit from
2. **Consistent Interface**: Enforce standard pointer management, initialization, and memory handling patterns
3. **Logical Organization**: Reorganize into subdirectories by category (Objects, GUI, Managers, Types)
4. **Maintainability**: Make it easier for other developers to contribute new GameAPI classes
5. **Documentation**: Clearly document inheritance hierarchies that mirror the game engine's structure

### Scope

This is a **lightweight refactor** focused on:
- Adding one abstract base class
- Reorganizing directory structure (no logic changes)
- Updating include paths across patches
- Documenting patterns for future expansion

**Out of Scope**: Adding new GameAPI classes, changing hook system, modifying GameVersion

---

## Current State Analysis

### Directory Structure

**Location**: `/Patches/Common/GameAPI/`
**Organization**: Flat (no subdirectories)
**Total Files**: 31 files (16 headers + 15 implementation files)

**Complete File List**:
```
CAppManager.h/cpp
CClientExoApp.h/cpp
CClientOptions.h/cpp
CExoString.h/cpp
CGameObject.h/cpp
CGameObjectArray.h/cpp
CResRef.h/cpp
CServerExoApp.h/cpp
CSWCCreature.h/cpp
CSWGuiObject.h/cpp
CSWInventory.h/cpp
CSWItem.h/cpp
CSWSCreature.h/cpp
CSWSCreatureStats.h/cpp
CSWSObject.h/cpp
CVirtualMachine.h/cpp
GameVersion.h/cpp
```

### Existing Inheritance Hierarchy

Currently only one inheritance chain exists:

```
CGameObject
  └── CSWSObject
        └── CSWSCreature
```

**CGameObject** (Base class):
- Protected member: `void* objectPtr`
- Constructor: `CGameObject(void* objectPtr) : objectPtr(objectPtr)`
- Destructor: Sets `objectPtr = nullptr` (no deletion)
- Accessor: `void* GetPtr() const`

**CSWSObject** (Server-side object):
- Inherits from CGameObject
- Adds InitializeFunctions/InitializeOffsets pattern
- Calls parent initialization methods first

**CSWSCreature** (Creature object):
- Inherits from CSWSObject
- Continues initialization chain pattern

### Common Patterns Across Classes

#### Pattern 1: Pointer Storage
All classes store a pointer to game memory with slight naming variations:
```cpp
// CGameObject, CSWSObject, CSWSCreature, CSWGuiObject, etc.
void* objectPtr;

// CExoString
void* stringPtr;

// CResRef
void* ptr;
```

#### Pattern 2: Static Initialization System
Every class implements this boilerplate:
```cpp
private/protected:
    static bool functionsInitialized;
    static bool offsetsInitialized;

    static void InitializeFunctions();
    static void InitializeOffsets();
```

Constructor pattern:
```cpp
ClassName::ClassName(void* ptr) : BaseClass(ptr) {
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}
```

#### Pattern 3: Function Pointer Lookup
```cpp
private:
    typedef ReturnType (__thiscall* FunctionNameFn)(void* thisPtr, ...params);
    static FunctionNameFn functionName;

// In InitializeFunctions():
functionName = reinterpret_cast<FunctionNameFn>(
    GameVersion::GetFunctionAddress("ClassName", "FunctionName")
);
```

#### Pattern 4: Offset Lookup
```cpp
private/protected:
    static int offsetPropertyName;

// In InitializeOffsets():
offsetPropertyName = GameVersion::GetOffset("ClassName", "PropertyName");
```

#### Pattern 5: GetPtr() Accessor
All classes provide identical implementation:
```cpp
void* GetPtr() const {
    return objectPtr; // or stringPtr, or ptr...
}
```

#### Pattern 6: Memory Management (CExoString Pattern)
```cpp
private:
    bool shouldFree;

// Wrapping existing game memory
CExoString::CExoString(void* stringPtr)
    : stringPtr(stringPtr), shouldFree(false) {}

// Creating new object
CExoString::CExoString(const char* text)
    : shouldFree(true) {
    stringPtr = malloc(8);
    // Call game constructor...
}

// Destructor
CExoString::~CExoString() {
    if (shouldFree && stringPtr) {
        // Call game destructor and free memory
    }
}
```

### Current Class Categories

Based on usage patterns, classes fall into four categories:

**Category 1: Game Objects** (inherit from CGameObject)
- CGameObject (base)
- CSWSObject
- CSWSCreature
- CSWItem (future)
- CSWDoor (future)
- CSWPlaceable (future)

**Category 2: GUI Objects** (will inherit from CSWGuiObject)
- CSWGuiObject (skeleton implementation)
- Hundreds of future GUI classes (CSWGuiPanel, CSWGuiButton, etc.)

**Category 3: Managers** (singletons)
- CAppManager (GetInstance() pattern)
- CVirtualMachine (GetInstance() pattern)
- CClientExoApp
- CServerExoApp

**Category 4: Value/Data Types** (wrappers)
- CExoString (can create or wrap)
- CResRef (can create or wrap)
- CGameObjectArray (utility)
- CSWSCreatureStats (component wrapper)
- CSWInventory (component wrapper)
- CClientOptions (settings wrapper)

---

## Proposed Solution

### Abstract Base Class: GameAPIObject

Create a single abstract base class that all GameAPI classes inherit from. This enforces consistent patterns while allowing flexibility for different use cases.

#### Header: `GameAPI/Core/GameAPIObject.h`

```cpp
#pragma once

/// <summary>
/// Abstract base class for all GameAPI wrapper classes.
/// Provides consistent pointer management, initialization interface,
/// and memory handling for objects that wrap game engine memory.
/// </summary>
class GameAPIObject {
public:
    /// <summary>
    /// Constructs a GameAPIObject that wraps an existing game memory address.
    /// </summary>
    /// <param name="objectPtr">Pointer to game memory to wrap</param>
    /// <param name="shouldFree">If true, destructor will free the memory (default: false)</param>
    GameAPIObject(void* objectPtr, bool shouldFree = false);

    /// <summary>
    /// Virtual destructor. If shouldFree is true, derived classes should
    /// call appropriate game destructors before memory is freed.
    /// </summary>
    virtual ~GameAPIObject();

    /// <summary>
    /// Gets the pointer to the underlying game memory.
    /// </summary>
    /// <returns>Pointer to game object, or nullptr if invalid</returns>
    void* GetPtr() const;

    /// <summary>
    /// Checks if the wrapped pointer is valid (non-null).
    /// </summary>
    /// <returns>True if objectPtr is not nullptr</returns>
    bool IsValid() const;

    /// <summary>
    /// Pure virtual function that derived classes must implement to
    /// initialize static function pointers via GameVersion lookups.
    /// Called once per class on first instantiation.
    /// </summary>
    virtual void InitializeFunctions() = 0;

    /// <summary>
    /// Pure virtual function that derived classes must implement to
    /// initialize static offset values via GameVersion lookups.
    /// Called once per class on first instantiation.
    /// </summary>
    virtual void InitializeOffsets() = 0;

protected:
    /// <summary>
    /// Pointer to the game engine memory this object wraps.
    /// Protected so derived classes can access directly.
    /// </summary>
    void* objectPtr;

    /// <summary>
    /// Flag indicating whether this wrapper owns the memory and should
    /// free it in the destructor. True when we allocate game objects,
    /// false when wrapping existing game memory (the common case).
    /// </summary>
    bool shouldFree;

private:
    // Prevent copying (game objects should not be copied)
    GameAPIObject(const GameAPIObject&) = delete;
    GameAPIObject& operator=(const GameAPIObject&) = delete;
};
```

#### Implementation: `GameAPI/Core/GameAPIObject.cpp`

```cpp
#include "GameAPIObject.h"

GameAPIObject::GameAPIObject(void* objectPtr, bool shouldFree)
    : objectPtr(objectPtr), shouldFree(shouldFree) {
}

GameAPIObject::~GameAPIObject() {
    // Base destructor doesn't free memory - derived classes handle
    // calling game destructors if needed before this destructor runs
    objectPtr = nullptr;
}

void* GameAPIObject::GetPtr() const {
    return objectPtr;
}

bool GameAPIObject::IsValid() const {
    return objectPtr != nullptr;
}
```

### Key Design Decisions

#### 1. Single Base Class for All Categories

**Decision**: Use one `GameAPIObject` base class for all wrapper types (game objects, managers, values)

**Rationale**:
- All wrappers share the same fundamental pattern: wrapping a pointer to game memory
- Managers and value types can override pure virtuals with empty implementations if they don't need function/offset lookups
- Simpler than maintaining multiple base class hierarchies
- Easy to understand for new contributors

**Trade-offs**:
- Some classes (like CGameObjectArray utility) may have unused virtual methods
- Could add slight overhead, but these aren't performance-critical objects (created infrequently)

#### 2. Per-Class Static Initialization Flags

**Decision**: Keep existing pattern where each class maintains its own `functionsInitialized` and `offsetsInitialized` static bools

**Rationale**:
- Proven pattern that works well in current codebase
- Each class knows when its static members are ready
- No centralized registry complexity
- Inheritance chain already handles calling parent initialization first

**Pattern**:
```cpp
class CGameObject : public GameAPIObject {
private:
    static bool functionsInitialized;
    static bool offsetsInitialized;

public:
    void InitializeFunctions() override {
        if (functionsInitialized) return;
        // Initialize function pointers...
        functionsInitialized = true;
    }

    void InitializeOffsets() override {
        if (offsetsInitialized) return;
        // Initialize offsets...
        offsetsInitialized = true;
    }
};
```

Derived classes call parent first:
```cpp
void CSWSObject::InitializeFunctions() override {
    if (functionsInitialized) return;
    CGameObject::InitializeFunctions(); // Parent first
    // Initialize own functions...
    functionsInitialized = true;
}
```

#### 3. shouldFree as Base Class Feature

**Decision**: Include `shouldFree` flag and memory management in base class

**Rationale**:
- Generalizes the CExoString pattern to all wrappers
- Zero cost when false (the common case - wrapping existing game memory)
- Enables value types (CExoString, CResRef) to create new game objects safely
- Consistent destructor pattern across all classes

**Usage Examples**:
```cpp
// Common case: Wrapping existing game memory (shouldFree = false)
CGameObject* creature = new CGameObject(creaturePtr);
delete creature; // Safe, doesn't free game memory

// Creating new game object (shouldFree = true)
CExoString* str = new CExoString("Hello"); // Allocates, sets shouldFree=true
delete str; // Calls game destructor and frees memory

// Wrapping existing string (shouldFree = false)
CExoString* wrapped = new CExoString(existingStringPtr);
delete wrapped; // Does NOT free game memory
```

#### 4. Pure Virtual Initialize Methods

**Decision**: Make InitializeFunctions() and InitializeOffsets() pure virtual

**Rationale**:
- Forces derived classes to explicitly implement (even if empty)
- Documents intent: "this class needs function lookups" vs "this class doesn't"
- Compile-time enforcement of interface
- Derived classes can still choose to do nothing if appropriate

**Empty Implementation Pattern** (for classes without functions/offsets):
```cpp
void CGameObjectArray::InitializeFunctions() override {
    // No functions to initialize for this utility class
}

void CGameObjectArray::InitializeOffsets() override {
    // No offsets to initialize for this utility class
}
```

---

## Proposed Directory Structure

### New Organization

Reorganize GameAPI into logical subdirectories:

```
Patches/Common/GameAPI/
├── Core/
│   ├── GameAPIObject.h          (NEW - abstract base class)
│   ├── GameAPIObject.cpp        (NEW)
│   ├── GameVersion.h            (existing)
│   ├── GameVersion.cpp          (existing)
│   └── Common.h                 (existing - templates, utilities)
│
├── Objects/                     (NEW - CGameObject hierarchy)
│   ├── CGameObject.h
│   ├── CGameObject.cpp
│   ├── CGameObjectArray.h
│   ├── CGameObjectArray.cpp
│   ├── CSWSObject.h             (Server-side base)
│   ├── CSWSObject.cpp
│   ├── CSWSCreature.h
│   ├── CSWSCreature.cpp
│   ├── CSWCCreature.h           (Client-side creature)
│   ├── CSWCCreature.cpp
│   ├── CSWItem.h
│   ├── CSWItem.cpp
│   ├── CSWSCreatureStats.h      (Component object)
│   ├── CSWSCreatureStats.cpp
│   ├── CSWInventory.h           (Component object)
│   └── CSWInventory.cpp
│
├── GUI/                         (NEW - GUI object hierarchy)
│   ├── CSWGuiObject.h           (Base for all GUI classes)
│   └── CSWGuiObject.cpp
│   # Future: Hundreds of GUI classes go here
│   #   CSWGuiPanel, CSWGuiButton, CSWGuiLabel, etc.
│
├── Managers/                    (NEW - Singleton managers)
│   ├── CAppManager.h
│   ├── CAppManager.cpp
│   ├── CVirtualMachine.h
│   ├── CVirtualMachine.cpp
│   ├── CClientExoApp.h
│   ├── CClientExoApp.cpp
│   ├── CServerExoApp.h
│   └── CServerExoApp.cpp
│
└── Types/                       (NEW - Value/data wrappers)
    ├── CExoString.h
    ├── CExoString.cpp
    ├── CResRef.h
    ├── CResRef.cpp
    └── CClientOptions.h
    └── CClientOptions.cpp
```

### Rationale for Organization

**Core/**: Fundamental infrastructure used by all other classes
- GameAPIObject: Base class all wrappers inherit from
- GameVersion: Address/offset lookup system
- Common.h: Shared templates and utilities

**Objects/**: Game world objects (CGameObject hierarchy)
- Everything that represents an in-game entity
- Includes component objects (Stats, Inventory) that are part of game objects

**GUI/**: User interface objects (CSWGuiObject hierarchy)
- All GUI classes inherit from CSWGuiObject
- Separates UI from game logic
- Room for future expansion (hundreds of GUI classes)

**Managers/**: Singleton application managers
- Global systems (App, VM, ExoApp)
- Typically accessed via GetInstance()
- Distinct from game objects

**Types/**: Basic data types and wrappers
- String, resource references, options
- Can create new instances or wrap existing
- Used as parameters/return types throughout API

---

## Updated Inheritance Hierarchies

### Current Hierarchy (Implemented)

```
GameAPIObject (NEW abstract base)
  └── CGameObject
        └── CSWSObject (Server-side objects)
              └── CSWSCreature
```

### Planned Hierarchies

#### Game Object Hierarchy
```
GameAPIObject
  └── CGameObject (Base for all in-game entities)
        ├── CSWSObject (Server-side base)
        │     ├── CSWSCreature
        │     ├── CSWSDoor
        │     ├── CSWSPlaceable
        │     ├── CSWSItem
        │     ├── CSWSTrigger
        │     ├── CSWSAreaOfEffectObject
        │     ├── CSWSStore
        │     └── CSWSSound
        │
        ├── CSWCObject (Client-side base)
        │     └── CSWCCreature (client representation)
        │
        └── CNWSObject (Network-synced base)
              └── ... (various network object types)
```

**Key Characteristics**:
- All represent in-game entities
- Server-side (SWS) vs Client-side (SWC) variants
- Share common CGameObject functionality (ID, type, area)

#### GUI Object Hierarchy
```
GameAPIObject
  └── CSWGuiObject (Base for all GUI elements)
        ├── CSWGuiPanel
        │     ├── CSWGuiInventoryPanel
        │     ├── CSWGuiCharacterPanel
        │     └── CSWGuiDialogPanel
        │
        ├── CSWGuiButton
        │     ├── CSWGuiRadioButton
        │     └── CSWGuiCheckBox
        │
        ├── CSWGuiLabel
        ├── CSWGuiTextBox
        ├── CSWGuiListBox
        ├── CSWGuiScrollBar
        ├── CSWGuiSlider
        └── CSWGuiProgressBar
        # Plus hundreds more GUI classes...
```

**Key Characteristics**:
- All GUI elements inherit from CSWGuiObject
- Likely several hundred classes in the game
- Share common GUI functionality (extent, visibility, parent/child)

#### Manager Hierarchy
```
GameAPIObject
  ├── CAppManager (Global application manager)
  ├── CVirtualMachine (Script execution engine)
  ├── CClientExoApp (Client application)
  ├── CServerExoApp (Server application)
  ├── CExoResMan (Resource manager - future)
  ├── CTlkTable (Talk table manager - future)
  └── C2DACache (2DA file cache - future)
```

**Key Characteristics**:
- Singletons accessed via GetInstance()
- Global systems and subsystems
- No inheritance relationships between managers

#### Value Type Hierarchy
```
GameAPIObject
  ├── CExoString (Game's string type)
  ├── CResRef (16-char resource reference)
  ├── CExoLocString (Localized string - future)
  ├── Vector (3D vector - future)
  └── CClientOptions (Game settings)
```

**Key Characteristics**:
- Can wrap existing game memory OR create new instances
- Use shouldFree flag to manage lifetime
- Often passed by value or const reference in game API

---

## Migration Guide

### Phase 1: Create Base Class and Directory Structure

**Steps**:
1. Create `GameAPI/Core/` directory
2. Implement `GameAPIObject.h` and `GameAPIObject.cpp`
3. Move `GameVersion.h/cpp` to `Core/`
4. Move `Common.h` to `Core/`
5. Create remaining subdirectories: `Objects/`, `GUI/`, `Managers/`, `Types/`

### Phase 2: Update Existing Classes

For each existing GameAPI class:

1. **Move to appropriate subdirectory**
2. **Inherit from GameAPIObject**
3. **Update constructor to call base class**
4. **Make InitializeFunctions/InitializeOffsets override virtual methods**
5. **Remove redundant GetPtr() implementation** (inherited from base)

**Example**: Updating CGameObject

**Before** (`GameAPI/CGameObject.h`):
```cpp
#pragma once

class CGameObject {
public:
    CGameObject(void* objectPtr);
    ~CGameObject();

    void* GetPtr() const;

    // Game object methods...

protected:
    void* objectPtr;

    static bool functionsInitialized;
    static bool offsetsInitialized;
    static void InitializeFunctions();
    static void InitializeOffsets();
};
```

**After** (`GameAPI/Objects/CGameObject.h`):
```cpp
#pragma once
#include "../Core/GameAPIObject.h"

class CGameObject : public GameAPIObject {
public:
    CGameObject(void* objectPtr);
    ~CGameObject();

    // GetPtr() inherited from GameAPIObject

    // Game object methods...

    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    static bool functionsInitialized;
    static bool offsetsInitialized;
};
```

**Constructor Update** (`GameAPI/Objects/CGameObject.cpp`):
```cpp
#include "CGameObject.h"

// Before:
CGameObject::CGameObject(void* objectPtr)
    : objectPtr(objectPtr) {
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

// After:
CGameObject::CGameObject(void* objectPtr)
    : GameAPIObject(objectPtr, false) {  // false = don't free (wrapping existing)
    if (!functionsInitialized) {
        InitializeFunctions();
    }
    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

// Remove GetPtr() implementation - inherited from base
```

### Phase 3: Update Include Paths in Patches

**Direct Migration**: Update all `#include` statements throughout patch source code.

**Example Patch File**: `Patches/ScriptExtender/ScriptExtender.cpp`

**Before**:
```cpp
#include "Common.h"
#include "GameAPI/GameVersion.h"
#include "GameAPI/CVirtualMachine.h"
#include "GameAPI/CExoString.h"
#include "GameAPI/CGameObject.h"
```

**After**:
```cpp
#include "GameAPI/Core/Common.h"
#include "GameAPI/Core/GameVersion.h"
#include "GameAPI/Managers/CVirtualMachine.h"
#include "GameAPI/Types/CExoString.h"
#include "GameAPI/Objects/CGameObject.h"
```

**Affected Files**: All patch source files that include GameAPI headers. Based on codebase analysis:
- `Patches/ScriptExtender/ScriptExtender.cpp`
- `Patches/AdditionalConsoleCommands/AdditionalConsoleCommands.cpp`
- `Patches/Freecam/Freecam.cpp`
- Future patches as they're developed

**Search Pattern**: Use grep to find all includes: `grep -r '#include "GameAPI/' Patches/`

### Phase 4: Update Build System (If Needed)

Since patches include GameAPI files directly (header-only style), no changes to build scripts should be necessary. The compiler will follow the updated include paths.

**Verify**:
1. Build a patch that uses GameAPI after migration
2. Ensure all includes resolve correctly
3. Test that hooks still work at runtime

### Phase 5: Document Patterns for Future Classes

Add developer documentation (beyond this file):
1. **CONTRIBUTING.md**: Section on adding new GameAPI classes
2. **GameAPI/README.md**: Quick reference for directory structure
3. **Class Template**: Skeleton file developers can copy

---

## Code Examples

### Example 1: Refactoring CExoString (Value Type with Memory Management)

**Before** (`GameAPI/CExoString.h`):
```cpp
#pragma once

class CExoString {
public:
    // Wrap existing game string (don't free)
    CExoString(void* stringPtr);

    // Create new string (allocate and free)
    CExoString(const char* text);

    ~CExoString();

    void* GetPtr() const;
    const char* CStr() const;

private:
    void* stringPtr;
    bool shouldFree;

    static bool functionsInitialized;
    static void InitializeFunctions();
    static void InitializeOffsets();

    static int offsetText;
};
```

**After** (`GameAPI/Types/CExoString.h`):
```cpp
#pragma once
#include "../Core/GameAPIObject.h"

class CExoString : public GameAPIObject {
public:
    // Wrap existing game string (don't free)
    CExoString(void* stringPtr);

    // Create new string (allocate and free)
    CExoString(const char* text);

    ~CExoString() override;

    // GetPtr() inherited from GameAPIObject
    const char* CStr() const;

    void InitializeFunctions() override;
    void InitializeOffsets() override;

private:
    // objectPtr and shouldFree inherited from GameAPIObject

    static bool functionsInitialized;
    static bool offsetsInitialized;

    static int offsetText;
};
```

**Implementation Changes** (`GameAPI/Types/CExoString.cpp`):
```cpp
#include "CExoString.h"
#include "../Core/GameVersion.h"

bool CExoString::functionsInitialized = false;
bool CExoString::offsetsInitialized = false;
int CExoString::offsetText = 0;

// Wrapping constructor
CExoString::CExoString(void* stringPtr)
    : GameAPIObject(stringPtr, false) {  // Don't free - wrapping existing
    if (!functionsInitialized) InitializeFunctions();
    if (!offsetsInitialized) InitializeOffsets();
}

// Allocating constructor
CExoString::CExoString(const char* text)
    : GameAPIObject(nullptr, true) {  // Will free - we're allocating
    if (!functionsInitialized) InitializeFunctions();
    if (!offsetsInitialized) InitializeOffsets();

    // Allocate game string
    objectPtr = malloc(8);
    // Call game constructor...
}

CExoString::~CExoString() {
    if (shouldFree && objectPtr) {
        // Call game destructor
        typedef void (__thiscall* DestructorFn)(void*);
        static auto destructor = reinterpret_cast<DestructorFn>(
            GameVersion::GetFunctionAddress("CExoString", "Destructor")
        );
        if (destructor) {
            destructor(objectPtr);
        }
        free(objectPtr);
    }
    // Base class destructor runs after this
}

void CExoString::InitializeFunctions() {
    if (functionsInitialized) return;
    // Initialize function pointers...
    functionsInitialized = true;
}

void CExoString::InitializeOffsets() {
    if (offsetsInitialized) return;
    offsetText = GameVersion::GetOffset("CExoString", "Text");
    offsetsInitialized = true;
}

const char* CExoString::CStr() const {
    if (!objectPtr || offsetText == 0) return "";
    return getObjectProperty<const char*>(objectPtr, offsetText);
}
```

### Example 2: Refactoring CVirtualMachine (Manager/Singleton)

**Before** (`GameAPI/CVirtualMachine.h`):
```cpp
#pragma once

class CVirtualMachine {
public:
    static CVirtualMachine* GetInstance();

    void* GetPtr() const;
    int ExecuteScript(const char* scriptName, void* objectSelf);

private:
    CVirtualMachine(void* vmPtr);

    void* vmPtr;

    static bool functionsInitialized;
    static void InitializeFunctions();

    typedef int (__thiscall* ExecuteScriptFn)(void*, const char*, void*);
    static ExecuteScriptFn executeScript;
};
```

**After** (`GameAPI/Managers/CVirtualMachine.h`):
```cpp
#pragma once
#include "../Core/GameAPIObject.h"

class CVirtualMachine : public GameAPIObject {
public:
    static CVirtualMachine* GetInstance();

    // GetPtr() inherited from GameAPIObject
    int ExecuteScript(const char* scriptName, void* objectSelf);

    void InitializeFunctions() override;
    void InitializeOffsets() override;  // Empty for managers

private:
    CVirtualMachine(void* vmPtr);

    // objectPtr inherited from GameAPIObject

    static bool functionsInitialized;

    typedef int (__thiscall* ExecuteScriptFn)(void*, const char*, void*);
    static ExecuteScriptFn executeScript;
};
```

**Implementation** (`GameAPI/Managers/CVirtualMachine.cpp`):
```cpp
#include "CVirtualMachine.h"
#include "../Core/GameVersion.h"

bool CVirtualMachine::functionsInitialized = false;
CVirtualMachine::ExecuteScriptFn CVirtualMachine::executeScript = nullptr;

CVirtualMachine::CVirtualMachine(void* vmPtr)
    : GameAPIObject(vmPtr, false) {  // Never free - singleton
    if (!functionsInitialized) InitializeFunctions();
}

CVirtualMachine* CVirtualMachine::GetInstance() {
    static auto vmPtr = *reinterpret_cast<void**>(
        GameVersion::GetGlobalPointer("CVirtualMachine")
    );
    static CVirtualMachine instance(vmPtr);
    return &instance;
}

void CVirtualMachine::InitializeFunctions() {
    if (functionsInitialized) return;

    executeScript = reinterpret_cast<ExecuteScriptFn>(
        GameVersion::GetFunctionAddress("CVirtualMachine", "ExecuteScript")
    );

    functionsInitialized = true;
}

void CVirtualMachine::InitializeOffsets() {
    // No offsets needed for this manager
}

int CVirtualMachine::ExecuteScript(const char* scriptName, void* objectSelf) {
    if (!objectPtr || !executeScript) return -1;
    return executeScript(objectPtr, scriptName, objectSelf);
}
```

### Example 3: Refactoring CSWSObject (Inheritance Chain)

**Before** (`GameAPI/CSWSObject.h`):
```cpp
#pragma once
#include "CGameObject.h"

class CSWSObject : public CGameObject {
public:
    CSWSObject(void* objectPtr);

    // Server-side object methods...

protected:
    static bool functionsInitialized;
    static bool offsetsInitialized;
    static void InitializeFunctions();
    static void InitializeOffsets();
};
```

**After** (`GameAPI/Objects/CSWSObject.h`):
```cpp
#pragma once
#include "CGameObject.h"  // CGameObject is in same Objects/ directory

class CSWSObject : public CGameObject {
public:
    CSWSObject(void* objectPtr);

    // Server-side object methods...

    void InitializeFunctions() override;
    void InitializeOffsets() override;

protected:
    static bool functionsInitialized;
    static bool offsetsInitialized;
};
```

**Implementation** (`GameAPI/Objects/CSWSObject.cpp`):
```cpp
#include "CSWSObject.h"
#include "../Core/GameVersion.h"

bool CSWSObject::functionsInitialized = false;
bool CSWSObject::offsetsInitialized = false;

CSWSObject::CSWSObject(void* objectPtr)
    : CGameObject(objectPtr) {  // Call parent constructor
    if (!functionsInitialized) InitializeFunctions();
    if (!offsetsInitialized) InitializeOffsets();
}

void CSWSObject::InitializeFunctions() {
    if (functionsInitialized) return;

    // Initialize parent first
    CGameObject::InitializeFunctions();

    // Initialize own function pointers...

    functionsInitialized = true;
}

void CSWSObject::InitializeOffsets() {
    if (offsetsInitialized) return;

    // Initialize parent first
    CGameObject::InitializeOffsets();

    // Initialize own offsets...

    offsetsInitialized = true;
}
```

### Example 4: Adding a New GameAPI Class (Future Pattern)

When adding new GameAPI classes, follow this template:

**Header** (`GameAPI/Objects/CSWSDoor.h`):
```cpp
#pragma once
#include "CSWSObject.h"  // Parent class

/// <summary>
/// Represents a server-side door object in the game world.
/// </summary>
class CSWSDoor : public CSWSObject {
public:
    /// <summary>
    /// Wraps an existing door object in game memory.
    /// </summary>
    /// <param name="objectPtr">Pointer to the door object</param>
    CSWSDoor(void* objectPtr);

    // Door-specific methods
    bool IsOpen() const;
    void SetOpen(bool open);
    bool IsLocked() const;
    void SetLocked(bool locked);
    int GetLockDC() const;

    void InitializeFunctions() override;
    void InitializeOffsets() override;

private:
    static bool functionsInitialized;
    static bool offsetsInitialized;

    // Function pointers
    typedef void (__thiscall* SetOpenFn)(void* thisPtr, bool open);
    static SetOpenFn setOpenFn;

    // Offsets
    static int offsetIsOpen;
    static int offsetIsLocked;
    static int offsetLockDC;
};
```

**Implementation** (`GameAPI/Objects/CSWSDoor.cpp`):
```cpp
#include "CSWSDoor.h"
#include "../Core/GameVersion.h"
#include "../Core/Common.h"  // For getObjectProperty/setObjectProperty

// Static member initialization
bool CSWSDoor::functionsInitialized = false;
bool CSWSDoor::offsetsInitialized = false;
CSWSDoor::SetOpenFn CSWSDoor::setOpenFn = nullptr;
int CSWSDoor::offsetIsOpen = 0;
int CSWSDoor::offsetIsLocked = 0;
int CSWSDoor::offsetLockDC = 0;

CSWSDoor::CSWSDoor(void* objectPtr)
    : CSWSObject(objectPtr) {  // Call parent
    if (!functionsInitialized) InitializeFunctions();
    if (!offsetsInitialized) InitializeOffsets();
}

void CSWSDoor::InitializeFunctions() {
    if (functionsInitialized) return;

    // Initialize parent first
    CSWSObject::InitializeFunctions();

    // Initialize door-specific functions
    setOpenFn = reinterpret_cast<SetOpenFn>(
        GameVersion::GetFunctionAddress("CSWSDoor", "SetOpen")
    );

    functionsInitialized = true;
}

void CSWSDoor::InitializeOffsets() {
    if (offsetsInitialized) return;

    // Initialize parent first
    CSWSObject::InitializeOffsets();

    // Initialize door-specific offsets
    offsetIsOpen = GameVersion::GetOffset("CSWSDoor", "IsOpen");
    offsetIsLocked = GameVersion::GetOffset("CSWSDoor", "IsLocked");
    offsetLockDC = GameVersion::GetOffset("CSWSDoor", "LockDC");

    offsetsInitialized = true;
}

bool CSWSDoor::IsOpen() const {
    if (!IsValid() || offsetIsOpen == 0) return false;
    return getObjectProperty<bool>(objectPtr, offsetIsOpen);
}

void CSWSDoor::SetOpen(bool open) {
    if (!IsValid() || !setOpenFn) return;
    setOpenFn(objectPtr, open);
}

bool CSWSDoor::IsLocked() const {
    if (!IsValid() || offsetIsLocked == 0) return false;
    return getObjectProperty<bool>(objectPtr, offsetIsLocked);
}

void CSWSDoor::SetLocked(bool locked) {
    if (!IsValid() || offsetIsLocked == 0) return;
    setObjectProperty(objectPtr, offsetIsLocked, locked);
}

int CSWSDoor::GetLockDC() const {
    if (!IsValid() || offsetLockDC == 0) return 0;
    return getObjectProperty<int>(objectPtr, offsetLockDC);
}
```

**Add to AddressDatabase** (`AddressDatabases/kotor1_gog_103.toml`):
```toml
[functions.CSWSDoor]
SetOpen = "0x004a7b20"

[offsets.CSWSDoor]
IsOpen = 128
IsLocked = 132
LockDC = 136
```

---

## Future Expansions

### GUI Hierarchy (High Priority)

**Scope**: Hundreds of GUI classes in the game engine

**Base Class**: CSWGuiObject (skeleton already exists)

**Common Functionality**:
- Extent (x, y, width, height)
- Visibility
- Parent/child relationships
- Event handling

**Example Classes to Implement**:
```cpp
CSWGuiPanel : public CSWGuiObject
CSWGuiInventoryPanel : public CSWGuiPanel
CSWGuiCharacterPanel : public CSWGuiPanel
CSWGuiButton : public CSWGuiObject
CSWGuiLabel : public CSWGuiObject
CSWGuiTextBox : public CSWGuiObject
CSWGuiListBox : public CSWGuiObject
// Potentially 200+ more...
```

**Benefits**:
- Consistent API for GUI manipulation
- Easy to add new GUI patches (UI mods)
- Base class handles common patterns

**Note**: Fix typo in `CSWGuiObject.h` line 15: `offsestInitialized` → `offsetsInitialized`

### Additional CGameObject Derivatives

**Server-Side Objects** (CSWSObject children):
- CSWSDoor (doors)
- CSWSPlaceable (placeables)
- CSWSTrigger (triggers)
- CSWSAreaOfEffectObject (AOE effects)
- CSWSStore (merchants)
- CSWSSound (sound objects)
- CSWSWaypoint (waypoints)

**Client-Side Objects** (CSWCObject children):
- CSWCCreature (already exists, needs migration)
- CSWCItem (client items)
- CSWCDoor (client doors)
- CSWCPlaceable (client placeables)

### Manager Classes

**Additional Managers to Wrap**:
- CExoResMan (resource manager)
- CTlkTable (dialog.tlk access)
- C2DACache (2DA file cache)
- CWorldTimer (game time)
- CRules (game rules/mechanics)
- CServerAIMaster (AI system)

### Value Types

**Additional Data Types**:
- CExoLocString (localized strings with multiple language entries)
- Vector (3D vectors - used everywhere)
- CScriptLocation (location = position + orientation)
- CResStruct (GFF struct wrappers)
- CResGFF (GFF file access)

---

## Implementation Checklist

When implementing this refactor, follow this checklist:

### Phase 1: Foundation
- [ ] Create `GameAPI/Core/` directory
- [ ] Implement `GameAPIObject.h` with documentation
- [ ] Implement `GameAPIObject.cpp`
- [ ] Write unit tests for GameAPIObject (if test framework exists)
- [ ] Create remaining subdirectories (Objects, GUI, Managers, Types)

### Phase 2: Core Classes
- [ ] Move and refactor `GameVersion.h/cpp` to `Core/`
- [ ] Move `Common.h` to `Core/`
- [ ] Update CGameObject to inherit from GameAPIObject
- [ ] Update CSWSObject (inherits from CGameObject)
- [ ] Update CSWSCreature (inherits from CSWSObject)
- [ ] Verify inheritance chain works correctly

### Phase 3: Remaining Classes
- [ ] Update and move all Object classes
  - [ ] CGameObjectArray
  - [ ] CSWCCreature
  - [ ] CSWItem
  - [ ] CSWSCreatureStats
  - [ ] CSWInventory
- [ ] Update and move all GUI classes
  - [ ] CSWGuiObject (fix typo: offsestInitialized)
- [ ] Update and move all Manager classes
  - [ ] CAppManager
  - [ ] CVirtualMachine
  - [ ] CClientExoApp
  - [ ] CServerExoApp
- [ ] Update and move all Type classes
  - [ ] CExoString
  - [ ] CResRef
  - [ ] CClientOptions

### Phase 4: Update Patches
- [ ] Create script to find all GameAPI includes: `grep -r '#include "GameAPI/' Patches/`
- [ ] Update includes in ScriptExtender patch
- [ ] Update includes in AdditionalConsoleCommands patch
- [ ] Update includes in Freecam patch
- [ ] Update includes in any other patches

### Phase 5: Testing
- [ ] Build all patches successfully
- [ ] Test ScriptExtender functionality
- [ ] Test AdditionalConsoleCommands functionality
- [ ] Test Freecam functionality
- [ ] Verify no runtime errors in game

### Phase 6: Documentation
- [ ] Add GameAPI/README.md with directory structure overview
- [ ] Update CLAUDE.md with new GameAPI patterns
- [ ] Create class template file for contributors
- [ ] Add CONTRIBUTING.md section on adding GameAPI classes

---

## Benefits Summary

### For Maintainability
1. **Consistent Patterns**: All classes follow the same interface
2. **Less Duplication**: Constructor boilerplate, GetPtr(), memory management shared
3. **Compile-Time Enforcement**: Pure virtual methods ensure derived classes implement required functions
4. **Clear Organization**: Subdirectories make it easy to find classes by category

### For Scalability
1. **Room to Grow**: Can add hundreds of GUI classes without cluttering root directory
2. **Inheritance Mirrors Game**: Our class hierarchy matches the game engine's structure
3. **Documentation Built-In**: Directory structure documents relationships
4. **Easier Contributions**: Clear patterns for developers to follow

### For New Contributors
1. **Obvious Structure**: Directories tell you where to look
2. **Template Available**: Copy existing class pattern for new wrappers
3. **Self-Documenting**: Base class documents common interface
4. **Less Error-Prone**: Compiler enforces correct patterns

---

## Potential Challenges

### Challenge 1: Build System Adjustments

**Issue**: Patches include GameAPI files directly. Moving files may break includes.

**Solution**: Direct migration of all include paths is straightforward. Use grep to find all occurrences:
```bash
grep -r '#include "GameAPI/' Patches/
```
Update each occurrence to use new paths.

### Challenge 2: Virtual Function Overhead

**Issue**: Making InitializeFunctions/InitializeOffsets virtual adds v-table overhead.

**Impact**: Minimal. These functions are called once per class at initialization, not in hot paths. The overhead is:
- 8 bytes per instance for v-table pointer
- Negligible call overhead (one pointer dereference)

**Benefit**: Far outweighs cost - compile-time enforcement prevents bugs.

### Challenge 3: shouldFree Overhead

**Issue**: Adding shouldFree to every instance adds 1-4 bytes per object.

**Impact**: Negligible. GameAPI objects are created infrequently (not thousands per frame). Memory is not a concern.

**Benefit**: Generalizes the CExoString pattern, enables safe memory management for all value types.

### Challenge 4: Migration Testing

**Issue**: Need to ensure all patches still work after migration.

**Solution**:
1. Migrate one class at a time, test incrementally
2. Start with leaf classes (CSWSCreature) before base classes
3. Build and test patches after each class migration
4. Keep git history clean with one commit per class

### Challenge 5: Empty Virtual Implementations

**Issue**: Some classes (like CGameObjectArray) don't need InitializeFunctions/InitializeOffsets but must implement them.

**Solution**: This is intentional - documents that the class has no functions/offsets to initialize. Empty implementation is clear and explicit:
```cpp
void CGameObjectArray::InitializeFunctions() override {
    // No functions to initialize
}
```

---

## Open Questions

### Q1: Should GameAPIObject be in its own file or combined with Common.h?

**Recommendation**: Separate file (`GameAPIObject.h/cpp`)
- Clear separation of concerns
- Common.h contains templates and utilities, not class definitions
- Easier to document and maintain

### Q2: Should we add GetTypeName() or type identification to GameAPIObject?

**Consideration**: Could add RTTI-style type identification:
```cpp
virtual const char* GetTypeName() const = 0;
```

**Recommendation**: Not in initial refactor. Add later if needed for debugging/logging.

### Q3: Should we provide forwarding headers for backwards compatibility?

**Decision**: No forwarding headers (per user's choice of "Direct migration")
- Clean break, no legacy cruft
- Forces conscious migration
- All patches in same repo, can update together

### Q4: Should InitializeFunctions/InitializeOffsets be protected or public?

**Current**: Static methods called from constructors (implicitly protected/private)
**Proposed**: Public virtual methods in base class

**Recommendation**: Keep public. Allows manual initialization if needed, no harm in calling explicitly.

### Q5: Should we use smart pointers (std::unique_ptr, std::shared_ptr)?

**Consideration**: Could use smart pointers for objectPtr management.

**Recommendation**: No. Wrapping raw game pointers, not managing C++ objects. Raw pointers are appropriate here. shouldFree flag is sufficient for the few cases where we allocate.

---

## Conclusion

This refactor establishes a solid foundation for the GameAPI system as it scales from ~15 classes to potentially hundreds. The changes are lightweight (one new base class, directory reorganization) but provide significant long-term benefits:

- **Consistency**: All wrappers follow the same patterns
- **Scalability**: Room to grow GUI and game object hierarchies
- **Maintainability**: Clear organization, reduced duplication
- **Contributor-Friendly**: Easy to understand and extend

The migration is straightforward (update includes, inherit from base class) and can be done incrementally with testing at each step.

**Estimated Effort**:
- Base class implementation: 2-3 hours
- Migrating 16 existing classes: 4-6 hours
- Updating patch includes: 1-2 hours
- Testing: 2-3 hours
- **Total: ~10-14 hours**

A small investment for long-term sustainability as the project grows.
