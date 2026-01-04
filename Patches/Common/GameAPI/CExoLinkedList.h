#pragma once
#include "../Common.h"
#include "GameAPIObject.h"
#include <type_traits>

// CExoLinkedListNode structure definition
// Memory layout: { CExoLinkedListNode* prev (0x0), CExoLinkedListNode* next (0x4), void* data (0x8) } = 0xC bytes
struct CExoLinkedListNode {
    CExoLinkedListNode* prev;  // 0x0
    CExoLinkedListNode* next;  // 0x4
    void* data;                // 0x8
};

// CExoLinkedListInternal structure definition
// Memory layout: { CExoLinkedListNode* head (0x0), CExoLinkedListNode* tail (0x4), int count (0x8) } = 0xC bytes
struct CExoLinkedListInternal {
    CExoLinkedListNode* head;  // 0x0
    CExoLinkedListNode* tail;  // 0x4
    int count;                 // 0x8
};

/// <summary>
/// Templated wrapper for KotOR's CExoLinkedList<T> class.
/// Memory layout: { CExoLinkedListInternal* internal (0x0) } = 0x4 bytes
///
/// CExoLinkedListInternal: { CExoLinkedListNode* head (0x0), CExoLinkedListNode* tail (0x4), int count (0x8) } = 0xC bytes
/// CExoLinkedListNode: { CExoLinkedListNode* prev (0x0), CExoLinkedListNode* next (0x4), void* data (0x8) } = 0xC bytes
///
/// Unlike CExoArrayList, this class uses game functions from CExoLinkedListInternal for most operations.
/// However, like CExoArrayList, we implement our own constructor logic to support any type T.
/// </summary>
template<typename T>
class CExoLinkedList : public GameAPIObject {
public:
    /// <summary>
    /// Wraps an existing CExoLinkedList in game memory.
    /// </summary>
    /// <param name="listPtr">Pointer to existing game CExoLinkedList</param>
    explicit CExoLinkedList(void* listPtr);

    /// <summary>
    /// Default constructor - creates an empty list.
    /// </summary>
    CExoLinkedList();

    /// <summary>
    /// Copy constructor - creates a deep copy of another list.
    /// </summary>
    /// <param name="copy">List to copy from</param>
    CExoLinkedList(const CExoLinkedList<T>& copy);

    /// <summary>
    /// Destructor - cleans up allocated memory.
    /// </summary>
    ~CExoLinkedList();

    // === Core operations ===

    /// <summary>
    /// Adds a value to the head of the list.
    /// </summary>
    void AddHead(const T& value);

    /// <summary>
    /// Adds a value to the tail of the list.
    /// </summary>
    void AddTail(const T& value);

    /// <summary>
    /// Adds a value before the specified position.
    /// </summary>
    /// <param name="value">Value to add</param>
    /// <param name="position">Position to insert before</param>
    void AddBefore(const T& value, CExoLinkedListNode* position);

    /// <summary>
    /// Removes the head element from the list and returns its data.
    /// </summary>
    /// <returns>Pointer to the removed data, or nullptr if list is empty</returns>
    T* RemoveHead();

    /// <summary>
    /// Removes the tail element from the list and returns its data.
    /// </summary>
    /// <returns>Pointer to the removed data, or nullptr if list is empty</returns>
    T* RemoveTail();

    /// <summary>
    /// Removes the element at the specified position and returns its data.
    /// </summary>
    /// <param name="position">Position to remove</param>
    /// <returns>Pointer to the removed data, or nullptr if position is invalid</returns>
    T* Remove(CExoLinkedListNode* position);

    /// <summary>
    /// Checks if the list contains a value.
    /// </summary>
    /// <param name="value">Value to search for</param>
    /// <returns>True if value exists in list</returns>
    bool Contains(const T& value) const;

    /// <summary>
    /// Removes all elements and frees memory.
    /// </summary>
    void Clear();

    // === Getters ===

    /// <summary>
    /// Gets the number of elements in the list.
    /// </summary>
    int GetCount() const;

    /// <summary>
    /// Gets the head position for iteration.
    /// </summary>
    /// <returns>Position pointer or nullptr if empty</returns>
    CExoLinkedListNode* GetHeadPosition() const;

    /// <summary>
    /// Gets the tail position for iteration.
    /// </summary>
    /// <returns>Position pointer or nullptr if empty</returns>
    CExoLinkedListNode* GetTailPosition() const;

    /// <summary>
    /// Gets the value at the specified position.
    /// </summary>
    /// <param name="position">Position to get</param>
    /// <returns>Pointer to value at position, or nullptr if invalid</returns>
    T* GetAt(CExoLinkedListNode* position);
    const T* GetAt(CExoLinkedListNode* position) const;

