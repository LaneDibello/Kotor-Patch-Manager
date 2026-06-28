#pragma once
#include <cstdlib>
#include <cstring>

// ===== RUNTIME VTABLE OVERRIDE =====
//
// Most KotOR menus derive from the game's CSWGuiPanel and customize behavior by
// overriding panel virtual functions (HandleInputEvent, Draw, ...). Our wrappers
// are built around real game objects at runtime, so the object still carries the
// *game's* vtable -- overriding C++ virtuals on our wrapper does nothing, because
// the game dispatches through the game object's vtable.
//
// VTableOverride solves this per-instance: it copies the game object's vtable into
// a buffer we own, lets callers replace individual slots with their own thunks, and
// repoints the object's vtable pointer at our copy. The original is kept so thunks
// can call through to it, and so the original is restored on destruction (before the
// game's own destructor runs against the object).
//
// THE BACK-POINTER:
//   When the game invokes an overridden virtual, ECX/`this` is the game object
//   pointer -- NOT our wrapper. To recover the wrapper, the copied buffer reserves
//   one extra slot *before* the vtable to hold an owner pointer:
//
//       buffer:   [ owner ][ slot0 ][ slot1 ] ... [ slotN-1 ]
//                          ^
//                          object's vtable pointer points here (&buffer[1])
//
//   so a thunk recovers the owner via GetOwner(gameObj) == (*(void***)gameObj)[-1].
//
// MEMORY: We only write the object's vtable *pointer* (writable heap data) and read
// our buffer as data -- we never execute the buffer itself -- so no VirtualProtect /
// FlushInstructionCache is needed (unlike code patching in trampoline.cpp).
//
// LIFETIME: The owner (wrapper) must outlive the game object's use of the overridden
// vtable. Construct/destroy this alongside the wrapper that owns the game object.
class VTableOverride {
public:
    // Copies gameObj's current vtable (slotCount entries), stores `owner` as the
    // recoverable back-pointer, and installs the copy on gameObj.
    VTableOverride(void* gameObj, void* owner, int slotCount)
        : gameObj(gameObj), originalVtable(nullptr), buffer(nullptr), slotCount(slotCount)
    {
        if (!gameObj || slotCount <= 0) {
            return;
        }

        originalVtable = *reinterpret_cast<void***>(gameObj);
        if (!originalVtable) {
            return;
        }

        // [owner][slot0..slotN-1]
        buffer = static_cast<void**>(malloc(sizeof(void*) * (slotCount + 1)));
        if (!buffer) {
            return;
        }

        buffer[0] = owner;
        memcpy(&buffer[1], originalVtable, sizeof(void*) * slotCount);

        *reinterpret_cast<void***>(gameObj) = &buffer[1];
    }

    // Restores the game object's original vtable and frees our copy.
    ~VTableOverride() {
        if (gameObj && originalVtable) {
            *reinterpret_cast<void***>(gameObj) = originalVtable;
        }
        if (buffer) {
            free(buffer);
            buffer = nullptr;
        }
    }

    // True if the override was installed successfully.
    bool IsActive() const { return buffer != nullptr; }

    // Replaces the function at `slot` with `customFn`.
    void Override(int slot, void* customFn) {
        if (!buffer || slot < 0 || slot >= slotCount) {
            return;
        }
        buffer[1 + slot] = customFn;
    }

    // Returns the original (game) function at `slot`, for call-through.
    void* GetOriginal(int slot) const {
        if (!originalVtable || slot < 0 || slot >= slotCount) {
            return nullptr;
        }
        return originalVtable[slot];
    }

    // Recovers the owner pointer stored alongside gameObj's overridden vtable.
    // Returns nullptr if gameObj does not carry a VTableOverride-installed vtable
    // (caller is responsible for only invoking this on objects it overrode).
    static void* GetOwner(void* gameObj) {
        if (!gameObj) {
            return nullptr;
        }
        void** vtable = *reinterpret_cast<void***>(gameObj);
        if (!vtable) {
            return nullptr;
        }
        return vtable[-1];
    }

private:
    void* gameObj;
    void** originalVtable;
    void** buffer;
    int slotCount;

    // Non-copyable: it owns a heap buffer and a live pointer-swap on the game object.
    VTableOverride(const VTableOverride&) = delete;
    VTableOverride& operator=(const VTableOverride&) = delete;
};
