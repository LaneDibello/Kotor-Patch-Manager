#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "Common.h"
#include "GameAPI/CResRef.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace
{
    constexpr int STOCK_NPC_SLOTS = 12;
    constexpr int MAX_NPC_SLOTS = 32;
    constexpr int EXTRA_NPC_SLOTS = MAX_NPC_SLOTS - STOCK_NPC_SLOTS;

    constexpr int OFFSET_PARTY_MEMBER_COUNT = 0x0;
    constexpr int OFFSET_PARTY_MEMBER_SLOTS = 0x8;

    constexpr int OFFSET_NPC_OBJECT_IDS = 0x1C;
    constexpr int OFFSET_NPC_AVAILABLE = 0x4C;
    constexpr int OFFSET_NPC_SELECTABLE = 0x7C;
    constexpr int OFFSET_PARTY_LEADER_SLOT = 0x114;
    constexpr int OFFSET_PLAYER_CHARACTER_SLOT = 0x118;
    constexpr int OFFSET_NEXT_PC_CURSOR = 0x254;

    constexpr int OFFSET_EQUIP_CURRENT_LOGICAL_SLOT = 0x50EC;

    constexpr DWORD NPC_OBJECT_SENTINEL = 0x7F000000u;
    constexpr int DEFAULT_INFLUENCE = -1;

    constexpr DWORD ADDRESS_SWKOTOR2_APP = 0x00A11C04u;
    constexpr DWORD ADDRESS_2DA_ROOT = 0x00A11C30u;
    constexpr int APP_MANAGER_CLIENT_OFFSET = 0x04;
    constexpr int APP_MANAGER_SERVER_OFFSET = 0x08;
    constexpr int OFFSET_2DA_COLLECTION = 0x130;
    constexpr int OFFSET_PARTY_NPC_2DA = 0xF4;

    constexpr DWORD ADDRESS_GET_NPC_OBJECT = 0x00700660u;
    constexpr DWORD ADDRESS_ADD_GAME_IN_PROGRESS = 0x00703650u;
    constexpr DWORD ADDRESS_REMOVE_GAME_IN_PROGRESS = 0x007036E0u;

    constexpr DWORD ADDRESS_CLIENT_GET_CREATURE_BY_OBJECT_ID = 0x0040CA10u;

    constexpr DWORD ADDRESS_GFF_GET_LIST = 0x006248C0u;
    constexpr DWORD ADDRESS_GFF_GET_LIST_COUNT = 0x00624970u;
    constexpr DWORD ADDRESS_GFF_GET_LIST_ELEMENT = 0x006249F0u;
    constexpr DWORD ADDRESS_GFF_ADD_LIST_ELEMENT = 0x00625960u;
    constexpr DWORD ADDRESS_GFF_READ_FIELD_BYTE = 0x00624B60u;
    constexpr DWORD ADDRESS_GFF_READ_FIELD_INT = 0x00624DE0u;
    constexpr DWORD ADDRESS_GFF_WRITE_FIELD_BYTE = 0x00625B30u;
    constexpr DWORD ADDRESS_GFF_WRITE_FIELD_INT = 0x00625D50u;
    constexpr DWORD ADDRESS_2DA_GET_INT_ENTRY_BY_ROW_LABEL = 0x00621790u;

    constexpr int PRIMARY_MEMBER_CAPACITY = 2;
    constexpr int PARTY_SELECT_PAGE_SLOTS = 16;
    constexpr int LAST_PARTY_SELECT_PAGE_BASE =
        MAX_NPC_SLOTS - PARTY_SELECT_PAGE_SLOTS;
    constexpr int PARTY_SELECTION_VIEW_RECORD_COUNT = PARTY_SELECT_PAGE_SLOTS;
    constexpr int PARTY_SELECTION_PAGE_CONTROL_COUNT = 2;
    constexpr int PARTY_SELECTION_STATE_CAPACITY = 2;
    constexpr int PARTY_SELECT_SLOT_STRIDE = 0x478;
    constexpr int PANEL_PARTY_SELECTION_DATA_BASE_OFFSET = 0x84;
    constexpr int GUI_BUTTON_OBJECT_SIZE = 0x1D0;

    constexpr int DATA_BUTTON_OFFSET = 0x04;
    constexpr int DATA_BUTTON_BACK_POINTER_OFFSET = 0x60;
    constexpr int DATA_BUTTON_SELECTED_OFFSET = 0x1D4;
    constexpr int DATA_BUTTON_INIT_PARAMS_OFFSET = 0x100;
    constexpr int DATA_DEFAULT_COLOR_OFFSET = 0x114;
    constexpr int DATA_NOT_AVAILABLE_LABEL_OFFSET = 0x1D8;
    constexpr int DATA_CHARACTER_LABEL_OFFSET = 0x320;
    constexpr int DATA_CHARACTER_TEXTURE_PARAMS_OFFSET = 0x394;
    constexpr int DATA_ALPHA_OFFSET = 0x3A4;
    constexpr int DATA_FLAGS_OFFSET = 0x468;
    constexpr int DATA_OBJECT_INDEX_OFFSET = 0x46C;
    constexpr int DATA_LOGICAL_SLOT_OFFSET = 0x470;
    constexpr int DATA_AUXILIARY_OFFSET = 0x474;
    constexpr std::array<int, 3> PARTY_SELECTION_RECORD_CONTROL_OFFSETS = {
        DATA_CHARACTER_LABEL_OFFSET,
        DATA_BUTTON_OFFSET,
        DATA_NOT_AVAILABLE_LABEL_OFFSET,
    };
    constexpr DWORD PANEL_SLOT_ENABLED_FLAG = 0x1u;
    constexpr DWORD PANEL_SLOT_HAS_OBJECT_FLAG = 0x2u;
    constexpr DWORD PANEL_SLOT_FORCED_FLAG = 0x4u;
    constexpr int PANEL_SELECTED_COUNT_OFFSET = 0x6C;
    constexpr int PANEL_FORCED_MODE_OFFSET = 0x70;
    constexpr int PANEL_MODE_OFFSET = 0x74;
    constexpr int PANEL_DONE_BUTTON_OFFSET = 0x376C;
    constexpr int PANEL_CURRENT_PORTRAIT_OFFSET = 0x48F8;
    constexpr int GUI_NAVIGATION_UP = 0;
    constexpr std::size_t GUI_BUTTON_SET_ACTIVE_VTABLE_INDEX =
        0x40 / sizeof(void*);
    constexpr int GUI_CONTROL_FLAGS_OFFSET = 0x48;
    constexpr DWORD GUI_CONTROL_VISIBLE_FLAG = 0x2u;
    constexpr DWORD GUI_CONTROL_MOUSE_IGNORE_FLAG = 0x20u;
    constexpr int GUI_CONTROL_PRIMARY_EVENT = 0x27;
    constexpr int AVAILABILITY_COUNT_CONSUMED_RESULT_FLAG = 0x100;
    constexpr int SELECTABILITY_CONSUMED_RESULT_FLAG = 0x100;

    constexpr int PARTY_SELECTION_PAGE_PREV_INDEX = 0;
    constexpr int PARTY_SELECTION_PAGE_NEXT_INDEX = 1;
    constexpr DWORD ADDRESS_GUI_PANEL_STOP_LOAD_FROM_LAYOUT = 0x0090DA20u;
    constexpr DWORD ADDRESS_PARTY_SELECTION_BUTTON_SET_SELECTED = 0x0058A930u;
    constexpr DWORD ADDRESS_PARTY_SELECTION_RECORD_CTOR = 0x0058B5D0u;
    constexpr DWORD ADDRESS_PARTY_SELECTION_RECORD_DTOR = 0x0058B650u;
    constexpr DWORD ADDRESS_PARTY_SELECTION_ON_TOGGLED = 0x0058C660u;
    constexpr DWORD ADDRESS_PARTY_SELECTION_ON_ENTER = 0x0058CB00u;
    constexpr DWORD ADDRESS_PARTY_SELECTION_ON_EXIT = 0x0058D4C0u;
    constexpr DWORD ADDRESS_PARTY_SELECTION_UPDATE_COUNT = 0x0058D520u;
    constexpr DWORD ADDRESS_GUI_PANEL_INIT_CONTROL = 0x0090DAA0u;
    constexpr DWORD ADDRESS_GUI_PANEL_ADD_CONTROL = 0x0090E200u;
    constexpr DWORD ADDRESS_GUI_PANEL_SET_ACTIVE_CONTROL = 0x0090D080u;
    constexpr DWORD ADDRESS_EXO_ARRAY_LIST_POINTER_DEL_INDEX = 0x00903DF0u;
    constexpr int GUI_PANEL_CONTROL_ARRAY_OFFSET = 0x24;
    constexpr int GUI_PANEL_CONTROL_COUNT_OFFSET = 0x28;
    constexpr DWORD ADDRESS_GUI_CONTROL_ADD_EVENT = 0x00916D90u;
    constexpr DWORD ADDRESS_GUI_BUTTON_SET_ENABLED = 0x009184B0u;
    constexpr DWORD ADDRESS_GUI_SET_MOVE_TO_CONTROL = 0x00916990u;
    constexpr DWORD ADDRESS_GUI_BUTTON_CTOR = 0x00512A40u;
    constexpr DWORD ADDRESS_GUI_BUTTON_DTOR = 0x004CE630u;
    constexpr DWORD ADDRESS_GUI_BORDER_SET_FILL_IMAGE = 0x00912C10u;
    constexpr DWORD ADDRESS_GUI_BORDER_SET_PULSING_ALPHA = 0x00912DC0u;
    constexpr DWORD ADDRESS_OBJECT_GET_PORTRAIT = 0x00663840u;
    constexpr DWORD ADDRESS_CEXOSTRING_CTOR_CSTR = 0x00605680u;
    constexpr DWORD ADDRESS_CEXOSTRING_DTOR = 0x00605890u;
    constexpr DWORD ADDRESS_GET_CURRENT_PARTY_TABLE = 0x0064C2F0u;
    constexpr DWORD ADDRESS_OPERATOR_NEW = 0x00921347u;
    constexpr DWORD ADDRESS_CREATURE_CTOR = 0x0067AB60u;
    constexpr DWORD ADDRESS_CREATURE_LOAD_FROM_TEMPLATE = 0x0068B5A0u;
    constexpr DWORD ADDRESS_CREATURE_SAVE_CREATURE = 0x0068D030u;
    constexpr DWORD ADDRESS_CRESGFF_CTOR = 0x00622FF0u;
    constexpr DWORD ADDRESS_CRESGFF_CREATE_FILE = 0x00626530u;
    constexpr DWORD ADDRESS_CRESGFF_WRITE_FILE = 0x00626700u;
    constexpr DWORD ADDRESS_CREATURE_STATS_GET_LEVEL = 0x006E1280u;
    constexpr DWORD ADDRESS_CREATURE_RECEIVE_EXPERIENCE = 0x006A34D0u;
    constexpr DWORD ADDRESS_OBJECT_CLEAR_ALL_ACTIONS = 0x006A7D60u;
    constexpr DWORD ADDRESS_SERVER_GET_PLAYER_CREATURE_ID = 0x0064C310u;
    constexpr DWORD ADDRESS_CREATURE_GET_FACTION = 0x006DB8D0u;
    constexpr DWORD ADDRESS_FACTION_ADD_MEMBER = 0x007E4850u;
    constexpr DWORD ADDRESS_CREATURE_STATS_SET_MOVEMENT_RATE = 0x006F67A0u;
    constexpr DWORD ADDRESS_CREATURE_HANDLE_ALIGNMENT_INFLUENCE = 0x00683A70u;
    constexpr DWORD ADDRESS_SERVER_GET_AI_MASTER = 0x0064BCA0u;
    constexpr DWORD ADDRESS_AI_MASTER_SET_AI_LEVEL = 0x0064DB40u;
    constexpr DWORD ADDRESS_GAME_EFFECT_CTOR = 0x007345E0u;
    constexpr DWORD ADDRESS_OBJECT_APPLY_EFFECT = 0x006AAEF0u;
    constexpr DWORD ADDRESS_EFFECT_TYPE_WORD = 0x0099C95Cu;
    constexpr DWORD ADDRESS_EFFECT_SUBTYPE_WORD = 0x0099C8B8u;
    constexpr unsigned int CREATURE_OBJECT_SIZE = 0x1220u;
    constexpr unsigned int RESGFF_OBJECT_SIZE = 0xA0u;
    constexpr unsigned int GAME_EFFECT_OBJECT_SIZE = 0x8Cu;
    constexpr unsigned short SAVED_CREATURE_RESOURCE_TYPE = 0x07EBu;
    constexpr DWORD ADDRESS_GET_CREATURE_BY_OBJECT_ID = 0x0064BB20u;
    constexpr DWORD ADDRESS_TRANSFER_INVENTORY = 0x006FE4D0u;
    constexpr DWORD ADDRESS_SERVER_TO_CLIENT_OBJECT_ID = 0x0040D3A0u;

    constexpr int GAME_OBJECT_ID_OFFSET = 0x04;
    constexpr int CREATURE_POST_LOAD_STATE_OFFSET = 0xF4;
    constexpr int PARTY_TARGET_EXPERIENCE_OFFSET = 0x16C;
    constexpr int CREATURE_EXPERIENCE_ADJUSTMENT_OFFSET = 0x26C;
    constexpr int CREATURE_STATS_OFFSET = 0x1198;
    constexpr int GAME_EFFECT_TYPE_OFFSET = 0x08;
    constexpr int GAME_EFFECT_SUBTYPE_FLAGS_OFFSET = 0x0A;
    constexpr WORD GAME_EFFECT_SUBTYPE_MASK = 0x7u;
    constexpr std::size_t SCALAR_DELETING_DESTRUCTOR_VTABLE_INDEX = 0;
    constexpr std::size_t OBJECT_AS_SERVER_OBJECT_VTABLE_INDEX = 4;
    constexpr std::size_t OBJECT_STATUS_VTABLE_INDEX = 0x9C / sizeof(void*);

    constexpr DWORD ADDRESS_CLIENT_GET_PARTY = 0x0040D070u;
    constexpr DWORD ADDRESS_PARTY_GET_CHARACTER = 0x004789D0u;
    constexpr DWORD ADDRESS_PARTY_GET_INDEX = 0x00479810u;
    constexpr DWORD ADDRESS_CLIENT_CREATURE_SET_IN_PARTY = 0x00421750u;
    constexpr DWORD ADDRESS_CLIENT_CREATURE_GET_SERVER_CREATURE = 0x00429780u;
    constexpr DWORD ADDRESS_SERVER_CREATURE_SET_IN_PARTY = 0x006A2E10u;
    constexpr DWORD ADDRESS_PARTY_TABLE_REMOVE_MEMBER = 0x006FE670u;
    constexpr DWORD ADDRESS_PARTY_TABLE_SPAWN_NPC = 0x00702E30u;
    constexpr DWORD ADDRESS_PARTY_TABLE_ADD_MEMBER = 0x006FE8F0u;
    constexpr DWORD ADDRESS_PATHFIND_RESET_WAYPOINT_DATA = 0x00647D60u;
    constexpr DWORD ADDRESS_CREATURE_GET_VISIBLE_LIST_ELEMENT = 0x006DC370u;
    constexpr DWORD ADDRESS_CREATURE_ADD_TO_VISIBLE_LIST = 0x006DC020u;
    constexpr DWORD ADDRESS_PARTY_RECALCULATE_FOLLOW_POINT = 0x0047B350u;
    constexpr int OBJECT_POSITION_OFFSET = 0x94;
    constexpr int OBJECT_ORIENTATION_OFFSET = 0xA0;
    constexpr int CREATURE_PATHFIND_INFO_OFFSET = 0x380;

    struct PartySlotSnapshot
    {
        DWORD objectId = NPC_OBJECT_SENTINEL;
        DWORD available = 0;
        BYTE selectable = 1;
        int influence = DEFAULT_INFLUENCE;
    };

    struct GffStructRef
    {
        DWORD index;
    };

    struct GffListRef
    {
        GffStructRef resStruct;
        char label[16];
    };

    static_assert(sizeof(GffStructRef) == 4);
    static_assert(sizeof(GffListRef) == 20);

    struct PartySelectionViewState
    {
        void* owner;
        BYTE* records;
        BYTE* pageButtons;
        unsigned int constructedRecords;
        unsigned int constructedButtons;
        int pageBase;
        bool ready;
        bool selected[MAX_NPC_SLOTS];
        bool forced[MAX_NPC_SLOTS];
    };

    std::array<PartySlotSnapshot, EXTRA_NPC_SLOTS> gExtendedPartySlots{};
    PartySelectionViewState
        gPartySelectionViews[PARTY_SELECTION_STATE_CAPACITY] = {};

    using GetNPCObjectFn = DWORD(__thiscall*)(void*, int, int, int);
    using AddGameInProgressFn = void(__thiscall*)(void*);
    using RemoveGameInProgressFn = void(__thiscall*)(void*);
    using ClientGetCreatureByObjectIdFn = void* (__thiscall*)(void*, DWORD);
    using GffGetListFn = int(__thiscall*)(
        void*, GffListRef*, GffStructRef*, char*);
    using GffGetListCountFn = int(__thiscall*)(void*, GffListRef*);
    using GffGetListElementFn = int(__thiscall*)(
        void*, GffStructRef*, GffListRef*, DWORD);
    using GffAddListElementFn = int(__thiscall*)(
        void*, GffStructRef*, GffListRef*, DWORD);
    using GffReadFieldByteFn = BYTE(__thiscall*)(
        void*, GffStructRef*, char*, int*, BYTE);
    using GffReadFieldIntFn = int(__thiscall*)(
        void*, GffStructRef*, char*, int*, int);
    using GffWriteFieldByteFn = int(__thiscall*)(
        void*, GffStructRef*, BYTE, char*);
    using GffWriteFieldIntFn = int(__thiscall*)(
        void*, GffStructRef*, int, char*);
    using TwoDaGetIntEntryByRowLabelFn = bool(__thiscall*)(
        void*, int, const void*, int*);
    using GetCurrentPartyTableFn = void* (__thiscall*)(void*);
    using ScalarDeletingDestructorFn = void* (__thiscall*)(void*, unsigned int);
    using GuiPartySelectionButtonSetSelectedFn = void(__thiscall*)(void*, int);
    using GuiPanelInitControlFn = void(__thiscall*)(void*, void*, const void*, int, int);
    using GuiPanelAddControlFn = void(__thiscall*)(void*, void*);
    using GuiPanelSetActiveControlFn = void(__thiscall*)(void*, void*, int);
    using ExoArrayListPointerDelIndexFn = void(__thiscall*)(void*, int);
    using GuiControlAddEventFn = void(__thiscall*)(void*, int, void*, void*);
    using GuiButtonSetEnabledFn = void(__thiscall*)(void*, int);
    using GuiButtonSetActiveFn = void(__thiscall*)(void*, int);
    using GuiSetMoveToControlFn = void(__thiscall*)(void*, int, void*);
    using GuiButtonCtorFn = void* (__thiscall*)(void*);
    using GuiButtonDtorFn = void(__thiscall*)(void*);
    using CExoStringCtorCStrFn = void* (__thiscall*)(void*, const char*);
    using CExoStringDtorFn = void(__thiscall*)(void*);
    using OperatorNewFn = void* (__cdecl*)(unsigned int);
    using CreatureCtorFn = void* (__thiscall*)(void*, DWORD, int);
    using CreatureLoadFromTemplateFn = int(__thiscall*)(void*, const void*, int);
    using CreatureSaveCreatureFn = int(__thiscall*)(void*, void*, void*);
    using CResGffCtorFn = void* (__thiscall*)(void*);
    using CResGffCreateFileFn = int(__thiscall*)(void*, void*, const void*, const void*);
    using CResGffWriteFileFn = int(__thiscall*)(void*, const void*, unsigned short);
    using CreatureStatsGetLevelFn = BYTE(__thiscall*)(void*, int);
    using CreatureReceiveExperienceFn = void(__thiscall*)(void*, int);
    using ObjectClearAllActionsFn = void(__thiscall*)(void*, int);
    using ServerGetPlayerCreatureIdFn = DWORD(__thiscall*)(void*);
    using CreatureGetFactionFn = void* (__thiscall*)(void*);
    using FactionAddMemberFn = void(__thiscall*)(void*, DWORD, int);
    using CreatureStatsSetMovementRateFn = void(__thiscall*)(void*, int);
    using CreatureHandleAlignmentInfluenceFn = void(__thiscall*)(void*);
    using ServerGetAiMasterFn = void* (__thiscall*)(void*);
    using AiMasterSetAiLevelFn = int(__thiscall*)(void*, void*, int);
    using GameEffectCtorFn = void* (__thiscall*)(void*, int);
    using ObjectApplyEffectFn = void(__thiscall*)(void*, void*, int, int);
    using ObjectStatusVirtualFn = short(__thiscall*)(void*, int);
    using ObjectAsServerObjectVirtualFn = void* (__thiscall*)(void*);
    using GetCreatureByObjectIdFn = void* (__thiscall*)(void*, DWORD);
    using TransferInventoryFn = void(__thiscall*)(void*, void*);
    using ServerToClientObjectIdFn = DWORD(__thiscall*)(void*, DWORD);
    using GuiPanelStopLoadFromLayoutFn = void(__thiscall*)(void*);
    using PartySelectionRecordCtorFn = void* (__thiscall*)(void*);
    using PartySelectionRecordDtorFn = void(__thiscall*)(void*);
    using GuiBorderSetFillImageFn = void(__thiscall*)(void*, const void*, int);
    using GuiBorderSetPulsingAlphaFn = void(__thiscall*)(void*, int, int, int);
    using ObjectGetPortraitFn = void* (__thiscall*)(void*, void*);
    using PartySelectionUpdateCountFn = void(__thiscall*)(void*);
    using ClientGetPartyFn = void* (__thiscall*)(void*);
    using PartyGetCharacterFn = void* (__thiscall*)(void*, int);
    using PartyGetIndexFn = int(__thiscall*)(void*, DWORD);
    using ClientCreatureSetInPartyFn = void(__thiscall*)(void*, int);
    using ClientCreatureGetServerCreatureFn = void* (__thiscall*)(void*);
    using ServerCreatureSetInPartyFn = void(__thiscall*)(void*, int, int);
    using PartyTableRemoveMemberFn = int(__thiscall*)(void*, int);
    using PartyTableSpawnNpcFn = DWORD(__thiscall*)(
        void*, int, int, const void*, const void*, int);
    using PartyTableAddMemberFn = int(__thiscall*)(void*, int, DWORD);
    using PathfindResetWaypointDataFn = void(__thiscall*)(void*);
    using CreatureGetVisibleListElementFn = int(__thiscall*)(void*, DWORD);
    using CreatureAddToVisibleListFn = void(__thiscall*)(
        void*, DWORD, int, int, int, int);
    using PartyRecalculateFollowPointFn = void(__thiscall*)(void*);

    template <typename Function>
    Function GameFunction(DWORD address)
    {
        return reinterpret_cast<Function>(address);
    }

    bool IsStockSlot(int slot)
    {
        return slot >= 0 && slot < STOCK_NPC_SLOTS;
    }

    bool IsExtendedSlot(int slot)
    {
        return slot >= STOCK_NPC_SLOTS && slot < MAX_NPC_SLOTS;
    }

    bool IsValidLogicalSlot(int slot)
    {
        return slot >= 0 && slot < MAX_NPC_SLOTS;
    }

    PartySlotSnapshot& GetExtendedSlotState(int slot)
    {
        return gExtendedPartySlots[
            static_cast<std::size_t>(slot - STOCK_NPC_SLOTS)];
    }

    bool IsLogicalSlotPrimaryMember(void* partyTable, int slot)
    {
        if (!IsValidLogicalSlot(slot)) {
            return false;
        }

        const int activeCount = getObjectProperty<int>(
            partyTable,
            OFFSET_PARTY_MEMBER_COUNT);
        for (int i = 0; i < activeCount; ++i) {
            if (getObjectProperty<int>(
                    partyTable,
                    OFFSET_PARTY_MEMBER_SLOTS +
                        i * static_cast<int>(sizeof(int))) == slot) {
                return true;
            }
        }
        return false;
    }

    DWORD ResolveOrMaterializeLogicalNPCObjectId(void* partyTable, int logicalSlot)
    {
        auto getNpcObject = GameFunction<GetNPCObjectFn>(ADDRESS_GET_NPC_OBJECT);
        return getNpcObject(partyTable, logicalSlot, 1, 1);
    }

    using RawCExoString = std::array<DWORD, 2>;

    void SetControlVisible(void* control, bool visible)
    {
        DWORD flags = getObjectProperty<DWORD>(control, GUI_CONTROL_FLAGS_OFFSET);
        if (visible) {
            flags |= GUI_CONTROL_VISIBLE_FLAG;
        } else {
            flags &= ~GUI_CONTROL_VISIBLE_FLAG;
        }
        setObjectProperty<DWORD>(control, GUI_CONTROL_FLAGS_OFFSET, flags);
    }

    void SetControlHitTestEnabled(void* control, bool enabled)
    {
        DWORD flags = getObjectProperty<DWORD>(control, GUI_CONTROL_FLAGS_OFFSET);
        if (enabled) {
            flags &= ~GUI_CONTROL_MOUSE_IGNORE_FLAG;
        } else {
            flags |= GUI_CONTROL_MOUSE_IGNORE_FLAG;
        }
        setObjectProperty<DWORD>(control, GUI_CONTROL_FLAGS_OFFSET, flags);
    }

    DWORD GetLogicalNPCObjectId(void* partyTable, int slot)
    {
        if (!IsValidLogicalSlot(slot)) {
            return NPC_OBJECT_SENTINEL;
        }
        if (IsExtendedSlot(slot)) {
            return GetExtendedSlotState(slot).objectId;
        }
        return getObjectProperty<DWORD>(
            partyTable,
            OFFSET_NPC_OBJECT_IDS + slot * static_cast<int>(sizeof(DWORD)));
    }

    int FindExtendedNPCSlotByObjectId(DWORD objectId)
    {
        if (objectId == NPC_OBJECT_SENTINEL) {
            return -1;
        }
        for (int slot = STOCK_NPC_SLOTS; slot < MAX_NPC_SLOTS; ++slot) {
            if (GetExtendedSlotState(slot).objectId == objectId) {
                return slot;
            }
        }
        return -1;
    }

    int FindLogicalNPCSlotByObjectId(void* partyTable, DWORD objectId)
    {
        if (objectId == NPC_OBJECT_SENTINEL) {
            return -1;
        }
        for (int logicalSlot = 0; logicalSlot < MAX_NPC_SLOTS; ++logicalSlot) {
            if (GetLogicalNPCObjectId(partyTable, logicalSlot) == objectId) {
                return logicalSlot;
            }
        }
        return -1;
    }

    void SetLogicalNPCObjectId(void* partyTable, int slot, DWORD objectId)
    {
        if (!IsValidLogicalSlot(slot)) {
            return;
        }
        if (IsExtendedSlot(slot)) {
            GetExtendedSlotState(slot).objectId = objectId;
            return;
        }
        setObjectProperty<DWORD>(
            partyTable,
            OFFSET_NPC_OBJECT_IDS + slot * static_cast<int>(sizeof(DWORD)),
            objectId);
    }

    DWORD GetLogicalNPCAvailability(void* partyTable, int logicalSlot)
    {
        if (!IsValidLogicalSlot(logicalSlot)) {
            return 0;
        }
        if (IsExtendedSlot(logicalSlot)) {
            return GetExtendedSlotState(logicalSlot).available;
        }
        return getObjectProperty<DWORD>(
            partyTable,
            OFFSET_NPC_AVAILABLE + logicalSlot * static_cast<int>(sizeof(DWORD)));
    }

    bool GetLogicalNPCSelectability(void* partyTable, int logicalSlot)
    {
        if (!IsValidLogicalSlot(logicalSlot) ||
            GetLogicalNPCAvailability(partyTable, logicalSlot) == 0) {
            return false;
        }
        return IsStockSlot(logicalSlot)
            ? getObjectProperty<BYTE>(
                  partyTable,
                  OFFSET_NPC_SELECTABLE + logicalSlot) != 0
            : GetExtendedSlotState(logicalSlot).selectable != 0;
    }

    int FindActivePartySlot(void* partyTable, int logicalSlot)
    {
        int count = getObjectProperty<int>(partyTable, OFFSET_PARTY_MEMBER_COUNT);
        count = std::max(0, std::min(count, PRIMARY_MEMBER_CAPACITY));
        for (int index = 0; index < count; ++index) {
            if (getObjectProperty<int>(
                    partyTable,
                    OFFSET_PARTY_MEMBER_SLOTS + index * 4) == logicalSlot) {
                return index;
            }
        }
        return -1;
    }

    void* GetApplication(int appOffset)
    {
        auto* appManager = *reinterpret_cast<void**>(ADDRESS_SWKOTOR2_APP);
        return getObjectProperty<void*>(appManager, appOffset);
    }

    void* GetServerApplication()
    {
        return GetApplication(APP_MANAGER_SERVER_OFFSET);
    }

    void* GetClientApplication()
    {
        return GetApplication(APP_MANAGER_CLIENT_OFFSET);
    }

    void* GetCurrentPartyTable()
    {
        void* server = GetServerApplication();
        return GameFunction<GetCurrentPartyTableFn>(
            ADDRESS_GET_CURRENT_PARTY_TABLE)(server);
    }

    void* GetServerCreatureByObjectId(void* server, DWORD objectId)
    {
        if (objectId == NPC_OBJECT_SENTINEL) {
            return nullptr;
        }
        return GameFunction<GetCreatureByObjectIdFn>(
            ADDRESS_GET_CREATURE_BY_OBJECT_ID)(server, objectId);
    }

    void* GetCurrentPlayerCreature(void* server)
    {
        const DWORD objectId = GameFunction<ServerGetPlayerCreatureIdFn>(
            ADDRESS_SERVER_GET_PLAYER_CREATURE_ID)(server);
        return GetServerCreatureByObjectId(server, objectId);
    }

    void* GetClientCreatureForServerObject(void* client, DWORD serverObjectId)
    {
        if (serverObjectId == NPC_OBJECT_SENTINEL) {
            return nullptr;
        }
        const DWORD clientObjectId = GameFunction<ServerToClientObjectIdFn>(
            ADDRESS_SERVER_TO_CLIENT_OBJECT_ID)(client, serverObjectId);
        return GameFunction<ClientGetCreatureByObjectIdFn>(
            ADDRESS_CLIENT_GET_CREATURE_BY_OBJECT_ID)(client, clientObjectId);
    }

    int GetClientPartyIndexForServerObject(DWORD serverObjectId)
    {
        if (serverObjectId == NPC_OBJECT_SENTINEL) {
            return -1;
        }
        void* client = GetClientApplication();
        if (!client) {
            return -1;
        }
        void* clientParty = GameFunction<ClientGetPartyFn>(
            ADDRESS_CLIENT_GET_PARTY)(client);
        if (!clientParty) {
            return -1;
        }
        const DWORD clientObjectId = GameFunction<ServerToClientObjectIdFn>(
            ADDRESS_SERVER_TO_CLIENT_OBJECT_ID)(client, serverObjectId);
        return GameFunction<PartyGetIndexFn>(ADDRESS_PARTY_GET_INDEX)(
            clientParty,
            clientObjectId);
    }

    BYTE* GetStockPartySelectionDataAt(void* panel, int recordIndex)
    {
        return reinterpret_cast<BYTE*>(panel) +
            PANEL_PARTY_SELECTION_DATA_BASE_OFFSET +
            recordIndex * PARTY_SELECT_SLOT_STRIDE;
    }

    void* AllocateAligned(std::size_t size, std::size_t alignment)
    {
        const std::size_t total = size + alignment - 1 + sizeof(void*);
        void* raw = std::malloc(total);
        if (!raw) {
            return nullptr;
        }
        const uintptr_t start = reinterpret_cast<uintptr_t>(raw) + sizeof(void*);
        const uintptr_t aligned = (start + alignment - 1) & ~(alignment - 1);
        reinterpret_cast<void**>(aligned)[-1] = raw;
        return reinterpret_cast<void*>(aligned);
    }

    void FreeAligned(void* memory)
    {
        if (memory) {
            std::free(reinterpret_cast<void**>(memory)[-1]);
        }
    }

    PartySelectionViewState* FindPartySelectionView(void* panel)
    {
        for (PartySelectionViewState& state : gPartySelectionViews) {
            if (state.owner == panel) {
                return &state;
            }
        }
        return nullptr;
    }

    PartySelectionViewState* ReservePartySelectionView(void* panel)
    {
        if (PartySelectionViewState* state = FindPartySelectionView(panel)) {
            return state;
        }
        for (PartySelectionViewState& state : gPartySelectionViews) {
            if (!state.owner) {
                std::memset(&state, 0, sizeof(state));
                state.owner = panel;
                state.pageBase = -1;
                return &state;
            }
        }
        return nullptr;
    }

    BYTE* GetViewRecord(PartySelectionViewState* state, int physicalIndex)
    {
        return state->records + physicalIndex * PARTY_SELECT_SLOT_STRIDE;
    }

    BYTE* GetPageButton(PartySelectionViewState* state, int controlIndex)
    {
        return state->pageButtons + controlIndex * GUI_BUTTON_OBJECT_SIZE;
    }

    void BindNamedControl(void* panel, void* destination, const char* tag)
    {
        RawCExoString tagString;
        GameFunction<CExoStringCtorCStrFn>(ADDRESS_CEXOSTRING_CTOR_CSTR)(
            &tagString,
            tag);
        GameFunction<GuiPanelInitControlFn>(ADDRESS_GUI_PANEL_INIT_CONTROL)(
            panel,
            destination,
            &tagString,
            1,
            1);
        GameFunction<CExoStringDtorFn>(ADDRESS_CEXOSTRING_DTOR)(&tagString);
    }

    void SetUnavailableViewRecord(BYTE* record, int logicalSlot)
    {
        GameFunction<GuiPartySelectionButtonSetSelectedFn>(
            ADDRESS_PARTY_SELECTION_BUTTON_SET_SELECTED)(
                record + DATA_BUTTON_OFFSET,
                0);
        setObjectProperty<DWORD>(
            record,
            DATA_FLAGS_OFFSET,
            PANEL_SLOT_HAS_OBJECT_FLAG);
        setObjectProperty<int>(record, DATA_OBJECT_INDEX_OFFSET, -1);
        setObjectProperty<int>(record, DATA_LOGICAL_SLOT_OFFSET, logicalSlot);
        setObjectProperty<int>(record, DATA_AUXILIARY_OFFSET, 0);
        setObjectProperty<float>(record, DATA_ALPHA_OFFSET, 1.0f);

        BYTE* button = record + DATA_BUTTON_OFFSET;
        BYTE* unavailable = record + DATA_NOT_AVAILABLE_LABEL_OFFSET;
        BYTE* character = record + DATA_CHARACTER_LABEL_OFFSET;
        SetControlVisible(button, true);
        SetControlHitTestEnabled(button, false);
        GameFunction<GuiButtonSetEnabledFn>(ADDRESS_GUI_BUTTON_SET_ENABLED)(
            button,
            0);
        SetControlVisible(unavailable, logicalSlot < STOCK_NPC_SLOTS);
        SetControlHitTestEnabled(unavailable, false);
        SetControlVisible(character, false);
        SetControlHitTestEnabled(character, false);
    }

    bool SetAvailableViewRecord(
        BYTE* record,
        PartySelectionViewState* state,
        void* partyTable,
        int logicalSlot)
    {
        const DWORD objectId = ResolveOrMaterializeLogicalNPCObjectId(
            partyTable,
            logicalSlot);
        void* creature = GetServerCreatureByObjectId(
            GetServerApplication(),
            objectId);
        if (!creature) {
            SetUnavailableViewRecord(record, logicalSlot);
            return false;
        }

        CResRef_struct portrait = {};
        GameFunction<ObjectGetPortraitFn>(ADDRESS_OBJECT_GET_PORTRAIT)(
            creature,
            &portrait);
        GameFunction<GuiBorderSetFillImageFn>(
            ADDRESS_GUI_BORDER_SET_FILL_IMAGE)(
                record + DATA_CHARACTER_TEXTURE_PARAMS_OFFSET,
                &portrait,
                0);

        const int activeSlot = FindActivePartySlot(partyTable, logicalSlot);
        const bool forced = state->forced[logicalSlot];
        const bool selected = state->selected[logicalSlot] || forced;
        const bool interactive =
            GetLogicalNPCSelectability(partyTable, logicalSlot) && !forced;
        DWORD flags = PANEL_SLOT_ENABLED_FLAG | PANEL_SLOT_HAS_OBJECT_FLAG;
        if (forced) {
            flags |= PANEL_SLOT_FORCED_FLAG;
        }
        setObjectProperty<DWORD>(record, DATA_FLAGS_OFFSET, flags);
        const int clientPartyIndex = activeSlot >= 0
            ? GetClientPartyIndexForServerObject(objectId)
            : -1;
        setObjectProperty<int>(
            record,
            DATA_OBJECT_INDEX_OFFSET,
            clientPartyIndex);
        setObjectProperty<int>(record, DATA_LOGICAL_SLOT_OFFSET, logicalSlot);
        setObjectProperty<int>(record, DATA_AUXILIARY_OFFSET, 0);
        setObjectProperty<float>(record, DATA_ALPHA_OFFSET, 1.0f);
        GameFunction<GuiPartySelectionButtonSetSelectedFn>(
            ADDRESS_PARTY_SELECTION_BUTTON_SET_SELECTED)(
                record + DATA_BUTTON_OFFSET,
                selected ? 1 : 0);

        BYTE* button = record + DATA_BUTTON_OFFSET;
        BYTE* unavailable = record + DATA_NOT_AVAILABLE_LABEL_OFFSET;
        BYTE* character = record + DATA_CHARACTER_LABEL_OFFSET;
        SetControlVisible(button, true);
        SetControlHitTestEnabled(button, interactive);
        GameFunction<GuiButtonSetEnabledFn>(ADDRESS_GUI_BUTTON_SET_ENABLED)(
            button,
            interactive ? 1 : 0);
        SetControlVisible(unavailable, false);
        SetControlHitTestEnabled(unavailable, false);
        SetControlVisible(character, true);
        SetControlHitTestEnabled(character, false);
        return true;
    }

    void InitializeViewRecord(
        void* panel,
        PartySelectionViewState* state,
        int physicalIndex)
    {
        BYTE* record = GetViewRecord(state, physicalIndex);
        std::memset(record, 0, PARTY_SELECT_SLOT_STRIDE);
        GameFunction<PartySelectionRecordCtorFn>(
            ADDRESS_PARTY_SELECTION_RECORD_CTOR)(record);
        ++state->constructedRecords;

        char tag[32];
        std::snprintf(tag, sizeof(tag), "LBL_NA%d", physicalIndex);
        BindNamedControl(panel, record + DATA_NOT_AVAILABLE_LABEL_OFFSET, tag);
        std::snprintf(tag, sizeof(tag), "LBL_CHAR%d", physicalIndex);
        BindNamedControl(panel, record + DATA_CHARACTER_LABEL_OFFSET, tag);
        std::snprintf(tag, sizeof(tag), "BTN_NPC%d", physicalIndex);
        BindNamedControl(panel, record + DATA_BUTTON_OFFSET, tag);

        auto addEvent = GameFunction<GuiControlAddEventFn>(
            ADDRESS_GUI_CONTROL_ADD_EVENT);
        BYTE* button = record + DATA_BUTTON_OFFSET;
        addEvent(
            button,
            GUI_CONTROL_PRIMARY_EVENT,
            panel,
            reinterpret_cast<void*>(ADDRESS_PARTY_SELECTION_ON_TOGGLED));
        addEvent(
            button,
            0,
            panel,
            reinterpret_cast<void*>(ADDRESS_PARTY_SELECTION_ON_ENTER));
        addEvent(
            button,
            1,
            panel,
            reinterpret_cast<void*>(ADDRESS_PARTY_SELECTION_ON_EXIT));

        setObjectProperty<void*>(
            record,
            DATA_BUTTON_BACK_POINTER_OFFSET,
            record);
        std::memcpy(
            record + DATA_DEFAULT_COLOR_OFFSET,
            GetStockPartySelectionDataAt(panel, 0) + DATA_DEFAULT_COLOR_OFFSET,
            sizeof(float) * 3);
        GameFunction<GuiBorderSetPulsingAlphaFn>(
            ADDRESS_GUI_BORDER_SET_PULSING_ALPHA)(
                record + DATA_BUTTON_INIT_PARAMS_OFFSET,
                1,
                1,
                0);
        setObjectProperty<DWORD>(record, DATA_FLAGS_OFFSET, 0u);
        setObjectProperty<int>(record, DATA_LOGICAL_SLOT_OFFSET, physicalIndex);
        setObjectProperty<int>(record, DATA_AUXILIARY_OFFSET, 0);
        SetUnavailableViewRecord(record, physicalIndex);
    }

    void __fastcall PartySelectionPrevButtonCallback(void* self, void*, void*);
    void __fastcall PartySelectionNextButtonCallback(void* self, void*, void*);

    void InitializePageButton(
        void* panel,
        PartySelectionViewState* state,
        int index,
        const char* tag,
        void* callback)
    {
        BYTE* button = GetPageButton(state, index);
        std::memset(button, 0, GUI_BUTTON_OBJECT_SIZE);
        GameFunction<GuiButtonCtorFn>(ADDRESS_GUI_BUTTON_CTOR)(button);
        ++state->constructedButtons;
        BindNamedControl(panel, button, tag);
        GameFunction<GuiControlAddEventFn>(ADDRESS_GUI_CONTROL_ADD_EVENT)(
            button,
            GUI_CONTROL_PRIMARY_EVENT,
            panel,
            callback);
    }

    bool CreatePartySelectionView(void* panel)
    {
        PartySelectionViewState* state = ReservePartySelectionView(panel);
        if (!state || state->ready) {
            return state && state->ready;
        }
        state->records = static_cast<BYTE*>(AllocateAligned(
            PARTY_SELECTION_VIEW_RECORD_COUNT * PARTY_SELECT_SLOT_STRIDE,
            16));
        state->pageButtons = static_cast<BYTE*>(AllocateAligned(
            PARTY_SELECTION_PAGE_CONTROL_COUNT * GUI_BUTTON_OBJECT_SIZE,
            16));
        if (!state->records || !state->pageButtons) {
            FreeAligned(state->records);
            FreeAligned(state->pageButtons);
            std::memset(state, 0, sizeof(*state));
            return false;
        }

        for (int index = 0; index < PARTY_SELECTION_VIEW_RECORD_COUNT; ++index) {
            InitializeViewRecord(panel, state, index);
        }
        InitializePageButton(
            panel,
            state,
            PARTY_SELECTION_PAGE_PREV_INDEX,
            "BTN_PAGE_PREV",
            funcAddr(PartySelectionPrevButtonCallback));
        InitializePageButton(
            panel,
            state,
            PARTY_SELECTION_PAGE_NEXT_INDEX,
            "BTN_PAGE_NEXT",
            funcAddr(PartySelectionNextButtonCallback));
        state->ready = true;
        return true;
    }

    void DetachControl(void* panel, void* control)
    {
        auto delIndex = GameFunction<ExoArrayListPointerDelIndexFn>(
            ADDRESS_EXO_ARRAY_LIST_POINTER_DEL_INDEX);
        void** controls = getObjectProperty<void**>(
            panel,
            GUI_PANEL_CONTROL_ARRAY_OFFSET);
        const int count = getObjectProperty<int>(
            panel,
            GUI_PANEL_CONTROL_COUNT_OFFSET);
        for (int index = 0; index < count; ++index) {
            if (controls[index] == control) {
                delIndex(
                    reinterpret_cast<BYTE*>(panel) +
                        GUI_PANEL_CONTROL_ARRAY_OFFSET,
                    index);
                return;
            }
        }
    }

    void DetachPartySelectionControls(
        void* panel,
        PartySelectionViewState* state,
        bool includePageButtons)
    {
        for (int index = 0; index < STOCK_NPC_SLOTS; ++index) {
            BYTE* record = GetStockPartySelectionDataAt(panel, index);
            for (int offset : PARTY_SELECTION_RECORD_CONTROL_OFFSETS) {
                DetachControl(panel, record + offset);
            }
        }
        if (state && state->records) {
            for (int index = 0;
                 index < PARTY_SELECTION_VIEW_RECORD_COUNT;
                 ++index) {
                BYTE* record = GetViewRecord(state, index);
                for (int offset : PARTY_SELECTION_RECORD_CONTROL_OFFSETS) {
                    DetachControl(panel, record + offset);
                }
            }
        }
        if (includePageButtons && state && state->pageButtons) {
            for (int index = 0;
                 index < PARTY_SELECTION_PAGE_CONTROL_COUNT;
                 ++index) {
                DetachControl(panel, GetPageButton(state, index));
            }
        }
    }

    void AttachStockPartySelectionControls(void* panel)
    {
        PartySelectionViewState* state = FindPartySelectionView(panel);
        DetachPartySelectionControls(panel, state, false);
        auto addControl = GameFunction<GuiPanelAddControlFn>(
            ADDRESS_GUI_PANEL_ADD_CONTROL);
        for (int offset : PARTY_SELECTION_RECORD_CONTROL_OFFSETS) {
            for (int index = 0; index < STOCK_NPC_SLOTS; ++index) {
                addControl(
                    panel,
                    GetStockPartySelectionDataAt(panel, index) + offset);
            }
        }
    }

    int CountSelectedSlots(const PartySelectionViewState* state)
    {
        int count = 0;
        for (int slot = 0; slot < MAX_NPC_SLOTS; ++slot) {
            count += state->selected[slot] || state->forced[slot];
        }
        return count;
    }

    void CapturePartySelectionPage(PartySelectionViewState* state)
    {
        if (!state || !state->ready || state->pageBase < 0) {
            return;
        }
        for (int physicalIndex = 0;
             physicalIndex < PARTY_SELECTION_VIEW_RECORD_COUNT;
             ++physicalIndex) {
            const int logicalSlot = state->pageBase + physicalIndex;
            BYTE* record = GetViewRecord(state, physicalIndex);
            const bool selected = getObjectProperty<int>(
                record,
                DATA_BUTTON_SELECTED_OFFSET) != 0;
            state->selected[logicalSlot] =
                selected || state->forced[logicalSlot];
        }
        setObjectProperty<int>(
            state->owner,
            PANEL_SELECTED_COUNT_OFFSET,
            CountSelectedSlots(state));
    }

    void InitializePartySelectionModel(
        void* panel,
        PartySelectionViewState* state,
        void* partyTable)
    {
        std::memset(state->selected, 0, sizeof(state->selected));
        int activeCount = getObjectProperty<int>(
            partyTable,
            OFFSET_PARTY_MEMBER_COUNT);
        activeCount = std::max(0, std::min(activeCount, PRIMARY_MEMBER_CAPACITY));
        for (int index = 0; index < activeCount; ++index) {
            const int logicalSlot = getObjectProperty<int>(
                partyTable,
                OFFSET_PARTY_MEMBER_SLOTS + index * 4);
            if (IsValidLogicalSlot(logicalSlot)) {
                state->selected[logicalSlot] = true;
            }
        }
        for (int slot = 0; slot < MAX_NPC_SLOTS; ++slot) {
            if (state->forced[slot]) {
                state->selected[slot] = true;
            }
        }
        state->pageBase = -1;
        setObjectProperty<int>(
            panel,
            PANEL_SELECTED_COUNT_OFFSET,
            CountSelectedSlots(state));
    }

    void ClearPartySelectionPortraitContext(void* panel)
    {
        BYTE* previous = getObjectProperty<BYTE*>(
            panel,
            PANEL_CURRENT_PORTRAIT_OFFSET);
        if (previous) {
            BYTE* button = previous + DATA_BUTTON_OFFSET;
            void** vtable = *reinterpret_cast<void***>(button);
            reinterpret_cast<GuiButtonSetActiveFn>(
                vtable[GUI_BUTTON_SET_ACTIVE_VTABLE_INDEX])(button, 0);
        }
        setObjectProperty<void*>(
            panel,
            PANEL_CURRENT_PORTRAIT_OFFSET,
            nullptr);
    }

    void ShowPartySelectionPage(void* panel, int pageBase)
    {
        PartySelectionViewState* state = FindPartySelectionView(panel);
        if (!state || !state->ready) {
            return;
        }
        CapturePartySelectionPage(state);
        void* partyTable = GetCurrentPartyTable();
        if (!partyTable) {
            return;
        }
        state->pageBase = pageBase;

        auto setActiveControl = GameFunction<GuiPanelSetActiveControlFn>(
            ADDRESS_GUI_PANEL_SET_ACTIVE_CONTROL);
        auto addControl = GameFunction<GuiPanelAddControlFn>(
            ADDRESS_GUI_PANEL_ADD_CONTROL);
        auto setEnabled = GameFunction<GuiButtonSetEnabledFn>(
            ADDRESS_GUI_BUTTON_SET_ENABLED);
        setActiveControl(panel, nullptr, 0);
        ClearPartySelectionPortraitContext(panel);
        DetachPartySelectionControls(panel, state, false);

        for (int physicalIndex = 0;
             physicalIndex < PARTY_SELECTION_VIEW_RECORD_COUNT;
             ++physicalIndex) {
            const int logicalSlot = pageBase + physicalIndex;
            BYTE* record = GetViewRecord(state, physicalIndex);
            if (GetLogicalNPCAvailability(partyTable, logicalSlot) != 0) {
                SetAvailableViewRecord(
                    record,
                    state,
                    partyTable,
                    logicalSlot);
            } else {
                SetUnavailableViewRecord(record, logicalSlot);
            }
        }

        for (int physicalIndex = 0;
             physicalIndex < PARTY_SELECTION_VIEW_RECORD_COUNT;
             ++physicalIndex) {
            addControl(
                panel,
                GetViewRecord(state, physicalIndex) +
                    DATA_CHARACTER_LABEL_OFFSET);
        }

        void* firstInteractive = nullptr;
        for (int physicalIndex = 0;
             physicalIndex < PARTY_SELECTION_VIEW_RECORD_COUNT;
             ++physicalIndex) {
            BYTE* record = GetViewRecord(state, physicalIndex);
            BYTE* button = record + DATA_BUTTON_OFFSET;
            const DWORD flags = getObjectProperty<DWORD>(
                record,
                DATA_FLAGS_OFFSET);
            const int logicalSlot = getObjectProperty<int>(
                record,
                DATA_LOGICAL_SLOT_OFFSET);
            const bool interactive =
                (flags & PANEL_SLOT_ENABLED_FLAG) != 0 &&
                (flags & PANEL_SLOT_FORCED_FLAG) == 0 &&
                GetLogicalNPCSelectability(partyTable, logicalSlot);
            addControl(panel, button);
            if (!firstInteractive && interactive) {
                firstInteractive = button;
            }
        }

        for (int physicalIndex = 0;
             physicalIndex < PARTY_SELECTION_VIEW_RECORD_COUNT;
             ++physicalIndex) {
            BYTE* unavailable = GetViewRecord(state, physicalIndex) +
                DATA_NOT_AVAILABLE_LABEL_OFFSET;
            if ((getObjectProperty<DWORD>(
                    unavailable,
                    GUI_CONTROL_FLAGS_OFFSET) &
                 GUI_CONTROL_VISIBLE_FLAG) != 0) {
                addControl(panel, unavailable);
            }
        }

        BYTE* previous = GetPageButton(state, PARTY_SELECTION_PAGE_PREV_INDEX);
        BYTE* next = GetPageButton(state, PARTY_SELECTION_PAGE_NEXT_INDEX);
        setEnabled(previous, pageBase != 0);
        setEnabled(next, pageBase != LAST_PARTY_SELECT_PAGE_BASE);
        BYTE* done = reinterpret_cast<BYTE*>(panel) +
            PANEL_DONE_BUTTON_OFFSET;
        auto setMoveToControl = GameFunction<GuiSetMoveToControlFn>(
            ADDRESS_GUI_SET_MOVE_TO_CONTROL);
        setMoveToControl(previous, GUI_NAVIGATION_UP, firstInteractive);
        setMoveToControl(next, GUI_NAVIGATION_UP, firstInteractive);
        setMoveToControl(done, GUI_NAVIGATION_UP, firstInteractive);

        setObjectProperty<int>(
            panel,
            PANEL_SELECTED_COUNT_OFFSET,
            CountSelectedSlots(state));
        GameFunction<PartySelectionUpdateCountFn>(
            ADDRESS_PARTY_SELECTION_UPDATE_COUNT)(panel);
        if (!firstInteractive) {
            firstInteractive = pageBase == 0 ? next : previous;
        }
        setActiveControl(panel, firstInteractive, 1);
    }

    void __fastcall PartySelectionPrevButtonCallback(void* self, void*, void*)
    {
        ShowPartySelectionPage(self, 0);
    }

    void __fastcall PartySelectionNextButtonCallback(void* self, void*, void*)
    {
        ShowPartySelectionPage(self, LAST_PARTY_SELECT_PAGE_BASE);
    }

    void DestroyPartySelectionView(void* panel)
    {
        PartySelectionViewState* state = FindPartySelectionView(panel);
        if (!state) {
            return;
        }
        DetachPartySelectionControls(panel, state, true);
        while (state->constructedButtons > 0) {
            --state->constructedButtons;
            GameFunction<GuiButtonDtorFn>(ADDRESS_GUI_BUTTON_DTOR)(
                GetPageButton(
                    state,
                    static_cast<int>(state->constructedButtons)));
        }
        while (state->constructedRecords > 0) {
            --state->constructedRecords;
            GameFunction<PartySelectionRecordDtorFn>(
                ADDRESS_PARTY_SELECTION_RECORD_DTOR)(
                    GetViewRecord(
                        state,
                        static_cast<int>(state->constructedRecords)));
        }
        FreeAligned(state->pageButtons);
        FreeAligned(state->records);
        std::memset(state, 0, sizeof(*state));
        AttachStockPartySelectionControls(panel);
    }

    void DeleteEngineObject(void* object)
    {
        void** vtable = *reinterpret_cast<void***>(object);
        reinterpret_cast<ScalarDeletingDestructorFn>(
            vtable[SCALAR_DELETING_DESTRUCTOR_VTABLE_INDEX])(object, 1u);
    }

    void* AllocateEngineObject(unsigned int size)
    {
        auto allocate = GameFunction<OperatorNewFn>(ADDRESS_OPERATOR_NEW);
        return allocate(size);
    }

    int SaveCreatureToExtendedResource(int logicalSlot, void* creature)
    {
        void* gff = AllocateEngineObject(RESGFF_OBJECT_SIZE);
        char outputPath[32];
        std::snprintf(outputPath, sizeof(outputPath), "GAMEINPROGRESS:AVAILNPC%d", logicalSlot);
        RawCExoString fileType;
        RawCExoString path;
        auto stringCtor = GameFunction<CExoStringCtorCStrFn>(ADDRESS_CEXOSTRING_CTOR_CSTR);
        auto stringDtor = GameFunction<CExoStringDtorFn>(ADDRESS_CEXOSTRING_DTOR);
        stringCtor(&fileType, "UTC ");
        stringCtor(&path, outputPath);

        GameFunction<CResGffCtorFn>(ADDRESS_CRESGFF_CTOR)(gff);
        DWORD topLevel;  // Stores the native CResStruct index handle.
        GameFunction<CResGffCreateFileFn>(ADDRESS_CRESGFF_CREATE_FILE)(
            gff, &topLevel, &fileType, nullptr);
        GameFunction<CreatureSaveCreatureFn>(ADDRESS_CREATURE_SAVE_CREATURE)(
            creature, gff, &topLevel);
        const int writeResult =
            GameFunction<CResGffWriteFileFn>(ADDRESS_CRESGFF_WRITE_FILE)(
                gff, &path, SAVED_CREATURE_RESOURCE_TYPE);
        DeleteEngineObject(gff);
        stringDtor(&path);
        stringDtor(&fileType);
        return writeResult;
    }

    void SaveExtendedMemberByObjectId(int logicalSlot,
                                      DWORD objectId,
                                      int saveMode)
    {
        void* creature = GetServerCreatureByObjectId(
            GetServerApplication(),
            objectId);
        if (!creature) {
            return;
        }
        if (saveMode != 0) {
            auto clearActions = GameFunction<ObjectClearAllActionsFn>(
                ADDRESS_OBJECT_CLEAR_ALL_ACTIONS);
            clearActions(creature, 1);
        }
        SaveCreatureToExtendedResource(logicalSlot, creature);
    }

    void RegisterLoadedCreatureWithAI(void* server, void* creature)
    {
        void** vtable = *reinterpret_cast<void***>(creature);
        auto asServerObject = reinterpret_cast<ObjectAsServerObjectVirtualFn>(
            vtable[OBJECT_AS_SERVER_OBJECT_VTABLE_INDEX]);
        void* serverObject = asServerObject(creature);
        auto getAiMaster = GameFunction<ServerGetAiMasterFn>(ADDRESS_SERVER_GET_AI_MASTER);
        void* aiMaster = getAiMaster(server);
        auto setAiLevel = GameFunction<AiMasterSetAiLevelFn>(ADDRESS_AI_MASTER_SET_AI_LEVEL);
        setAiLevel(aiMaster, serverObject, 3);
    }

    void ApplyPostLoadCreatureRecovery(void* creature, int applyRecovery)
    {
        if (applyRecovery == 0) {
            return;
        }
        void** vtable = *reinterpret_cast<void***>(creature);
        auto getStatus = reinterpret_cast<ObjectStatusVirtualFn>(
            vtable[OBJECT_STATUS_VTABLE_INDEX]);
        if (getStatus(creature, 0) > 0) {
            return;
        }

        setObjectProperty<DWORD>(creature, CREATURE_POST_LOAD_STATE_OFFSET, 1u);
        void* effect = AllocateEngineObject(GAME_EFFECT_OBJECT_SIZE);
        auto effectCtor = GameFunction<GameEffectCtorFn>(ADDRESS_GAME_EFFECT_CTOR);
        effectCtor(effect, 1);
        const WORD effectType = *reinterpret_cast<const WORD*>(ADDRESS_EFFECT_TYPE_WORD);
        const WORD effectSubtype = *reinterpret_cast<const WORD*>(ADDRESS_EFFECT_SUBTYPE_WORD);
        setObjectProperty<WORD>(effect, GAME_EFFECT_TYPE_OFFSET, effectType);
        WORD flags = getObjectProperty<WORD>(effect, GAME_EFFECT_SUBTYPE_FLAGS_OFFSET);
        flags = static_cast<WORD>(
            (flags & static_cast<WORD>(~GAME_EFFECT_SUBTYPE_MASK)) |
            (effectSubtype & GAME_EFFECT_SUBTYPE_MASK));
        setObjectProperty<WORD>(effect, GAME_EFFECT_SUBTYPE_FLAGS_OFFSET, flags);
        auto applyEffect = GameFunction<ObjectApplyEffectFn>(ADDRESS_OBJECT_APPLY_EFFECT);
        applyEffect(creature, effect, 0, 0);
    }

    void AddCreatureToPlayerFaction(void* creature)
    {
        void* playerCreature = GetCurrentPlayerCreature(
            GetServerApplication());
        if (!playerCreature) {
            return;
        }

        auto getFaction = GameFunction<CreatureGetFactionFn>(ADDRESS_CREATURE_GET_FACTION);
        void* faction = getFaction(playerCreature);
        if (!faction) {
            return;
        }

        auto addMember = GameFunction<FactionAddMemberFn>(ADDRESS_FACTION_ADD_MEMBER);
        addMember(faction, getObjectProperty<DWORD>(creature, GAME_OBJECT_ID_OFFSET), 0);
    }

    float GetRecruitmentExperienceScale(int logicalSlot)
    {
        float scale = 1.0f;
        void* root = *reinterpret_cast<void**>(ADDRESS_2DA_ROOT);
        void* collection = root
            ? getObjectProperty<void*>(root, OFFSET_2DA_COLLECTION)
            : nullptr;
        void* table = collection
            ? getObjectProperty<void*>(collection, OFFSET_PARTY_NPC_2DA)
            : nullptr;
        if (!table) {
            return scale;
        }

        RawCExoString column;
        GameFunction<CExoStringCtorCStrFn>(ADDRESS_CEXOSTRING_CTOR_CSTR)(
            &column,
            "PercentXP");
        int percent = 0;
        const bool found = GameFunction<TwoDaGetIntEntryByRowLabelFn>(
            ADDRESS_2DA_GET_INT_ENTRY_BY_ROW_LABEL)(
                table,
                logicalSlot,
                &column,
                &percent);
        GameFunction<CExoStringDtorFn>(ADDRESS_CEXOSTRING_DTOR)(&column);
        if (found != 0 && percent != 0) {
            scale = static_cast<float>(percent) / 100.0f;
        }
        return scale;
    }

    void ApplyExtendedRecruitmentExperience(
        void* partyTable,
        int logicalSlot,
        void* creature)
    {
        const int targetExperience = static_cast<int>(
            static_cast<float>(getObjectProperty<int>(
                partyTable,
                PARTY_TARGET_EXPERIENCE_OFFSET)) *
            GetRecruitmentExperienceScale(logicalSlot));
        void* stats = getObjectProperty<void*>(creature, CREATURE_STATS_OFFSET);
        auto getLevel = GameFunction<CreatureStatsGetLevelFn>(ADDRESS_CREATURE_STATS_GET_LEVEL);
        const int level = static_cast<int>(getLevel(stats, 0));
        const int currentLevelThreshold = level * (level - 1) * 500;
        setObjectProperty<int>(creature, CREATURE_EXPERIENCE_ADJUSTMENT_OFFSET, 0);
        auto receiveExperience = GameFunction<CreatureReceiveExperienceFn>(
            ADDRESS_CREATURE_RECEIVE_EXPERIENCE);
        const int adjustment = currentLevelThreshold <= targetExperience
            ? targetExperience
            : currentLevelThreshold - targetExperience;
        receiveExperience(creature, adjustment);
    }

    int DirectAddExtendedNPCFromCreature(void* partyTable,
                                         int logicalSlot,
                                         void* creature)
    {
        PartySlotSnapshot& state = GetExtendedSlotState(logicalSlot);
        state.available = 1;
        state.objectId = getObjectProperty<DWORD>(
            creature,
            GAME_OBJECT_ID_OFFSET);

        auto transferInventory = GameFunction<TransferInventoryFn>(ADDRESS_TRANSFER_INVENTORY);
        transferInventory(partyTable, creature);
        ApplyExtendedRecruitmentExperience(
            partyTable,
            logicalSlot,
            creature);
        AddCreatureToPlayerFaction(creature);

        void* stats = getObjectProperty<void*>(creature, CREATURE_STATS_OFFSET);
        auto setMovementRate = GameFunction<CreatureStatsSetMovementRateFn>(
            ADDRESS_CREATURE_STATS_SET_MOVEMENT_RATE);
        setMovementRate(stats, 0);
        auto handleInfluence = GameFunction<CreatureHandleAlignmentInfluenceFn>(
            ADDRESS_CREATURE_HANDLE_ALIGNMENT_INFLUENCE);
        handleInfluence(creature);
        const int writeResult =
            SaveCreatureToExtendedResource(logicalSlot, creature);

        state.objectId = NPC_OBJECT_SENTINEL;
        return writeResult;
    }

    DWORD LoadExtendedCreatureFromSavedResource(void* partyTable,
                                                 void* server,
                                                 int logicalSlot,
                                                 int applyRecovery)
    {
        void* creature = AllocateEngineObject(CREATURE_OBJECT_SIZE);
        GameFunction<CreatureCtorFn>(ADDRESS_CREATURE_CTOR)(creature, NPC_OBJECT_SENTINEL, 0);
        char identity[16];
        std::snprintf(identity, sizeof(identity), "AVAILNPC%d", logicalSlot);
        GameFunction<AddGameInProgressFn>(ADDRESS_ADD_GAME_IN_PROGRESS)(partyTable);
        int loadResult = 0;
        {
            CResRef identityRef(static_cast<const char*>(identity));
            loadResult = GameFunction<CreatureLoadFromTemplateFn>(
                ADDRESS_CREATURE_LOAD_FROM_TEMPLATE)(
                    creature,
                    identityRef.GetPtr(),
                    0);
        }
        if (loadResult == 0) {
            DeleteEngineObject(creature);
            return NPC_OBJECT_SENTINEL;
        }
        GameFunction<RemoveGameInProgressFn>(ADDRESS_REMOVE_GAME_IN_PROGRESS)(partyTable);
        ApplyPostLoadCreatureRecovery(creature, applyRecovery);
        const DWORD objectId = getObjectProperty<DWORD>(creature, GAME_OBJECT_ID_OFFSET);
        GetExtendedSlotState(logicalSlot).objectId = objectId;
        RegisterLoadedCreatureWithAI(server, creature);
        return objectId;
    }

    DWORD GetPrimaryMemberObjectId(void* partyTable, int memberIndex)
    {
        const int logicalSlot = getObjectProperty<int>(
            partyTable,
            OFFSET_PARTY_MEMBER_SLOTS +
                memberIndex * static_cast<int>(sizeof(int)));
        return GetLogicalNPCObjectId(partyTable, logicalSlot);
    }

    bool RemoveExtendedPartyMember(void* partyTable, int logicalSlot)
    {
        const int activeIndex = FindActivePartySlot(partyTable, logicalSlot);
        if (activeIndex < 0) {
            return false;
        }
        const DWORD objectId = GetLogicalNPCObjectId(
            partyTable,
            logicalSlot);
        const int clientPartyIndex = GetClientPartyIndexForServerObject(
            objectId);
        if (clientPartyIndex < 0) {
            return false;
        }
        void* clientParty = GameFunction<ClientGetPartyFn>(
            ADDRESS_CLIENT_GET_PARTY)(GetClientApplication());
        void* clientMember = clientParty
            ? GameFunction<PartyGetCharacterFn>(ADDRESS_PARTY_GET_CHARACTER)(
                  clientParty,
                  clientPartyIndex)
            : nullptr;
        if (!clientMember) {
            return false;
        }
        void* creature = GameFunction<ClientCreatureGetServerCreatureFn>(
            ADDRESS_CLIENT_CREATURE_GET_SERVER_CREATURE)(clientMember);
        GameFunction<ClientCreatureSetInPartyFn>(
            ADDRESS_CLIENT_CREATURE_SET_IN_PARTY)(clientMember, 0);
        if (creature) {
            GameFunction<ServerCreatureSetInPartyFn>(
                ADDRESS_SERVER_CREATURE_SET_IN_PARTY)(creature, 0, 1);
        }
        return GameFunction<PartyTableRemoveMemberFn>(
            ADDRESS_PARTY_TABLE_REMOVE_MEMBER)(
                partyTable,
                logicalSlot) != 0;
    }

    bool AddExtendedPartyMember(void* partyTable, int logicalSlot)
    {
        if (!IsExtendedSlot(logicalSlot) ||
            GetLogicalNPCAvailability(partyTable, logicalSlot) == 0 ||
            FindActivePartySlot(partyTable, logicalSlot) >= 0) {
            return false;
        }
        int activeCount = getObjectProperty<int>(
            partyTable,
            OFFSET_PARTY_MEMBER_COUNT);
        if (activeCount < 0 || activeCount >= PRIMARY_MEMBER_CAPACITY) {
            return false;
        }
        void* playerCreature = GetCurrentPlayerCreature(
            GetServerApplication());
        if (!playerCreature) {
            return false;
        }
        auto* playerBytes = static_cast<BYTE*>(playerCreature);
        const DWORD objectId = GameFunction<PartyTableSpawnNpcFn>(
            ADDRESS_PARTY_TABLE_SPAWN_NPC)(
                partyTable,
                logicalSlot,
                1,
                playerBytes + OBJECT_POSITION_OFFSET,
                playerBytes + OBJECT_ORIENTATION_OFFSET,
                1);
        if (objectId == NPC_OBJECT_SENTINEL ||
            GameFunction<PartyTableAddMemberFn>(
                ADDRESS_PARTY_TABLE_ADD_MEMBER)(
                    partyTable,
                    logicalSlot,
                    objectId) == 0) {
            return false;
        }

        void* creature = GetServerCreatureByObjectId(
            GetServerApplication(),
            objectId);
        if (!creature) {
            return false;
        }
        GameFunction<ObjectClearAllActionsFn>(ADDRESS_OBJECT_CLEAR_ALL_ACTIONS)(
            creature,
            1);
        if (void* path = getObjectProperty<void*>(
                creature,
                CREATURE_PATHFIND_INFO_OFFSET)) {
            GameFunction<PathfindResetWaypointDataFn>(
                ADDRESS_PATHFIND_RESET_WAYPOINT_DATA)(path);
        }
        GameFunction<ServerCreatureSetInPartyFn>(
            ADDRESS_SERVER_CREATURE_SET_IN_PARTY)(creature, 1, 1);
        void** vtable = *reinterpret_cast<void***>(creature);
        if (vtable && vtable[0x70 / sizeof(void*)]) {
            reinterpret_cast<void(__thiscall*)(void*)>(
                vtable[0x70 / sizeof(void*)])(creature);
        }
        if (GameFunction<CreatureGetVisibleListElementFn>(
                ADDRESS_CREATURE_GET_VISIBLE_LIST_ELEMENT)(
                    playerCreature,
                    objectId) == 0) {
            GameFunction<CreatureAddToVisibleListFn>(
                ADDRESS_CREATURE_ADD_TO_VISIBLE_LIST)(
                    playerCreature,
                    objectId,
                    1,
                    1,
                    0,
                    0);
        }
        void* clientParty = GameFunction<ClientGetPartyFn>(
            ADDRESS_CLIENT_GET_PARTY)(GetClientApplication());
        if (clientParty) {
            activeCount = getObjectProperty<int>(
                partyTable,
                OFFSET_PARTY_MEMBER_COUNT);
            void* clientMember = GameFunction<PartyGetCharacterFn>(
                ADDRESS_PARTY_GET_CHARACTER)(clientParty, activeCount);
            if (clientMember) {
                GameFunction<ClientCreatureSetInPartyFn>(
                    ADDRESS_CLIENT_CREATURE_SET_IN_PARTY)(clientMember, 1);
            }
        }
        return true;
    }

    void StageStockPartySelectionRecords(
        void* panel,
        PartySelectionViewState* state,
        void* partyTable)
    {
        auto setSelected = GameFunction<GuiPartySelectionButtonSetSelectedFn>(
            ADDRESS_PARTY_SELECTION_BUTTON_SET_SELECTED);
        for (int logicalSlot = 0;
             logicalSlot < STOCK_NPC_SLOTS;
             ++logicalSlot) {
            BYTE* record = GetStockPartySelectionDataAt(panel, logicalSlot);
            const bool available =
                GetLogicalNPCAvailability(partyTable, logicalSlot) != 0;
            const bool selected = available &&
                (state->selected[logicalSlot] || state->forced[logicalSlot]);
            const int activeIndex = FindActivePartySlot(
                partyTable,
                logicalSlot);
            const int clientPartyIndex = activeIndex >= 0
                ? GetClientPartyIndexForServerObject(
                      GetLogicalNPCObjectId(partyTable, logicalSlot))
                : -1;
            DWORD flags = PANEL_SLOT_HAS_OBJECT_FLAG;
            if (available) {
                flags |= PANEL_SLOT_ENABLED_FLAG;
            }
            if (state->forced[logicalSlot]) {
                flags |= PANEL_SLOT_FORCED_FLAG;
            }
            setObjectProperty<DWORD>(record, DATA_FLAGS_OFFSET, flags);
            setObjectProperty<int>(
                record,
                DATA_OBJECT_INDEX_OFFSET,
                clientPartyIndex);
            setObjectProperty<int>(
                record,
                DATA_LOGICAL_SLOT_OFFSET,
                logicalSlot);
            setSelected(
                record + DATA_BUTTON_OFFSET,
                selected ? 1 : 0);
        }
    }

    void StagePartySelectionDecisionRecords(
        void* panel,
        PartySelectionViewState* state,
        void* partyTable)
    {
        int enabledCount = 0;
        int forcedCount = 0;
        int selectedCount = 0;
        for (int logicalSlot = 0;
             logicalSlot < MAX_NPC_SLOTS;
             ++logicalSlot) {
            const bool available =
                GetLogicalNPCAvailability(partyTable, logicalSlot) != 0;
            enabledCount += available;
            forcedCount += available && state->forced[logicalSlot];
            selectedCount += available &&
                (state->selected[logicalSlot] || state->forced[logicalSlot]);
        }

        auto setSelected = GameFunction<GuiPartySelectionButtonSetSelectedFn>(
            ADDRESS_PARTY_SELECTION_BUTTON_SET_SELECTED);
        for (int index = 0; index < STOCK_NPC_SLOTS; ++index) {
            BYTE* record = GetStockPartySelectionDataAt(panel, index);
            DWORD flags = PANEL_SLOT_HAS_OBJECT_FLAG;
            if (index < enabledCount) {
                flags |= PANEL_SLOT_ENABLED_FLAG;
            }
            if (index < forcedCount) {
                flags |= PANEL_SLOT_FORCED_FLAG;
            }
            setObjectProperty<DWORD>(record, DATA_FLAGS_OFFSET, flags);
            setSelected(
                record + DATA_BUTTON_OFFSET,
                index < selectedCount ? 1 : 0);
        }
    }

    void PreparePartySelectionModelForAccept(void* panel)
    {
        PartySelectionViewState* state = FindPartySelectionView(panel);
        void* partyTable = GetCurrentPartyTable();
        if (!state || !state->ready || !partyTable) {
            return;
        }
        CapturePartySelectionPage(state);

        bool removedExtendedMember = false;
        for (int logicalSlot = STOCK_NPC_SLOTS;
             logicalSlot < MAX_NPC_SLOTS;
             ++logicalSlot) {
            if (FindActivePartySlot(partyTable, logicalSlot) < 0 ||
                state->selected[logicalSlot] ||
                state->forced[logicalSlot]) {
                continue;
            }
            removedExtendedMember |= RemoveExtendedPartyMember(
                partyTable,
                logicalSlot);
        }
        if (removedExtendedMember) {
            if (void* clientParty = GameFunction<ClientGetPartyFn>(
                    ADDRESS_CLIENT_GET_PARTY)(GetClientApplication())) {
                GameFunction<PartyRecalculateFollowPointFn>(
                    ADDRESS_PARTY_RECALCULATE_FOLLOW_POINT)(clientParty);
            }
        }
        StageStockPartySelectionRecords(panel, state, partyTable);
    }

    void FinishPartySelectionModelAccept(void* panel)
    {
        PartySelectionViewState* state = FindPartySelectionView(panel);
        void* partyTable = GetCurrentPartyTable();
        if (!state || !state->ready || !partyTable) {
            return;
        }
        for (int logicalSlot = STOCK_NPC_SLOTS;
             logicalSlot < MAX_NPC_SLOTS;
             ++logicalSlot) {
            if (!(state->selected[logicalSlot] || state->forced[logicalSlot]) ||
                GetLogicalNPCAvailability(partyTable, logicalSlot) == 0 ||
                FindActivePartySlot(partyTable, logicalSlot) >= 0) {
                continue;
            }
            if (!AddExtendedPartyMember(partyTable, logicalSlot)) {
                state->selected[logicalSlot] = false;
            }
        }
        std::memset(state->forced, 0, sizeof(state->forced));
    }

}  // namespace

