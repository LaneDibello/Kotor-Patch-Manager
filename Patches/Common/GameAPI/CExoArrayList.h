#pragma once
#include "../Common.h"
#include "GameAPIObject.h"
#include <cstring>
#include <type_traits>

/// <summary>
/// Templated wrapper for KotOR's CExoArrayList<T> class.
/// Memory layout: { T* data (0x0), int size (0x4), int capacity (0x8) } = 0xC bytes
///
/// Unlike other GameAPI classes, CExoArrayList implements its own logic rather than
/// calling game functions. This allows it to work with any type T and avoids issues
/// with missing/optimized-out functions in the game binary.
/// </summary>
template<typename T>
class CExoArrayList : public GameAPIObject {
    // === Element category (see docs/CExoArrayList_WrapperAware.md) ===
    // A "wrapper pointer" element (e.g. CSWGuiControl*) is a pointer to a
    // GameAPIObject-derived handle. The game stores these lists as arrays of
    // raw game-object pointers, so we marshal via GetPtr() on the way in and
    // reconstruct a wrapper on the way out.
    static constexpr bool kIsWrapperPtr =
        std::is_pointer<T>::value &&
        std::is_base_of<GameAPIObject, typename std::remove_pointer<T>::type>::value;

    // A "wrapper value" element (e.g. CSWGuiControl) would require sizing slots
    // by the game class size and a (shallow) memcpy on copy. Reserved for now.
    static constexpr bool kIsWrapperValue =
        !std::is_pointer<T>::value &&
        std::is_base_of<GameAPIObject, T>::value;

    static_assert(!kIsWrapperValue,
        "CExoArrayList<Wrapper> (by-value storage of a GameAPIObject) is not yet "
        "supported. Use CExoArrayList<Wrapper*> for the game's pointer arrays. "
        "See docs/CExoArrayList_WrapperAware.md (row 4).");

public:
    /// <summary>
    /// Wraps an existing CExoArrayList in game memory.
    /// </summary>
    /// <param name="arrayPtr">Pointer to existing game CExoArrayList</param>
    explicit CExoArrayList(void* arrayPtr);

    /// <summary>
    /// Default constructor - creates an empty array.
    /// </summary>
    CExoArrayList();

    /// <summary>
    /// Allocation constructor - creates an array with specified capacity.
    /// </summary>
    /// <param name="capacity">Initial capacity to allocate</param>
    explicit CExoArrayList(int capacity);

    /// <summary>
    /// Copy constructor - creates a deep copy of another array.
    /// </summary>
    /// <param name="copy">Array to copy from</param>
    CExoArrayList(const CExoArrayList<T>& copy);

    /// <summary>
    /// Destructor - cleans up allocated memory.
    /// </summary>
    ~CExoArrayList();

    // === Core operations ===

    /// <summary>
    /// Appends a value to the end of the array.
    /// </summary>
    void Add(const T& value);

    /// <summary>
    /// Appends a value only if it doesn't already exist in the array.
    /// </summary>
    void AddUnique(const T& value);

    /// <summary>
    /// Allocates or reallocates the internal array to the specified capacity.
    /// </summary>
    void Allocate(int newCapacity);

    /// <summary>
    /// Removes all elements and frees memory.
    /// </summary>
    void Clear();

    /// <summary>
    /// Counts how many times a value appears in the array.
    /// </summary>
    int Count(const T& value) const;

    /// <summary>
    /// Deletes the element at the specified index, shifting subsequent elements back.
    /// </summary>
    void DeleteAt(int index);

    /// <summary>
    /// Finds the first index of a value, or -1 if not found.
    /// </summary>
    int IndexOf(const T& value) const;

    /// <summary>
    /// Inserts a value at the specified index, shifting subsequent elements forward.
    /// </summary>
    void Insert(const T& value, int index);

    /// <summary>
    /// Removes the last occurrence of a value from the array.
    /// </summary>
    void Remove(const T& value);

    /// <summary>
    /// Removes all occurrences of a value from the array.
    /// </summary>
    void RemoveAll(const T& value);

    /// <summary>
    /// Sets the size of the array (typically for shrinking).
    /// </summary>
    void SetSize(int newSize);

    // === Getters ===

    /// <summary>
    /// Gets pointer to the internal data array.
    /// </summary>
    T* GetData() const;

    /// <summary>
    /// Gets the current number of elements.
    /// </summary>
    int GetSize() const;

    /// <summary>
    /// Gets the current capacity.
    /// </summary>
    int GetCapacity() const;

    /// <summary>
    /// Gets the raw stored value at an index as a void*. For wrapper-pointer
    /// lists this is the raw game-object pointer; for other lists it is the
    /// element's bytes reinterpreted as a pointer (rarely useful for PODs).
    /// </summary>
    void* GetRaw(int index) const;