    /// <summary>
    /// Advances to the next position and returns the data at the new position.
    /// </summary>
    /// <param name="position">Position reference to advance</param>
    /// <returns>Pointer to data at new position, or nullptr if end of list</returns>
    T* GetNext(CExoLinkedListNode*& position);
    const T* GetNext(CExoLinkedListNode*& position) const;

    /// <summary>
    /// Goes back to the previous position and returns the data at the new position.
    /// </summary>
    /// <param name="position">Position reference to move back</param>
    /// <returns>Pointer to data at new position, or nullptr if beginning of list</returns>
    T* GetPrev(CExoLinkedListNode*& position);
    const T* GetPrev(CExoLinkedListNode*& position) const;

    // === Operators ===

    /// <summary>
    /// Assignment operator - copies content from another list.
    /// </summary>
    CExoLinkedList<T>& operator=(const CExoLinkedList<T>& rhs);

    /// <summary>
    /// Equality comparison.
    /// </summary>
    bool operator==(const CExoLinkedList<T>& rhs) const;

    /// <summary>
    /// Inequality comparison.
    /// </summary>
    bool operator!=(const CExoLinkedList<T>& rhs) const;

    // === GameAPIObject overrides ===

    void InitializeFunctions() override;
    void InitializeOffsets() override;

private:
    // === Static members for CExoLinkedListInternal game functions ===
    static bool functionsInitialized;
    static bool offsetsInitialized;

    // Function typedefs for CExoLinkedListInternal
    typedef void(__thiscall* InternalConstructor)(CExoLinkedListInternal* thisPtr);
    typedef CExoLinkedListNode*(__thiscall* InternalAddHead)(CExoLinkedListInternal* thisPtr, void* value);
    typedef CExoLinkedListNode*(__thiscall* InternalAddTail)(CExoLinkedListInternal* thisPtr, void* value);
    typedef CExoLinkedListNode*(__thiscall* InternalAddBefore)(CExoLinkedListInternal* thisPtr, void* value, CExoLinkedListNode* position);
    typedef void*(__thiscall* InternalRemoveHead)(CExoLinkedListInternal* thisPtr);
    typedef void*(__thiscall* InternalRemoveTail)(CExoLinkedListInternal* thisPtr);
    typedef void*(__thiscall* InternalRemove)(CExoLinkedListInternal* thisPtr, CExoLinkedListNode* position);
    typedef int(__thiscall* InternalContains)(CExoLinkedListInternal* thisPtr, void* value);
    typedef void*(__thiscall* InternalGetAtPos)(CExoLinkedListInternal* thisPtr, CExoLinkedListNode* position);
    typedef void*(__thiscall* InternalGetNext)(CExoLinkedListInternal* thisPtr, CExoLinkedListNode** position);
    typedef void*(__thiscall* InternalGetPrev)(CExoLinkedListInternal* thisPtr, CExoLinkedListNode** position);

    // Static function pointers
    static InternalConstructor internalConstructor;
    static InternalAddHead internalAddHead;
    static InternalAddTail internalAddTail;
    static InternalAddBefore internalAddBefore;
    static InternalRemoveHead internalRemoveHead;
    static InternalRemoveTail internalRemoveTail;
    static InternalRemove internalRemove;
    static InternalContains internalContains;
    static InternalGetAtPos internalGetAtPos;
    static InternalGetNext internalGetNext;
    static InternalGetPrev internalGetPrev;

    /// <summary>
    /// Helper to get the CExoLinkedListInternal pointer.
    /// </summary>
    CExoLinkedListInternal* GetInternal() const;

    /// <summary>
    /// Helper to set the CExoLinkedListInternal pointer.
    /// </summary>
    void SetInternal(CExoLinkedListInternal* internal);

    /// <summary>
    /// Helper to allocate and initialize a new CExoLinkedListInternal.
    /// </summary>
    void AllocateInternal();

    /// <summary>
    /// Helper to free a node's data if needed (for non-trivial types).
    /// </summary>
    void DestroyNodeData(void* data);
};

// === Static member initialization ===
template<typename T>
bool CExoLinkedList<T>::functionsInitialized = false;

template<typename T>
bool CExoLinkedList<T>::offsetsInitialized = false;

template<typename T>
typename CExoLinkedList<T>::InternalConstructor CExoLinkedList<T>::internalConstructor = nullptr;

template<typename T>
typename CExoLinkedList<T>::InternalAddHead CExoLinkedList<T>::internalAddHead = nullptr;

