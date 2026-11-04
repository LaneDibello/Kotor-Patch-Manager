// K1 Marked Empty Containers
//
// CSWGuiTargetActionMenu::UpdateNameLabel assembles the hovered object's name
// into a temporary CExoString and passes it to SetNameLabel.
//
// MarkEmptyContainer runs immediately before SetNameLabel's argument setup:
// if the hovered server object is a CSWSPlaceable with an inventory
// (m_bHasInventory != 0) that currently holds zero items (GetItemCount(0) == 0),
// it appends " (empty)" to the temporary CExoString. The container's actual
// name, the loot-window title, and scripting remain untouched.

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#if !defined(_WIN32)
// create-patch.py finds exported hooks by this spelling.
#define __cdecl
#endif

namespace {

// CExoString stores its char* pointer at offset 0.
const char* TextOf(const unsigned char* exoString)
{
    const char* text = nullptr;
    std::memcpy(&text, exoString, sizeof(text));
    return text;
}

// Reads the vtable pointer at offset 0 of a polymorphic object.
std::uintptr_t VtableOf(const unsigned char* object)
{
    std::uintptr_t vtable = 0;
    std::memcpy(&vtable, object, sizeof(vtable));
    return vtable;
}

#if defined(_WIN32)

// Windows 1.03 (32-bit x86, MSVC __thiscall)
constexpr std::uintptr_t kPlaceableVtable = 0x007494D0;       // CSWSPlaceable vtable
constexpr std::size_t kHasInventoryOffset = 0x324;            // CSWSPlaceable::m_bHasInventory
constexpr std::uintptr_t kGetItemCountAddress = 0x00587710;   // CSWSPlaceable::GetItemCount(int)
constexpr std::uintptr_t kExoStringAssignAddress = 0x005E5140; // CExoString::operator=(char const*)

int CallGetItemCount(void* placeable, int excludePlot)
{
    using GetItemCountFn = int(__thiscall*)(void*, int);
    return reinterpret_cast<GetItemCountFn>(kGetItemCountAddress)(placeable, excludePlot);
}

void AssignExoString(void* exoString, const char* text)
{
    using AssignFn = void*(__thiscall*)(void*, const char*);
    reinterpret_cast<AssignFn>(kExoStringAssignAddress)(exoString, text);
}

#elif defined(__APPLE__)

// macOS Aspyr (64-bit x86_64, System V AMD64 ABI)
constexpr std::uintptr_t kPlaceableVtable = 0x1005B2390;       // CSWSPlaceable vtable
constexpr std::size_t kHasInventoryOffset = 0x490;             // CSWSPlaceable::m_bHasInventory
constexpr std::uintptr_t kGetItemCountAddress = 0x100416E98;   // CSWSPlaceable::GetItemCount(int)
constexpr std::uintptr_t kExoStringAssignAddress = 0x10034CED4; // CExoString::operator=(char const*)

int CallGetItemCount(void* placeable, int excludePlot)
{
    using GetItemCountFn = int(*)(void*, int);
    return reinterpret_cast<GetItemCountFn>(kGetItemCountAddress)(placeable, excludePlot);
}

void AssignExoString(void* exoString, const char* text)
{
    using AssignFn = void*(*)(void*, const char*);
    reinterpret_cast<AssignFn>(kExoStringAssignAddress)(exoString, text);
}

#endif

} // namespace

extern "C" void __cdecl MarkEmptyContainer(unsigned char* exoName,
                                           unsigned char* serverObject)
{
    if (exoName == nullptr) {
        return;
    }

    // If no object is hovered, UpdateNameLabel jumps directly to the SetNameLabel
    // call site with exoName's buffer still nullptr.
    const char* currentName = TextOf(exoName);
    if (currentName == nullptr || currentName[0] == '\0' || serverObject == nullptr) {
        return;
    }

    // Verify that the hovered server object is a CSWSPlaceable.
    if (VtableOf(serverObject) != kPlaceableVtable) {
        return;
    }

    // Verify that the placeable is a container (m_bHasInventory != 0).
    std::int32_t hasInventory = 0;
    std::memcpy(&hasInventory, serverObject + kHasInventoryOffset, sizeof(hasInventory));
    if (hasInventory == 0) {
        return;
    }

    // Count all items (0 = include plot items). Leave non-empty containers alone.
    if (CallGetItemCount(serverObject, 0) != 0) {
        return;
    }

    char labeled[256];
    std::snprintf(labeled, sizeof(labeled), "%s (empty)", currentName);
    AssignExoString(exoName, labeled);
}
