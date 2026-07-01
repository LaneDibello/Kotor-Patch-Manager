// K1SecurityDemolitionsXP.cpp
// Source-only KotorPatchManager detour patch for KOTOR1 GOG 1.03.

#include <cstdint>
#include <cstddef>

namespace {

constexpr std::uintptr_t APP_MANAGER = 0x007A39FC;
constexpr std::uintptr_t GetCreatureById = 0x004AE770;
constexpr std::uintptr_t GetPlayerCreatureId = 0x004AEA40;
constexpr std::uintptr_t GetPartyTable = 0x004AEE70;
constexpr std::uintptr_t PartyHasObjectId = 0x00563740;
constexpr std::uintptr_t DistributeExperience = 0x005653A0;
constexpr std::uintptr_t GetLevel = 0x005A5FD0;

constexpr std::uint32_t INVALID_OBJECT_ID = 0x7F000000u;

using GetCreatureByIdFn = void* (__thiscall *)(void*, std::uint32_t);
using GetPlayerIdFn = std::uint32_t (__thiscall *)(void*);
using GetPartyTableFn = void* (__thiscall *)(void*);
using PartyHasObjectIdFn = int (__thiscall *)(void*, std::uint32_t);
using DistributeXPFn = void (__thiscall *)(void*, int, int);
using GetLevelFn = int (__thiscall *)(void*, int);
using GetCreatureFromObjectFn = void* (__thiscall *)(void*);

void* Ptr(void* base, std::ptrdiff_t offset)
{
    return base ? *reinterpret_cast<void**>(reinterpret_cast<std::uintptr_t>(base) + offset) : nullptr;
}

std::uint32_t U32(void* base, std::ptrdiff_t offset)
{
    return base ? *reinterpret_cast<std::uint32_t*>(reinterpret_cast<std::uintptr_t>(base) + offset) : 0u;
}

bool ValidObjectId(std::uint32_t id)
{
    return id != 0u && id != INVALID_OBJECT_ID && id != 0xFFFFFFFFu;
}

void* Server()
{
    void* app = *reinterpret_cast<void**>(APP_MANAGER);
    return Ptr(app, 0x08);
}

void* Party()
{
    void* server = Server();
    return server ? reinterpret_cast<GetPartyTableFn>(GetPartyTable)(server) : nullptr;
}

std::uint32_t MainPlayerId()
{
    void* server = Server();
    return server ? reinterpret_cast<GetPlayerIdFn>(GetPlayerCreatureId)(server) : INVALID_OBJECT_ID;
}

void* CreatureById(std::uint32_t id)
{
    void* server = Server();
    return server && ValidObjectId(id)
        ? reinterpret_cast<GetCreatureByIdFn>(GetCreatureById)(server, id)
        : nullptr;
}

void* CreatureFromObject(void* object)
{
    if (!object) {
        return nullptr;
    }

    void** vtable = *reinterpret_cast<void***>(object);
    void* fn = vtable ? vtable[0x30 / 4] : nullptr;
    return fn ? reinterpret_cast<GetCreatureFromObjectFn>(fn)(object) : nullptr;
}

int MainPlayerLevel()
{
    void* creature = CreatureById(MainPlayerId());
    void* stats = Ptr(creature, 0x0A74);
    if (!stats) {
        return 0;
    }

    int level = reinterpret_cast<GetLevelFn>(GetLevel)(stats, 0) & 0xFF;
    return level > 0 ? level : 0;
}

bool IsPartyCreature(void* creature)
{
    std::uint32_t id = U32(creature, 0x04);
    if (!ValidObjectId(id)) {
        return false;
    }
    if (id == MainPlayerId()) {
        return true;
    }

    void* party = Party();
    return party && reinterpret_cast<PartyHasObjectIdFn>(PartyHasObjectId)(party, id) != 0;
}

void AwardXP(void* creature, int dc, int lowMultiplier, int highMultiplier)
{
    if (!IsPartyCreature(creature)) {
        return;
    }

    int level = MainPlayerLevel();
    if (level <= 0) {
        return;
    }

    if (dc < 1) {
        dc = 1;
    }

    int multiplier = (dc >= level + 20) ? highMultiplier : lowMultiplier;
    void* party = Party();
    if (party) {
        reinterpret_cast<DistributeXPFn>(DistributeExperience)(party, level * multiplier, 1);
    }
}

} // namespace

extern "C" void __cdecl AwardSecurityUnlockXP(void* object, int openLockDc)
{
    AwardXP(CreatureFromObject(object), openLockDc, 5, 10);
}

extern "C" void __cdecl AwardMineDisarmXP(void* creature, int disarmDc, int checkedSuccess)
{
    if (checkedSuccess > 0) {
        AwardXP(creature, disarmDc, 10, 15);
    }
}

extern "C" void __cdecl AwardMineRecoverXP(void* creature, int recoverDc, int noCheckRecovery)
{
    if (noCheckRecovery == 0) {
        AwardXP(creature, recoverDc, 10, 15);
    }
}