template<typename T>
typename CExoLinkedList<T>::InternalAddTail CExoLinkedList<T>::internalAddTail = nullptr;

template<typename T>
typename CExoLinkedList<T>::InternalAddBefore CExoLinkedList<T>::internalAddBefore = nullptr;

template<typename T>
typename CExoLinkedList<T>::InternalRemoveHead CExoLinkedList<T>::internalRemoveHead = nullptr;

template<typename T>
typename CExoLinkedList<T>::InternalRemoveTail CExoLinkedList<T>::internalRemoveTail = nullptr;

template<typename T>
typename CExoLinkedList<T>::InternalRemove CExoLinkedList<T>::internalRemove = nullptr;

template<typename T>
typename CExoLinkedList<T>::InternalContains CExoLinkedList<T>::internalContains = nullptr;

template<typename T>
typename CExoLinkedList<T>::InternalGetAtPos CExoLinkedList<T>::internalGetAtPos = nullptr;

template<typename T>
typename CExoLinkedList<T>::InternalGetNext CExoLinkedList<T>::internalGetNext = nullptr;

template<typename T>
typename CExoLinkedList<T>::InternalGetPrev CExoLinkedList<T>::internalGetPrev = nullptr;

// === Implementation ===

template<typename T>
CExoLinkedList<T>::CExoLinkedList(void* listPtr)
    : GameAPIObject(listPtr, false) {  // false = wrapping existing memory

    if (!functionsInitialized) {
        InitializeFunctions();
    }

    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

template<typename T>
CExoLinkedList<T>::CExoLinkedList()
    : GameAPIObject(nullptr, true) {  // true = we own this memory

    if (!functionsInitialized) {
        InitializeFunctions();
    }

    if (!offsetsInitialized) {
        InitializeOffsets();
    }

    // Allocate the 0x4 byte structure (just a pointer to CExoLinkedListInternal)
    objectPtr = malloc(0x4);
    if (objectPtr) {
        AllocateInternal();
    }
}

template<typename T>
CExoLinkedList<T>::CExoLinkedList(const CExoLinkedList<T>& copy)
    : GameAPIObject(nullptr, true) {  // true = we own this memory

    if (!functionsInitialized) {
        InitializeFunctions();
    }

    if (!offsetsInitialized) {
        InitializeOffsets();
    }

    // Allocate the 0x4 byte structure
    objectPtr = malloc(0x4);
    if (objectPtr) {
        AllocateInternal();

        // Copy elements from source
        *this = copy;
    }
}

template<typename T>
CExoLinkedList<T>::~CExoLinkedList() {
    if (shouldFree && objectPtr) {
        Clear();

        // Free the CExoLinkedListInternal
        CExoLinkedListInternal* internal = GetInternal();
        if (internal) {
            free(internal);
        }

        free(objectPtr);
        objectPtr = nullptr;
    }
}

template<typename T>
void CExoLinkedList<T>::AddHead(const T& value) {
    CExoLinkedListInternal* internal = GetInternal();
    if (!internal || !internalAddHead) return;

    // Allocate memory for the value and copy it
    T* valueCopy = static_cast<T*>(malloc(sizeof(T)));
    if (valueCopy) {
        new (valueCopy) T(value);  // Placement new for copy construction
        internalAddHead(internal, valueCopy);
    }
}

template<typename T>
void CExoLinkedList<T>::AddTail(const T& value) {
    CExoLinkedListInternal* internal = GetInternal();
    if (!internal || !internalAddTail) return;

    // Allocate memory for the value and copy it
    T* valueCopy = static_cast<T*>(malloc(sizeof(T)));
    if (valueCopy) {
        new (valueCopy) T(value);  // Placement new for copy construction
        internalAddTail(internal, valueCopy);
    }
}

template<typename T>
void CExoLinkedList<T>::AddBefore(const T& value, CExoLinkedListNode* position) {
    CExoLinkedListInternal* internal = GetInternal();
    if (!internal || !internalAddBefore || !position) return;

    // Allocate memory for the value and copy it
    T* valueCopy = static_cast<T*>(malloc(sizeof(T)));
    if (valueCopy) {
        new (valueCopy) T(value);  // Placement new for copy construction
        internalAddBefore(internal, valueCopy, position);
    }
}

template<typename T>
T* CExoLinkedList<T>::RemoveHead() {
    CExoLinkedListInternal* internal = GetInternal();
    if (!internal || !internalRemoveHead) return nullptr;

    // RemoveHead returns the data that was removed
    void* data = internalRemoveHead(internal);
    return static_cast<T*>(data);
}

template<typename T>
T* CExoLinkedList<T>::RemoveTail() {
    CExoLinkedListInternal* internal = GetInternal();
    if (!internal || !internalRemoveTail) return nullptr;

    // RemoveTail returns the data that was removed
    void* data = internalRemoveTail(internal);
    return static_cast<T*>(data);
}

template<typename T>
T* CExoLinkedList<T>::Remove(CExoLinkedListNode* position) {
    CExoLinkedListInternal* internal = GetInternal();
    if (!internal || !internalRemove || !position) return nullptr;

    // Remove returns the data that was removed
    void* data = internalRemove(internal, position);
    return static_cast<T*>(data);
}

template<typename T>
bool CExoLinkedList<T>::Contains(const T& value) const {
    CExoLinkedListInternal* internal = GetInternal();
    if (!internal || !internalContains) return false;

    // For Contains, we need to search manually since we can't pass a const T& to the game function
    CExoLinkedListNode* pos = GetHeadPosition();
    while (pos) {
        const T* data = GetAt(pos);
        if (data && *data == value) {
            return true;
        }
        GetNext(pos);
    }
    return false;
}

template<typename T>
void CExoLinkedList<T>::Clear() {
    CExoLinkedListInternal* internal = GetInternal();
    if (!internal) return;

    // Remove all nodes and free their data
    while (GetCount() > 0) {
        T* data = RemoveHead();
        DestroyNodeData(data);
    }
}

template<typename T>
int CExoLinkedList<T>::GetCount() const {
    CExoLinkedListInternal* internal = GetInternal();
    if (!internal) return 0;
    return internal->count;
}

template<typename T>
CExoLinkedListNode* CExoLinkedList<T>::GetHeadPosition() const {
    CExoLinkedListInternal* internal = GetInternal();
    if (!internal) return nullptr;
    return internal->head;
}

template<typename T>
CExoLinkedListNode* CExoLinkedList<T>::GetTailPosition() const {
    CExoLinkedListInternal* internal = GetInternal();
    if (!internal) return nullptr;
    return internal->tail;
}

template<typename T>
T* CExoLinkedList<T>::GetAt(CExoLinkedListNode* position) {
    CExoLinkedListInternal* internal = GetInternal();
    if (!internal || !internalGetAtPos || !position) return nullptr;

    void* data = internalGetAtPos(internal, position);
    return static_cast<T*>(data);
}

template<typename T>
const T* CExoLinkedList<T>::GetAt(CExoLinkedListNode* position) const {
    CExoLinkedListInternal* internal = GetInternal();
    if (!internal || !internalGetAtPos || !position) return nullptr;

    void* data = internalGetAtPos(internal, position);
    return static_cast<const T*>(data);
}

template<typename T>
T* CExoLinkedList<T>::GetNext(CExoLinkedListNode*& position) {
    CExoLinkedListInternal* internal = GetInternal();
    if (!internal || !internalGetNext || !position) return nullptr;

    void* data = internalGetNext(internal, &position);
    return static_cast<T*>(data);
}

template<typename T>
const T* CExoLinkedList<T>::GetNext(CExoLinkedListNode*& position) const {
    CExoLinkedListInternal* internal = GetInternal();
    if (!internal || !internalGetNext || !position) return nullptr;

    void* data = internalGetNext(internal, &position);
    return static_cast<const T*>(data);
}

template<typename T>
T* CExoLinkedList<T>::GetPrev(CExoLinkedListNode*& position) {
    CExoLinkedListInternal* internal = GetInternal();
    if (!internal || !internalGetPrev || !position) return nullptr;

    void* data = internalGetPrev(internal, &position);
    return static_cast<T*>(data);
}

template<typename T>
const T* CExoLinkedList<T>::GetPrev(CExoLinkedListNode*& position) const {
    CExoLinkedListInternal* internal = GetInternal();
    if (!internal || !internalGetPrev || !position) return nullptr;

    void* data = internalGetPrev(internal, &position);
    return static_cast<const T*>(data);
}

template<typename T>
CExoLinkedList<T>& CExoLinkedList<T>::operator=(const CExoLinkedList<T>& rhs) {
    if (this == &rhs) return *this;

    // Clear existing content
    Clear();

    // Copy elements from rhs
    CExoLinkedListNode* pos = rhs.GetHeadPosition();
    while (pos) {
        const T* data = rhs.GetAt(pos);
        if (data) {
            AddTail(*data);
        }
        rhs.GetNext(pos);
    }

    return *this;
}

template<typename T>
bool CExoLinkedList<T>::operator==(const CExoLinkedList<T>& rhs) const {
    if (GetCount() != rhs.GetCount()) return false;

    CExoLinkedListNode* lhsPos = GetHeadPosition();
    CExoLinkedListNode* rhsPos = rhs.GetHeadPosition();

    while (lhsPos && rhsPos) {
        const T* lhsData = GetAt(lhsPos);
        const T* rhsData = rhs.GetAt(rhsPos);

        if (!lhsData || !rhsData || *lhsData != *rhsData) {
            return false;
        }

        GetNext(lhsPos);
        rhs.GetNext(rhsPos);
    }

    return true;
}

template<typename T>
bool CExoLinkedList<T>::operator!=(const CExoLinkedList<T>& rhs) const {
    return !(*this == rhs);
}

template<typename T>
void CExoLinkedList<T>::InitializeFunctions() {
    if (functionsInitialized) return;

    if (!GameVersion::IsInitialized()) {
        OutputDebugStringA("[CExoLinkedList] ERROR: GameVersion not initialized\n");
        return;
    }

    try {
        internalConstructor = reinterpret_cast<InternalConstructor>(
            GameVersion::GetFunctionAddress("CExoLinkedListInternal", "Constructor")
        );
        internalAddHead = reinterpret_cast<InternalAddHead>(
            GameVersion::GetFunctionAddress("CExoLinkedListInternal", "AddHead")
        );
        internalAddTail = reinterpret_cast<InternalAddTail>(
            GameVersion::GetFunctionAddress("CExoLinkedListInternal", "AddTail")
        );
        internalAddBefore = reinterpret_cast<InternalAddBefore>(
            GameVersion::GetFunctionAddress("CExoLinkedListInternal", "AddBefore")
        );
        internalRemoveHead = reinterpret_cast<InternalRemoveHead>(
            GameVersion::GetFunctionAddress("CExoLinkedListInternal", "RemoveHead")
        );
        internalRemoveTail = reinterpret_cast<InternalRemoveTail>(
            GameVersion::GetFunctionAddress("CExoLinkedListInternal", "RemoveTail")
        );
        internalRemove = reinterpret_cast<InternalRemove>(
            GameVersion::GetFunctionAddress("CExoLinkedListInternal", "Remove")
        );
        internalContains = reinterpret_cast<InternalContains>(
            GameVersion::GetFunctionAddress("CExoLinkedListInternal", "Contains")
        );
        internalGetAtPos = reinterpret_cast<InternalGetAtPos>(
            GameVersion::GetFunctionAddress("CExoLinkedListInternal", "GetAtPos")
        );
        internalGetNext = reinterpret_cast<InternalGetNext>(
            GameVersion::GetFunctionAddress("CExoLinkedListInternal", "GetNext")
        );
        internalGetPrev = reinterpret_cast<InternalGetPrev>(
            GameVersion::GetFunctionAddress("CExoLinkedListInternal", "GetPrev")
        );

        functionsInitialized = true;
    }
    catch (const GameVersionException& e) {
        debugLog("[CExoLinkedList] ERROR: %s\n", e.what());
    }
}

template<typename T>
void CExoLinkedList<T>::InitializeOffsets() {
    if (offsetsInitialized) return;

    // Offsets are fixed and known:
    // CExoLinkedList<T>: internal @ 0x0
    // CExoLinkedListInternal: head @ 0x0, tail @ 0x4, count @ 0x8
    // CExoLinkedListNode: prev @ 0x0, next @ 0x4, data @ 0x8
    offsetsInitialized = true;
}

template<typename T>
CExoLinkedListInternal* CExoLinkedList<T>::GetInternal() const {
    if (!objectPtr) return nullptr;
    return getObjectProperty<CExoLinkedListInternal*>(objectPtr, 0x0);
}

template<typename T>
void CExoLinkedList<T>::SetInternal(CExoLinkedListInternal* internal) {
    if (objectPtr) {
        setObjectProperty<CExoLinkedListInternal*>(objectPtr, 0x0, internal);
    }
}

template<typename T>
void CExoLinkedList<T>::AllocateInternal() {
    if (!objectPtr || !internalConstructor) return;

    // Allocate the 0xC byte CExoLinkedListInternal structure
    CExoLinkedListInternal* internal = static_cast<CExoLinkedListInternal*>(malloc(0xC));
    if (internal) {
        // Call the game's constructor to initialize it
        internalConstructor(internal);
        SetInternal(internal);
    }
}

template<typename T>
void CExoLinkedList<T>::DestroyNodeData(void* data) {
    if (!data) return;

    T* typedData = static_cast<T*>(data);

    // Call destructor if needed
    if (!std::is_trivially_destructible<T>::value) {
        typedData->~T();
    }

    // Free the memory
    free(typedData);
}
