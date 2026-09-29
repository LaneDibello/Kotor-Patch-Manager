#include <windows.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <string>

namespace {

constexpr int SlotCount = 6;
constexpr int SlotOffset = 0x34;
constexpr int ModelArgumentOffset = 0x0C;
constexpr int TextureArgumentOffset = 0x10;
constexpr std::size_t ResRefLength = 16;

// Native KotOR II function addresses.
constexpr std::uintptr_t C2DAConstructorAddress = 0x006207E0;
constexpr std::uintptr_t C2DADestructorAddress = 0x00620920;
constexpr std::uintptr_t C2DALoadAddress = 0x00621E70;
constexpr std::uintptr_t C2DAGetStringAddress = 0x00620C70;
constexpr std::uintptr_t ExoStringDefaultConstructorAddress = 0x004EAED0;
constexpr std::uintptr_t ExoStringConstructorAddress = 0x00605680;
constexpr std::uintptr_t ExoStringDestructorAddress = 0x00605890;

struct ResRef {
    char value[ResRefLength];
};

struct ExoString {
    char* value;
    DWORD length;
};

// Native KotOR II function types.
using C2DAConstructor = void* (__thiscall*)(void*, ResRef, int);
using C2DADestructor = void (__thiscall*)(void*);
using C2DALoad = void (__thiscall*)(void*);
using C2DAGetString = bool (__thiscall*)(void*, int, ExoString*, ExoString*);
using ExoStringDefaultConstructor = void (__thiscall*)(ExoString*);
using ExoStringConstructor = void (__thiscall*)(ExoString*, char*);
using ExoStringDestructor = void (__thiscall*)(ExoString*);

// Native KotOR II function pointers.
const auto C2DAConstruct = reinterpret_cast<C2DAConstructor>(C2DAConstructorAddress);
const auto C2DADestroy = reinterpret_cast<C2DADestructor>(C2DADestructorAddress);
const auto C2DALoadTable = reinterpret_cast<C2DALoad>(C2DALoadAddress);
const auto C2DAReadString = reinterpret_cast<C2DAGetString>(C2DAGetStringAddress);
const auto ExoStringConstructDefault = reinterpret_cast<ExoStringDefaultConstructor>(
    ExoStringDefaultConstructorAddress);
const auto ExoStringConstruct = reinterpret_cast<ExoStringConstructor>(
    ExoStringConstructorAddress);
const auto ExoStringDestroy = reinterpret_cast<ExoStringDestructor>(
    ExoStringDestructorAddress);

struct ClassMenuEntry {
    bool valid = false;
    std::array<char, ResRefLength> model{};
    std::array<char, ResRefLength> texture{};
};

std::array<ClassMenuEntry, SlotCount> g_entries;
ClassMenuEntry* g_activeEntry = nullptr;

// Copy and normalize a resource name.
void CopyResRef(std::array<char, ResRefLength>& output, const std::string& value)
{
    output.fill('\0');
    for (std::size_t index = 0; index < value.size(); ++index) {
        const unsigned char character = static_cast<unsigned char>(value[index]);
        output[index] = character >= 'A' && character <= 'Z'
            ? static_cast<char>(character + ('a' - 'A'))
            : static_cast<char>(character);
    }
}

// Load all six previews from classmenu.2da.
void LoadClassMenu2DA()
{
    g_entries = {};

    alignas(4) std::array<unsigned char, 0x54> table{};
    ResRef tableName{};
    std::memcpy(tableName.value, "classmenu", 9);
    C2DAConstruct(table.data(), tableName, 0);
    C2DALoadTable(table.data());

    char modelName[] = "Model";
    char textureName[] = "Texture";
    ExoString modelColumn{};
    ExoString textureColumn{};
    ExoStringConstruct(&modelColumn, modelName);
    ExoStringConstruct(&textureColumn, textureName);

    for (int slot = 0; slot < SlotCount; ++slot) {
        ExoString modelValue{};
        ExoString textureValue{};
        ExoStringConstructDefault(&modelValue);
        ExoStringConstructDefault(&textureValue);
        const bool haveValues =
            C2DAReadString(table.data(), slot, &modelColumn, &modelValue) &&
            C2DAReadString(table.data(), slot, &textureColumn, &textureValue);

        if (haveValues && modelValue.value && textureValue.value) {
            const std::string model(modelValue.value, modelValue.length);
            const std::string texture(textureValue.value, textureValue.length);
            if (!model.empty() && model.size() <= ResRefLength &&
                !texture.empty() && texture.size() <= ResRefLength) {
                ClassMenuEntry& entry = g_entries[slot];
                CopyResRef(entry.model, model);
                CopyResRef(entry.texture, texture);
                entry.valid = true;
            }
        }

        ExoStringDestroy(&textureValue);
        ExoStringDestroy(&modelValue);
    }

    ExoStringDestroy(&textureColumn);
    ExoStringDestroy(&modelColumn);
    C2DADestroy(table.data());
}

// Replace one resolver output value.
void OverrideOutput(void* resolverFrame, int argumentOffset,
                    const std::array<char, ResRefLength>& value)
{
    if (!g_activeEntry || !resolverFrame) {
        return;
    }

    auto** argument = reinterpret_cast<void**>(
        static_cast<unsigned char*>(resolverFrame) + argumentOffset);
    if (!*argument) {
        return;
    }

    std::memcpy(*argument, value.data(), value.size());
}

} // namespace

// Select the current class preview row.
extern "C" void __cdecl BeginClassMenuAppearance(void* classSelectionFrame)
{
    g_activeEntry = nullptr;
    if (!classSelectionFrame) {
        return;
    }

    const int slot = *reinterpret_cast<int*>(
        static_cast<unsigned char*>(classSelectionFrame) - SlotOffset);
    if (slot == 0) {
        LoadClassMenu2DA();
    }
    if (slot >= 0 && slot < SlotCount && g_entries[slot].valid) {
        g_activeEntry = &g_entries[slot];
    }
}

// Replace the current preview model.
extern "C" void __cdecl OverrideClassMenuModel(void* resolverFrame)
{
    if (g_activeEntry) {
        OverrideOutput(resolverFrame, ModelArgumentOffset, g_activeEntry->model);
    }
}

// Replace the current preview texture.
extern "C" void __cdecl OverrideClassMenuTexture(void* resolverFrame)
{
    if (g_activeEntry) {
        OverrideOutput(resolverFrame, TextureArgumentOffset, g_activeEntry->texture);
    }
}

// Stop overriding resolver output.
extern "C" void __cdecl EndClassMenuAppearance()
{
    g_activeEntry = nullptr;
}

// Clear the active row when the patch unloads.
BOOL WINAPI DllMain(HINSTANCE, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_DETACH) {
        g_activeEntry = nullptr;
    }
    return TRUE;
}