extern "C" void __cdecl FinishPartySelectionLayout(void* panel)
{
    CreatePartySelectionView(panel);
    GameFunction<GuiPanelStopLoadFromLayoutFn>(
        ADDRESS_GUI_PANEL_STOP_LOAD_FROM_LAYOUT)(panel);
}

extern "C" void __cdecl PreparePartySelectionDestructor(void* panel)
{
    DestroyPartySelectionView(panel);
}

extern "C" int __cdecl GetNumLogicalNpcAvailable32(void* partyTable)
{
    int count = 0;
    for (int logicalSlot = 0;
         logicalSlot < MAX_NPC_SLOTS;
         ++logicalSlot) {
        count += GetLogicalNPCAvailability(partyTable, logicalSlot) != 0;
    }
    return AVAILABILITY_COUNT_CONSUMED_RESULT_FLAG | (count & 0xFF);
}

extern "C" void __cdecl WriteExtendedPartyPersistenceFromFrame(void* frameBase)
{
    auto* ebp = reinterpret_cast<BYTE*>(frameBase);
    void* gff = *reinterpret_cast<void**>(ebp - 0x3C);
    auto* parent = reinterpret_cast<GffStructRef*>(ebp - 0x2C);

    GffListRef availabilityList{};
    char availabilityListLabel[] = "PT_AVAIL_NPCS";
    if (GameFunction<GffGetListFn>(ADDRESS_GFF_GET_LIST)(
            gff, &availabilityList, parent, availabilityListLabel)) {
        int listCount = std::max(
            0,
            GameFunction<GffGetListCountFn>(ADDRESS_GFF_GET_LIST_COUNT)(
                gff, &availabilityList));
        char availabilityLabel[] = "PT_NPC_AVAIL";
        char selectabilityLabel[] = "PT_NPC_SELECT";
        for (int logicalSlot = STOCK_NPC_SLOTS;
             logicalSlot < MAX_NPC_SLOTS;
             ++logicalSlot) {
            GffStructRef element{};
            if (logicalSlot < listCount) {
                if (!GameFunction<GffGetListElementFn>(
                        ADDRESS_GFF_GET_LIST_ELEMENT)(
                        gff, &element, &availabilityList, logicalSlot)) {
                    continue;
                }
            } else {
                while (listCount <= logicalSlot) {
                    GffStructRef appended{};
                    if (!GameFunction<GffAddListElementFn>(
                            ADDRESS_GFF_ADD_LIST_ELEMENT)(
                            gff, &appended, &availabilityList, 0)) {
                        return;
                    }
                    if (listCount == logicalSlot) {
                        element = appended;
                    }
                    ++listCount;
                }
            }

            const PartySlotSnapshot& slot =
                GetExtendedSlotState(logicalSlot);
            GameFunction<GffWriteFieldByteFn>(
                ADDRESS_GFF_WRITE_FIELD_BYTE)(
                gff,
                &element,
                slot.available != 0 ? 1u : 0u,
                availabilityLabel);
            GameFunction<GffWriteFieldByteFn>(
                ADDRESS_GFF_WRITE_FIELD_BYTE)(
                gff,
                &element,
                slot.selectable != 0 ? 1u : 0u,
                selectabilityLabel);
        }
    }

    GffListRef influenceList{};
    char influenceListLabel[] = "PT_INFLUENCE";
    if (GameFunction<GffGetListFn>(ADDRESS_GFF_GET_LIST)(
            gff, &influenceList, parent, influenceListLabel)) {
        int listCount = std::max(
            0,
            GameFunction<GffGetListCountFn>(ADDRESS_GFF_GET_LIST_COUNT)(
                gff, &influenceList));
        char influenceLabel[] = "PT_NPC_INFLUENCE";
        for (int logicalSlot = STOCK_NPC_SLOTS;
             logicalSlot < MAX_NPC_SLOTS;
             ++logicalSlot) {
            GffStructRef element{};
            if (logicalSlot < listCount) {
                if (!GameFunction<GffGetListElementFn>(
                        ADDRESS_GFF_GET_LIST_ELEMENT)(
                        gff, &element, &influenceList, logicalSlot)) {
                    continue;
                }
            } else {
                while (listCount <= logicalSlot) {
                    GffStructRef appended{};
                    if (!GameFunction<GffAddListElementFn>(
                            ADDRESS_GFF_ADD_LIST_ELEMENT)(
                            gff, &appended, &influenceList, 0)) {
                        return;
                    }
                    if (listCount == logicalSlot) {
                        element = appended;
                    }
                    ++listCount;
                }
            }

            GameFunction<GffWriteFieldIntFn>(ADDRESS_GFF_WRITE_FIELD_INT)(
                gff,
                &element,
                GetExtendedSlotState(logicalSlot).influence,
                influenceLabel);
        }
    }

}