    /// <summary>
    /// Invokes fn for each element in order. For wrapper-pointer lists
    /// (e.g. CExoArrayList&lt;CSWGuiControl*&gt;), fn receives a non-owning stack
    /// wrapper by reference (e.g. CSWGuiControl&amp;). For all other element types,
    /// fn receives a reference to the stored element (T&amp;). This is the safe way
    /// to read wrapper lists; see operator[] caveat in the docs.
    /// </summary>
    template<typename Fn>
    void ForEach(Fn&& fn);

    // === Operators ===

    /// <summary>
    /// Array subscript operator for element access.
    /// </summary>
    T& operator[](int index);
    const T& operator[](int index) const;

    /// <summary>
    /// Assignment operator - copies content from another array.
    /// </summary>
    CExoArrayList<T>& operator=(const CExoArrayList<T>& rhs);

    /// <summary>
    /// Equality comparison.
    /// </summary>
    bool operator==(const CExoArrayList<T>& rhs) const;

    /// <summary>
    /// Inequality comparison.
    /// </summary>
    bool operator!=(const CExoArrayList<T>& rhs) const;

    // === GameAPIObject overrides ===

    void InitializeFunctions() override;
    void InitializeOffsets() override;

private:
    static bool functionsInitialized;
    static bool offsetsInitialized;

    /// <summary>
    /// Maps a caller-supplied element to the value actually stored in the
    /// buffer. For wrapper-pointer lists this is the wrapped raw game pointer
    /// (value-&gt;GetPtr()); for all other element types it is the value itself.
    /// Used by every store/compare path so the buffer always holds the same
    /// representation the game expects.
    /// </summary>
    static T toStored(const T& value);

    /// <summary>
    /// Helper to grow capacity when needed (doubles capacity, or sets to 10 if 0).
    /// </summary>
    void GrowIfNeeded();

    /// <summary>
    /// Helper to call destructors on elements if needed.
    /// </summary>
    void DestroyElement(T* element);

    /// <summary>
    /// Helper to set the internal data pointer.
    /// </summary>
    void SetData(T* data);

    /// <summary>
    /// Helper to set the internal size.
    /// </summary>
    void SetSizeInternal(int size);

    /// <summary>
    /// Helper to set the internal capacity.
    /// </summary>
    void SetCapacityInternal(int capacity);
};

// === Static member initialization ===
template<typename T>
bool CExoArrayList<T>::functionsInitialized = false;

template<typename T>
bool CExoArrayList<T>::offsetsInitialized = false;

// === Implementation ===

template<typename T>
CExoArrayList<T>::CExoArrayList(void* arrayPtr)
    : GameAPIObject(arrayPtr, false) {  // false = wrapping existing memory

    if (!functionsInitialized) {
        InitializeFunctions();
    }

    if (!offsetsInitialized) {
        InitializeOffsets();
    }
}

template<typename T>
CExoArrayList<T>::CExoArrayList()
    : GameAPIObject(nullptr, true) {  // true = we own this memory

    if (!functionsInitialized) {
        InitializeFunctions();
    }

    if (!offsetsInitialized) {
        InitializeOffsets();
    }

    // Allocate the 0xC byte structure
    objectPtr = malloc(0xC);
    if (objectPtr) {
        SetData(nullptr);
        SetSizeInternal(0);
        SetCapacityInternal(0);
    }
}

template<typename T>
CExoArrayList<T>::CExoArrayList(int capacity)
    : GameAPIObject(nullptr, true) {  // true = we own this memory

    if (!functionsInitialized) {
        InitializeFunctions();
    }

    if (!offsetsInitialized) {
        InitializeOffsets();
    }

    // Allocate the 0xC byte structure
    objectPtr = malloc(0xC);
    if (objectPtr) {
        SetData(nullptr);
        SetSizeInternal(0);
        SetCapacityInternal(0);
        Allocate(capacity);
    }
}

template<typename T>
CExoArrayList<T>::CExoArrayList(const CExoArrayList<T>& copy)
    : GameAPIObject(nullptr, true) {  // true = we own this memory

    if (!functionsInitialized) {
        InitializeFunctions();
    }

    if (!offsetsInitialized) {
        InitializeOffsets();
    }

    // Allocate the 0xC byte structure
    objectPtr = malloc(0xC);
    if (objectPtr) {
        SetData(nullptr);
        SetSizeInternal(0);
        SetCapacityInternal(0);

        // Copy elements from source
        *this = copy;
    }
}

template<typename T>
CExoArrayList<T>::~CExoArrayList() {
    if (shouldFree && objectPtr) {
        Clear();
        free(objectPtr);
        objectPtr = nullptr;
    }
}

