#include <stdint.h>

namespace {

constexpr unsigned BankColumns = 15;
constexpr unsigned MaxModelName = 31;
constexpr unsigned MaxResRef = 16;

constexpr uintptr_t FindModelVA = 0x008CE6B0u;
constexpr uintptr_t AddRefVA = 0x00845350u;
constexpr uintptr_t StricmpVA = 0x0093E852u;
constexpr uintptr_t OperatorNewVA = 0x00921347u;

constexpr uintptr_t C2DACtorVA = 0x006207E0u;
constexpr uintptr_t C2DADtorVA = 0x00620920u;
constexpr uintptr_t C2DALoadVA = 0x00621E70u;
constexpr uintptr_t C2DAGetStringVA = 0x00620A40u;
constexpr uintptr_t CExoStringDtorVA = 0x00605890u;

constexpr unsigned GobPrimary = 0x84;
constexpr unsigned TreeName = 0x08;
constexpr unsigned ModelAnimations = 0x58;
constexpr unsigned ModelAnimationCount = 0x5C;
constexpr unsigned AnimationOwner = 0x84;
constexpr unsigned TwoDARowCount = 0x24;

struct CResRef {
    unsigned char value[16];
};

struct CExoString {
    char* text;
    uint32_t length;
};

struct C2DA {
    unsigned char value[0x54];
};

struct Bank {
    void* model;
    char name[MaxResRef + 1];
    bool attempted;
};

struct Mapping {
    char target[MaxModelName + 1];
    unsigned char bank_count;
    Bank banks[BankColumns];
};

template<class T>
T field(const void* object, unsigned offset) {
    return *reinterpret_cast<const T*>(
        static_cast<const unsigned char*>(object) + offset);
}

using FindModelFn = void* (__cdecl*)(const char*);
using AddRefFn = int (__thiscall*)(void*, void*);
using StricmpFn = int (__cdecl*)(const char*, const char*);
using OperatorNewFn = void* (__cdecl*)(unsigned int);
using C2DACtorFn = C2DA* (__thiscall*)(C2DA*, CResRef, int);
using C2DADtorFn = void (__thiscall*)(C2DA*);
using C2DALoadFn = int (__thiscall*)(C2DA*);
using C2DAGetStringFn = int (__thiscall*)(C2DA*, int, int, CExoString*);
using CExoStringDtorFn = void (__thiscall*)(CExoString*);

bool same_ci(const char* left, const char* right) {
    return reinterpret_cast<StricmpFn>(StricmpVA)(left, right) == 0;
}

void copy_text(char* output, const char* input) {
    while ((*output++ = *input++)) {}
}

bool unused_cell(const char* text) {
    return !text[0] || text[0] == '*';
}

void* primary_model(void* gob) {
    return field<void*>(gob, GobPrimary);
}

const char* model_name(void* model) {
    return static_cast<const char*>(model) + TreeName;
}

void* direct_animation(void* model, const char* name) {
    void** animations = field<void**>(model, ModelAnimations);
    const int count = field<int>(model, ModelAnimationCount);
    for (int i = 0; i < count; ++i) {
        void* animation = animations[i];
        if (same_ci(static_cast<const char*>(animation) + TreeName, name)) {
            return animation;
        }
    }
    return nullptr;
}

Mapping* mappings;
unsigned mapping_count;
bool initialized;

const char* cell_text(C2DA& table, CExoString& value, int row, int column) {
    reinterpret_cast<C2DAGetStringFn>(C2DAGetStringVA)(
        &table, row, column, &value);
    return value.text;
}

void load_mappings() {
    const CResRef resref = {{
        's', 't', 'u', 'n', 't', 'm', 'o', 'd',
        'e', 'l', 0, 0, 0, 0, 0, 0
    }};

    C2DA table;
    reinterpret_cast<C2DACtorFn>(C2DACtorVA)(&table, resref, 0);
    if (!reinterpret_cast<C2DALoadFn>(C2DALoadVA)(&table)) {
        reinterpret_cast<C2DADtorFn>(C2DADtorVA)(&table);
        return;
    }

    const int row_count = field<int>(&table, TwoDARowCount);
    mappings = static_cast<Mapping*>(
        reinterpret_cast<OperatorNewFn>(OperatorNewVA)(
            uint32_t(row_count) * uint32_t(sizeof(Mapping))));

    CExoString value = {};

    for (int row = 0; row < row_count; ++row) {
        const char* target = cell_text(table, value, row, 0);
        if (unused_cell(target)) continue;

        Mapping& mapping = mappings[mapping_count];
        copy_text(mapping.target, target);
        mapping.bank_count = 0;

        for (unsigned column = 1; column <= BankColumns; ++column) {
            const char* bank_name = cell_text(
                table, value, row, int(column));
            if (unused_cell(bank_name)) continue;

            Bank& bank = mapping.banks[mapping.bank_count++];
            bank.model = nullptr;
            bank.attempted = false;
            copy_text(bank.name, bank_name);
        }

        if (mapping.bank_count) ++mapping_count;
    }

    reinterpret_cast<CExoStringDtorFn>(CExoStringDtorVA)(&value);
    reinterpret_cast<C2DADtorFn>(C2DADtorVA)(&table);
}

void ensure_initialized() {
    if (initialized) return;
    initialized = true;
    load_mappings();
}

void* load_bank(Bank& bank) {
    if (bank.attempted) return bank.model;
    bank.attempted = true;

    bank.model = reinterpret_cast<FindModelFn>(FindModelVA)(bank.name);
    if (bank.model) {
        reinterpret_cast<AddRefFn>(AddRefVA)(bank.model, nullptr);
    }
    return bank.model;
}

Mapping* find_mapping(void* base) {
    const char* base_name = model_name(base);
    for (unsigned i = 0; i < mapping_count; ++i) {
        if (same_ci(base_name, mappings[i].target)) return &mappings[i];
    }
    return nullptr;
}

void* choose_sidecar(void* base, const char* name) {
    Mapping* mapping = find_mapping(base);
    if (!mapping) return nullptr;

    for (unsigned i = 0; i < mapping->bank_count; ++i) {
        void* bank = load_bank(mapping->banks[i]);
        if (!bank) continue;

        void* animation = direct_animation(bank, name);
        if (animation) return animation;
    }
    return nullptr;
}

void* sidecar_owner(void* animation) {
    void* owner = field<void*>(animation, AnimationOwner);
    for (unsigned row = 0; row < mapping_count; ++row) {
        Mapping& mapping = mappings[row];
        for (unsigned bank = 0; bank < mapping.bank_count; ++bank) {
            if (mapping.banks[bank].model == owner) return owner;
        }
    }
    return nullptr;
}

void* resolve_new(void* gob, const char* name) {
    return choose_sidecar(primary_model(gob), name);
}

} // namespace

extern "C" {

__declspec(dllexport) void __cdecl KAB_PlaybackSelect(
    void* gob, const char* name, void** animation, void** source_model,
    void* run) {
    ensure_initialized();

    void* selected = run ? *static_cast<void**>(run) : nullptr;
    void* owner = nullptr;
    if (selected) {
        owner = sidecar_owner(selected);
        if (!owner) return;
    } else {
        selected = resolve_new(gob, name);
        if (!selected) return;
        owner = field<void*>(selected, AnimationOwner);
    }

    *animation = selected;
    *source_model = owner;
}

__declspec(dllexport) void* __cdecl KAB_QuerySelect(
    void* gob, const char* name, void* native_animation,
    void** native_result) {
    ensure_initialized();

    void* result = resolve_new(gob, name);
    if (!result) result = native_animation;
    *native_result = result;
    return result;
}

__declspec(dllexport) void* __cdecl KAB_OutOfOrderSelect(
    void* gob, const char* name, void* native_animation) {
    ensure_initialized();

    void* result = resolve_new(gob, name);
    return result ? result : native_animation;
}

} // extern "C"