extern "C" void __cdecl ReadExtendedPartyPersistenceFromFrame(void* frameBase)
{
    gExtendedPartySlots = {};
    auto* ebp = reinterpret_cast<BYTE*>(frameBase);
    void* gff = *reinterpret_cast<void**>(ebp - 0x74);
    auto* parent = reinterpret_cast<GffStructRef*>(ebp - 0x64);

    GffListRef availabilityList{};
    char availabilityListLabel[] = "PT_AVAIL_NPCS";
    if (GameFunction<GffGetListFn>(ADDRESS_GFF_GET_LIST)(
            gff, &availabilityList, parent, availabilityListLabel)) {
        const int listCount = std::max(
            0,
            std::min(
                GameFunction<GffGetListCountFn>(ADDRESS_GFF_GET_LIST_COUNT)(
                    gff, &availabilityList),
                MAX_NPC_SLOTS));
        char availabilityLabel[] = "PT_NPC_AVAIL";
        char selectabilityLabel[] = "PT_NPC_SELECT";
        for (int logicalSlot = STOCK_NPC_SLOTS;
             logicalSlot < listCount;
             ++logicalSlot) {
            GffStructRef element{};
            if (!GameFunction<GffGetListElementFn>(
                    ADDRESS_GFF_GET_LIST_ELEMENT)(
                    gff, &element, &availabilityList, logicalSlot)) {
                continue;
            }

            int success = 0;
            PartySlotSnapshot& slot = GetExtendedSlotState(logicalSlot);
            slot.available = GameFunction<GffReadFieldByteFn>(
                ADDRESS_GFF_READ_FIELD_BYTE)(
                gff, &element, availabilityLabel, &success, 0) != 0;
            success = 0;
            slot.selectable = GameFunction<GffReadFieldByteFn>(
                ADDRESS_GFF_READ_FIELD_BYTE)(
                gff, &element, selectabilityLabel, &success, 1) != 0;
        }
    }

    GffListRef influenceList{};
    char influenceListLabel[] = "PT_INFLUENCE";
    if (GameFunction<GffGetListFn>(ADDRESS_GFF_GET_LIST)(
            gff, &influenceList, parent, influenceListLabel)) {
        const int listCount = std::max(
            0,
            std::min(
                GameFunction<GffGetListCountFn>(ADDRESS_GFF_GET_LIST_COUNT)(
                    gff, &influenceList),
                MAX_NPC_SLOTS));
        char influenceLabel[] = "PT_NPC_INFLUENCE";
        for (int logicalSlot = STOCK_NPC_SLOTS;
             logicalSlot < listCount;
             ++logicalSlot) {
            GffStructRef element{};
            if (!GameFunction<GffGetListElementFn>(
                    ADDRESS_GFF_GET_LIST_ELEMENT)(
                    gff, &element, &influenceList, logicalSlot)) {
                continue;
            }

            int success = 0;
            GetExtendedSlotState(logicalSlot).influence =
                GameFunction<GffReadFieldIntFn>(ADDRESS_GFF_READ_FIELD_INT)(
                    gff, &element, influenceLabel, &success, DEFAULT_INFLUENCE);
        }
    }

}