template<typename T>
void CExoArrayList<T>::Add(const T& value) {
    GrowIfNeeded();

    T* data = GetData();
    int size = GetSize();

    if (data) {
        data[size] = toStored(value);
        SetSizeInternal(size + 1);
    }
}

template<typename T>
void CExoArrayList<T>::AddUnique(const T& value) {
    if (Count(value) == 0) {
        Add(value);
    }
}

template<typename T>
void CExoArrayList<T>::Allocate(int newCapacity) {
    if (!objectPtr) return;

    T* oldData = GetData();
    int currentSize = GetSize();

    // Allocate new array
    T* newData = nullptr;
    if (newCapacity > 0) {
        newData = static_cast<T*>(malloc(sizeof(T) * newCapacity));

        // Copy existing elements
        if (newData && oldData) {
            int copyCount = (currentSize < newCapacity) ? currentSize : newCapacity;
            memcpy(newData, oldData, sizeof(T) * copyCount);
        }
    }

    // Free old data
    if (oldData) {
        free(oldData);
    }

    // Update structure
    SetData(newData);
    SetCapacityInternal(newCapacity);

    // Adjust size if we shrank below it
    if (currentSize > newCapacity) {
        SetSizeInternal(newCapacity);
    }
}

template<typename T>
void CExoArrayList<T>::Clear() {
    if (!objectPtr) return;

    T* data = GetData();
    int size = GetSize();

    // Call destructors if needed
    if (data && !std::is_trivially_destructible<T>::value) {
        for (int i = 0; i < size; i++) {
            DestroyElement(&data[i]);
        }
    }

    // Free memory
    if (data) {
        free(data);
    }

    // Reset structure
    SetData(nullptr);
    SetSizeInternal(0);
    SetCapacityInternal(0);
}

template<typename T>
int CExoArrayList<T>::Count(const T& value) const {
    if (!objectPtr) return 0;

    T* data = GetData();
    int size = GetSize();
    int count = 0;
    T target = toStored(value);

    for (int i = 0; i < size; i++) {
        if (data[i] == target) {
            count++;
        }
    }

    return count;
}

template<typename T>
void CExoArrayList<T>::DeleteAt(int index) {
    if (!objectPtr) return;

    int size = GetSize();
    if (index < 0 || index >= size) return;

    T* data = GetData();

    // Call destructor if needed
    if (!std::is_trivially_destructible<T>::value) {
        DestroyElement(&data[index]);
    }

    // Shift elements back
    if (index < size - 1) {
        memmove(&data[index], &data[index + 1], sizeof(T) * (size - index - 1));
    }

    SetSizeInternal(size - 1);
}

template<typename T>
int CExoArrayList<T>::IndexOf(const T& value) const {
    if (!objectPtr) return -1;

    T* data = GetData();
    int size = GetSize();
    T target = toStored(value);

    for (int i = 0; i < size; i++) {
        if (data[i] == target) {
            return i;
        }
    }

    return -1;
}

template<typename T>
void CExoArrayList<T>::Insert(const T& value, int index) {
    if (!objectPtr) return;

    int size = GetSize();
    if (index < 0 || index > size) return;

    GrowIfNeeded();

    T* data = GetData();

    // Shift elements forward
    if (index < size) {
        memmove(&data[index + 1], &data[index], sizeof(T) * (size - index));
    }

    // Insert new value
    data[index] = toStored(value);
    SetSizeInternal(size + 1);
}

template<typename T>
void CExoArrayList<T>::Remove(const T& value) {
    if (!objectPtr) return;

    int size = GetSize();
    T target = toStored(value);

    // Find last occurrence
    for (int i = size - 1; i >= 0; i--) {
        T* data = GetData();
        if (data[i] == target) {
            DeleteAt(i);
            return;
        }
    }
}

template<typename T>
void CExoArrayList<T>::RemoveAll(const T& value) {
    if (!objectPtr) return;

    // Remove from back to front to avoid index shifting issues
    T target = toStored(value);
    for (int i = GetSize() - 1; i >= 0; i--) {
        T* data = GetData();
        if (data[i] == target) {
            DeleteAt(i);
        }
    }
}

template<typename T>
void CExoArrayList<T>::SetSize(int newSize) {
    if (!objectPtr) return;

    int currentSize = GetSize();
    int capacity = GetCapacity();

    if (newSize < 0) return;

    // If shrinking, call destructors on removed elements
    if (newSize < currentSize && !std::is_trivially_destructible<T>::value) {
        T* data = GetData();
        for (int i = newSize; i < currentSize; i++) {
            DestroyElement(&data[i]);
        }
    }

    // If size exceeds capacity, reallocate
    if (newSize > capacity) {
        Allocate(newSize);
    }

    SetSizeInternal(newSize);
}