extern "C" void __cdecl PrepareClearPartyMember(
    const int* logicalSlot)
{
    if (IsExtendedSlot(*logicalSlot)) {
        GetExtendedSlotState(*logicalSlot).objectId = NPC_OBJECT_SENTINEL;
    }
}

extern "C" void __cdecl PrepareSavePartyMember(
    const int* logicalSlot,
    const int* saveMode)
{
    if (IsExtendedSlot(*logicalSlot)) {
        SaveExtendedMemberByObjectId(
            *logicalSlot,
            GetExtendedSlotState(*logicalSlot).objectId,
            *saveMode);
    }
}

extern "C" void __cdecl PrepareSavePartyMemberByObject(
    const int* logicalSlot,
    const DWORD* objectId)
{
    if (IsExtendedSlot(*logicalSlot)) {
        SaveExtendedMemberByObjectId(*logicalSlot, *objectId, 1);
    }
}

extern "C" void __cdecl FinalizeSetPartyLeader(void* frameBase)
{
    auto* ebp = reinterpret_cast<BYTE*>(frameBase);
    void* partyTable = *reinterpret_cast<void**>(ebp - 0x08);
    const DWORD objectId = *reinterpret_cast<DWORD*>(ebp + 0x08);
    const int logicalSlot = FindLogicalNPCSlotByObjectId(
        partyTable,
        objectId);
    if (IsExtendedSlot(logicalSlot)) {
        setObjectProperty<int>(
            partyTable,
            OFFSET_PARTY_LEADER_SLOT,
            logicalSlot);
    }
}

extern "C" void __cdecl PrepareSetPartyInfluence(
    const int* logicalSlot,
    const int* value)
{
    if (IsExtendedSlot(*logicalSlot)) {
        GetExtendedSlotState(*logicalSlot).influence = *value;
    }
}

extern "C" void __cdecl StoreLogicalPartyNpcObject(void* frameBase)
{
    auto* ebp = reinterpret_cast<BYTE*>(frameBase);
    SetLogicalNPCObjectId(
        *reinterpret_cast<void**>(ebp - 0x04),
        *reinterpret_cast<int*>(ebp + 0x08),
        *reinterpret_cast<DWORD*>(ebp + 0x0C));
}

extern "C" void __cdecl StoreLogicalPartyNpcSelectability(void* frameBase)
{
    auto* ebp = reinterpret_cast<BYTE*>(frameBase);
    void* partyTable = *reinterpret_cast<void**>(ebp - 0x04);
    const int logicalSlot = *reinterpret_cast<int*>(ebp + 0x08);
    const BYTE value = *reinterpret_cast<BYTE*>(ebp + 0x0C);
    if (IsExtendedSlot(logicalSlot)) {
        GetExtendedSlotState(logicalSlot).selectable = value;
    } else {
        setObjectProperty<BYTE>(
            partyTable,
            OFFSET_NPC_SELECTABLE + logicalSlot,
            value);
    }
}