template<typename T>
T* CExoArrayList<T>::GetData() const {
    if (!objectPtr) return nullptr;
    return getObjectProperty<T*>(objectPtr, 0x0);
}

template<typename T>
int CExoArrayList<T>::GetSize() const {
    if (!objectPtr) return 0;
    return getObjectProperty<int>(objectPtr, 0x4);
}

template<typename T>
int CExoArrayList<T>::GetCapacity() const {
    if (!objectPtr) return 0;
    return getObjectProperty<int>(objectPtr, 0x8);
}

template<typename T>
T CExoArrayList<T>::toStored(const T& value) {
    if constexpr (kIsWrapperPtr) {
        // value is a wrapper pointer (e.g. CSWGuiControl*); store the raw
        // game-object pointer it wraps.
        return reinterpret_cast<T>(value ? value->GetPtr() : nullptr);
    } else {
        return value;
    }
}

template<typename T>
void* CExoArrayList<T>::GetRaw(int index) const {
    T* data = GetData();
    if (!data || index < 0 || index >= GetSize()) return nullptr;
    if constexpr (std::is_pointer<T>::value) {
        // Pointer element: return the stored pointer value (the raw game pointer
        // for wrapper-pointer lists).
        return reinterpret_cast<void*>(data[index]);
    } else {
        // Value element: return the address of the inline slot.
        return &data[index];
    }
}

template<typename T>
template<typename Fn>
void CExoArrayList<T>::ForEach(Fn&& fn) {
    T* data = GetData();
    int size = GetSize();
    for (int i = 0; i < size; i++) {
        if constexpr (kIsWrapperPtr) {
            // Reconstruct a non-owning wrapper around the stored raw pointer.
            typename std::remove_pointer<T>::type wrapper(reinterpret_cast<void*>(data[i]));
            fn(wrapper);
        } else {
            fn(data[i]);
        }
    }
}

template<typename T>
T& CExoArrayList<T>::operator[](int index) {
    T* data = GetData();
    return data[index];
}

template<typename T>
const T& CExoArrayList<T>::operator[](int index) const {
    T* data = GetData();
    return data[index];
}

template<typename T>
CExoArrayList<T>& CExoArrayList<T>::operator=(const CExoArrayList<T>& rhs) {
    if (this == &rhs) return *this;

    // Clear existing content
    Clear();

    // Copy elements
    int rhsSize = rhs.GetSize();
    if (rhsSize > 0) {
        Allocate(rhsSize);
        T* rhsData = rhs.GetData();
        T* thisData = GetData();

        if (rhsData && thisData) {
            memcpy(thisData, rhsData, sizeof(T) * rhsSize);
            SetSizeInternal(rhsSize);
        }
    }

    return *this;
}

template<typename T>
bool CExoArrayList<T>::operator==(const CExoArrayList<T>& rhs) const {
    int size = GetSize();
    if (size != rhs.GetSize()) return false;

    T* thisData = GetData();
    T* rhsData = rhs.GetData();

    for (int i = 0; i < size; i++) {
        if (thisData[i] != rhsData[i]) return false;
    }

    return true;
}

template<typename T>
bool CExoArrayList<T>::operator!=(const CExoArrayList<T>& rhs) const {
    return !(*this == rhs);
}

template<typename T>
void CExoArrayList<T>::InitializeFunctions() {
    if (functionsInitialized) return;

    // No game functions to initialize - we implement everything ourselves
    functionsInitialized = true;
}

template<typename T>
void CExoArrayList<T>::InitializeOffsets() {
    if (offsetsInitialized) return;

    // Offsets are fixed and known:
    // data @ 0x0, size @ 0x4, capacity @ 0x8
    offsetsInitialized = true;
}

template<typename T>
void CExoArrayList<T>::GrowIfNeeded() {
    if (!objectPtr) return;

    int size = GetSize();
    int capacity = GetCapacity();

    if (size >= capacity) {
        int newCapacity = (capacity == 0) ? 10 : capacity * 2;
        Allocate(newCapacity);
    }
}

template<typename T>
void CExoArrayList<T>::DestroyElement(T* element) {
    if (element && !std::is_trivially_destructible<T>::value) {
        element->~T();
    }
}

template<typename T>
void CExoArrayList<T>::SetData(T* data) {
    if (objectPtr) {
        setObjectProperty<T*>(objectPtr, 0x0, data);
    }
}

template<typename T>
void CExoArrayList<T>::SetSizeInternal(int size) {
    if (objectPtr) {
        setObjectProperty<int>(objectPtr, 0x4, size);
    }
}

template<typename T>
void CExoArrayList<T>::SetCapacityInternal(int capacity) {
    if (objectPtr) {
        setObjectProperty<int>(objectPtr, 0x8, capacity);
    }
}