extern "C" void __cdecl ClearExtendedPartyMembers()
{
    for (PartySlotSnapshot& slot : gExtendedPartySlots) {
        slot.objectId = NPC_OBJECT_SENTINEL;
    }
}

extern "C" void __cdecl ResetExtendedPartySidecar()
{
    gExtendedPartySlots = {};
}

extern "C" void __cdecl PreparePartySelectionOnPanelAdded(void* panel)
{
    PartySelectionViewState* state = FindPartySelectionView(panel);
    if (state) {
        state->pageBase = -1;
    }
    AttachStockPartySelectionControls(panel);
}

extern "C" void __cdecl FinishPartySelectionOnPanelAdded(void* frameBase)
{
    auto* ebp = reinterpret_cast<BYTE*>(frameBase);
    void* panel = *reinterpret_cast<void**>(ebp - 0xDC);
    PartySelectionViewState* state = FindPartySelectionView(panel);
    void* partyTable = GetCurrentPartyTable();
    if (!state || !state->ready || !partyTable) {
        return;
    }
    InitializePartySelectionModel(panel, state, partyTable);
    ShowPartySelectionPage(panel, 0);
}

extern "C" void __cdecl PreparePartySelectionOnPanelRemoved(void* panel)
{
    if (PartySelectionViewState* state = FindPartySelectionView(panel)) {
        CapturePartySelectionPage(state);
    }
    AttachStockPartySelectionControls(panel);
}

extern "C" void __cdecl FinishPartySelectionOnPanelRemoved(void* frameBase)
{
    auto* ebp = reinterpret_cast<BYTE*>(frameBase);
    void* panel = *reinterpret_cast<void**>(ebp - 0x24);
    PartySelectionViewState* state = FindPartySelectionView(panel);
    if (!state) {
        return;
    }
    state->pageBase = -1;
    std::memset(state->selected, 0, sizeof(state->selected));
    std::memset(state->forced, 0, sizeof(state->forced));
    setObjectProperty<DWORD>(panel, PANEL_FORCED_MODE_OFFSET, 0u);
}

extern "C" void __cdecl PrepareSetForcedNPC32(
    void* panel,
    const int* firstLogicalSlot,
    const int* secondLogicalSlot)
{
    PartySelectionViewState* state = FindPartySelectionView(panel);
    if (!state) {
        return;
    }
    const int slots[2] = {*firstLogicalSlot, *secondLogicalSlot};
    bool hasForcedSlot = false;
    for (int logicalSlot : slots) {
        if (!IsValidLogicalSlot(logicalSlot)) {
            continue;
        }
        state->forced[logicalSlot] = true;
        state->selected[logicalSlot] = true;
        hasForcedSlot = true;
    }
    if (hasForcedSlot) {
        setObjectProperty<DWORD>(panel, PANEL_FORCED_MODE_OFFSET, 1u);
    }
}

extern "C" void __cdecl PreparePartySelectionOnDone(void* panel)
{
    PartySelectionViewState* state = FindPartySelectionView(panel);
    void* partyTable = GetCurrentPartyTable();
    if (!state || !state->ready || !partyTable) {
        return;
    }
    CapturePartySelectionPage(state);
    if (getObjectProperty<int>(panel, PANEL_MODE_OFFSET) != 0) {
        StagePartySelectionDecisionRecords(panel, state, partyTable);
    }
}

extern "C" void __cdecl PreparePartySelectionAccept(void* panel)
{
    PreparePartySelectionModelForAccept(panel);
}

extern "C" void __cdecl FinishPartySelectionAccept(void* frameBase)
{
    auto* ebp = reinterpret_cast<BYTE*>(frameBase);
    void* panel = *reinterpret_cast<void**>(ebp - 0x194);
    FinishPartySelectionModelAccept(panel);
}

extern "C" void __cdecl StoreCreatePartyPlayerObjectId(
    void* frameBase,
    DWORD objectId)
{
    auto* ebp = reinterpret_cast<BYTE*>(frameBase);
    void* partyTable = *reinterpret_cast<void**>(ebp - 0x104);
    const int logicalSlot = getObjectProperty<int>(
        partyTable,
        OFFSET_PLAYER_CHARACTER_SLOT);
    SetLogicalNPCObjectId(partyTable, logicalSlot, objectId);
}

extern "C" DWORD __cdecl ResolveCreatePartyMemberObjectId(
    void* frameBase)
{
    auto* ebp = reinterpret_cast<BYTE*>(frameBase);
    return GetLogicalNPCObjectId(
        *reinterpret_cast<void**>(ebp - 0x104),
        *reinterpret_cast<int*>(ebp - 0x18));
}

extern "C" int __cdecl PrepareExtendedNextEquipmentCandidate(
    void* frameBase)
{
    auto* ebp = reinterpret_cast<BYTE*>(frameBase);
    void* equipPanel = *reinterpret_cast<void**>(ebp - 0x58);
    const int logicalSlot = getObjectProperty<int>(
        equipPanel,
        OFFSET_EQUIP_CURRENT_LOGICAL_SLOT);
    if (!IsExtendedSlot(logicalSlot)) {
        return 0;
    }

    void* partyTable = *reinterpret_cast<void**>(ebp - 0x20);
    void* client = GetClientApplication();
    const DWORD objectId = ResolveOrMaterializeLogicalNPCObjectId(
        partyTable,
        logicalSlot);
    void* clientCreature = GetClientCreatureForServerObject(client, objectId);

    *reinterpret_cast<void**>(ebp - 0x18) = clientCreature;
    *reinterpret_cast<int*>(ebp - 0x1C) = clientCreature != nullptr;
    return 1;
}

extern "C" int __cdecl RouteNextEquipmentCandidate(void* frameBase)
{
    auto* ebp = reinterpret_cast<BYTE*>(frameBase);
    return *reinterpret_cast<int*>(ebp - 0x1C) == 0;
}

extern "C" DWORD __cdecl ResolveExperienceFeatMemberObjectId(
    void* frameBase)
{
    auto* ebp = reinterpret_cast<BYTE*>(frameBase);
    return GetPrimaryMemberObjectId(
        *reinterpret_cast<void**>(ebp - 0x68),
        *reinterpret_cast<int*>(ebp - 0x20));
}

extern "C" DWORD __cdecl ResolveExperienceRecipientObjectId(
    void* frameBase)
{
    auto* ebp = reinterpret_cast<BYTE*>(frameBase);
    return GetPrimaryMemberObjectId(
        *reinterpret_cast<void**>(ebp - 0x68),
        *reinterpret_cast<int*>(ebp - 0x28));
}

extern "C" DWORD __cdecl ResolveUnstealthMemberObjectId(
    void* frameBase)
{
    auto* ebp = reinterpret_cast<BYTE*>(frameBase);
    return GetPrimaryMemberObjectId(
        *reinterpret_cast<void**>(ebp - 0x28),
        static_cast<int>(*reinterpret_cast<BYTE*>(ebp - 0x01)));
}

extern "C" DWORD __cdecl ResolveAutoLevelMemberObjectId(
    void* frameBase)
{
    auto* ebp = reinterpret_cast<BYTE*>(frameBase);
    return GetPrimaryMemberObjectId(
        *reinterpret_cast<void**>(ebp - 0x18),
        *reinterpret_cast<int*>(ebp - 0x04));
}

extern "C" DWORD __cdecl ResolveAddMemberDuplicateObjectId(void* frameBase)
{
    auto* ebp = reinterpret_cast<BYTE*>(frameBase);
    return GetLogicalNPCObjectId(
        *reinterpret_cast<void**>(ebp - 0x34),
        *reinterpret_cast<int*>(ebp - 0x04));
}

extern "C" void __cdecl StoreAddMemberObjectId(void* frameBase)
{
    auto* ebp = reinterpret_cast<BYTE*>(frameBase);
    SetLogicalNPCObjectId(
        *reinterpret_cast<void**>(ebp - 0x34),
        *reinterpret_cast<int*>(ebp + 0x08),
        *reinterpret_cast<DWORD*>(ebp + 0x0C));
}

extern "C" void __cdecl ClearLogicalPartyNpcAvailability(void* frameBase)
{
    auto* ebp = reinterpret_cast<BYTE*>(frameBase);
    void* partyTable = *reinterpret_cast<void**>(ebp - 0x04);
    const int logicalSlot = *reinterpret_cast<int*>(ebp + 0x08);
    if (IsStockSlot(logicalSlot)) {
        setObjectProperty<DWORD>(
            partyTable,
            OFFSET_NPC_AVAILABLE +
                logicalSlot * static_cast<int>(sizeof(DWORD)),
            0u);
    } else {
        GetExtendedSlotState(logicalSlot).available = 0u;
    }
}

extern "C" DWORD __cdecl ResolveKillNpcObjectId(void* frameBase)
{
    auto* ebp = reinterpret_cast<BYTE*>(frameBase);
    return GetLogicalNPCObjectId(
        *reinterpret_cast<void**>(ebp - 0x30),
        *reinterpret_cast<int*>(ebp + 0x08));
}

extern "C" void __cdecl ClearKillNpcObjectId(void* frameBase)
{
    auto* ebp = reinterpret_cast<BYTE*>(frameBase);
    SetLogicalNPCObjectId(
        *reinterpret_cast<void**>(ebp - 0x30),
        *reinterpret_cast<int*>(ebp + 0x08),
        NPC_OBJECT_SENTINEL);
}

extern "C" DWORD __cdecl ResolveNextPcMemberObjectId(void* frameBase)
{
    auto* ebp = reinterpret_cast<BYTE*>(frameBase);
    void* partyTable = *reinterpret_cast<void**>(ebp - 0x08);
    const int cursor = getObjectProperty<int>(
        partyTable,
        OFFSET_NEXT_PC_CURSOR);
    return GetPrimaryMemberObjectId(partyTable, cursor);
}

extern "C" DWORD __cdecl ResolveSwitchBackPlayerObjectId(void* frameBase)
{
    auto* ebp = reinterpret_cast<BYTE*>(frameBase);
    void* partyTable = *reinterpret_cast<void**>(ebp - 0xFC);
    return GetLogicalNPCObjectId(
        partyTable,
        getObjectProperty<int>(
            partyTable,
            OFFSET_PLAYER_CHARACTER_SLOT));
}

extern "C" int __cdecl PrepareAddPartyNpc(
    void* partyTable,
    const int* logicalSlot,
    void* const* creature)
{
    if (!IsExtendedSlot(*logicalSlot)) {
        return 0;
    }

    if (*creature && GetExtendedSlotState(*logicalSlot).available == 0) {
        return DirectAddExtendedNPCFromCreature(
            partyTable,
            *logicalSlot,
            *creature);
    }
    return 0;
}

extern "C" int __cdecl FinalizeLogicalPartyNpcId(
    void* frameBase,
    int stockResult)
{
    if (stockResult >= 0) {
        return stockResult;
    }

    auto* ebp = reinterpret_cast<BYTE*>(frameBase);
    const DWORD objectId = *reinterpret_cast<DWORD*>(ebp + 0x08);
    return FindExtendedNPCSlotByObjectId(objectId);
}

extern "C" int __cdecl FinalizeIsLogicalPartyMember(
    void* frameBase,
    int stockResult)
{
    auto* ebp = reinterpret_cast<BYTE*>(frameBase);
    const DWORD objectId = *reinterpret_cast<DWORD*>(ebp + 0x08);
    if (objectId == NPC_OBJECT_SENTINEL) {
        return 0;
    }
    if (stockResult != 0) {
        return stockResult;
    }

    void* partyTable = *reinterpret_cast<void**>(ebp - 0x0C);
    const int slot = FindLogicalNPCSlotByObjectId(partyTable, objectId);
    return IsLogicalSlotPrimaryMember(partyTable, slot);
}

extern "C" int __cdecl FinalizeIsLogicalPartyNpcAvailableByObject(
    void* frameBase,
    int stockResult)
{
    auto* ebp = reinterpret_cast<BYTE*>(frameBase);
    const DWORD objectId = *reinterpret_cast<DWORD*>(ebp + 0x08);
    if (objectId == NPC_OBJECT_SENTINEL) {
        return 0;
    }
    if (stockResult != 0) {
        return stockResult;
    }

    return FindExtendedNPCSlotByObjectId(objectId) >= 0;
}

extern "C" int __cdecl FinalizeIsLogicalPartyLeader(
    void* frameBase,
    int stockResult)
{
    if (stockResult != 0) {
        return stockResult;
    }

    auto* ebp = reinterpret_cast<BYTE*>(frameBase);
    void* partyTable = *reinterpret_cast<void**>(ebp - 0x08);
    const int leaderSlot = getObjectProperty<int>(
        partyTable,
        OFFSET_PARTY_LEADER_SLOT);
    if (!IsExtendedSlot(leaderSlot)) {
        return 0;
    }

    const DWORD objectId = *reinterpret_cast<DWORD*>(ebp + 0x08);
    return FindLogicalNPCSlotByObjectId(partyTable, objectId) == leaderSlot;
}

extern "C" int __cdecl FinalizeGetLogicalPartyNpcAvailability(
    void* frameBase,
    int stockResult)
{
    auto* ebp = reinterpret_cast<BYTE*>(frameBase);
    const int logicalSlot = *reinterpret_cast<int*>(ebp + 0x08);
    return IsExtendedSlot(logicalSlot)
        ? (GetExtendedSlotState(logicalSlot).available != 0)
        : stockResult;
}

extern "C" int __cdecl FinalizeGetLogicalPartyInfluence(
    void* frameBase,
    int stockResult)
{
    auto* ebp = reinterpret_cast<BYTE*>(frameBase);
    const int logicalSlot = *reinterpret_cast<int*>(ebp + 0x08);
    return IsExtendedSlot(logicalSlot)
        ? GetExtendedSlotState(logicalSlot).influence
        : stockResult;
}

extern "C" int __cdecl PrepareGetLogicalPartyNpcObject(void* frameBase)
{
    auto* ebp = reinterpret_cast<BYTE*>(frameBase);
    void* partyTable = *reinterpret_cast<void**>(ebp - 0x60);
    const int logicalSlot = *reinterpret_cast<int*>(ebp + 0x08);
    if (!IsExtendedSlot(logicalSlot)) {
        return 0;
    }

    const int createObject = *reinterpret_cast<int*>(ebp + 0x0C);
    const int loadMode = *reinterpret_cast<int*>(ebp + 0x10);
    const PartySlotSnapshot& state = GetExtendedSlotState(logicalSlot);
    DWORD result = state.objectId;
    if (result == NPC_OBJECT_SENTINEL && createObject != 0) {
        result = LoadExtendedCreatureFromSavedResource(
            partyTable,
            GetServerApplication(),
            logicalSlot,
            loadMode);
    }
    *reinterpret_cast<DWORD*>(ebp - 0x5C) = result;
    return 1;
}

extern "C" int __cdecl PrepareGetLogicalPartyNpcSelectability(
    void* frameBase)
{
    auto* ebp = reinterpret_cast<BYTE*>(frameBase);
    const int logicalSlot = *reinterpret_cast<int*>(ebp + 0x08);
    if (!IsExtendedSlot(logicalSlot)) {
        return 0;
    }

    return SELECTABILITY_CONSUMED_RESULT_FLAG | static_cast<int>(
        GetExtendedSlotState(logicalSlot).selectable);
}

extern "C" void __cdecl FinishRebuildLogicalPartyTable()
{
    void* partyTable = GetCurrentPartyTable();
    for (int logicalSlot = STOCK_NPC_SLOTS;
         logicalSlot < MAX_NPC_SLOTS;
         ++logicalSlot) {
        const PartySlotSnapshot& state = GetExtendedSlotState(logicalSlot);
        if (state.available == 0 || state.objectId != NPC_OBJECT_SENTINEL) {
            continue;
        }

        ResolveOrMaterializeLogicalNPCObjectId(partyTable, logicalSlot);
    }
}
