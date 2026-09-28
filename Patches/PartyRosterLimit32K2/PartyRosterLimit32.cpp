// Implements Vriff's 32-slot roster patch for KOTOR II GOG Aspyr.
// Pairs with kotor2-gog-aspyr.hooks.toml.

#include "Common.h"
#include "GameAPI/CResRef.h"

#include <cstdio>
#include <array>
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
    constexpr int APP_MANAGER_CLIENT_OFFSET = 0x04;
    constexpr int APP_MANAGER_SERVER_OFFSET = 0x08;

    constexpr DWORD ADDRESS_GET_NPC_OBJECT = 0x00700660u;
    constexpr DWORD ADDRESS_ADD_GAME_IN_PROGRESS = 0x00703650u;
    constexpr DWORD ADDRESS_REMOVE_GAME_IN_PROGRESS = 0x007036E0u;

    constexpr DWORD ADDRESS_CLIENT_GET_CREATURE_BY_OBJECT_ID = 0x0040CA10u;

    constexpr DWORD ADDRESS_GFF_READ_FIELD_VOID = 0x006257A0u;
    constexpr DWORD ADDRESS_GFF_WRITE_FIELD_DWORD = 0x00625CF0u;
    constexpr DWORD ADDRESS_GFF_WRITE_FIELD_VOID = 0x00626310u;

    constexpr DWORD PARTY_EXTENSION_MAGIC = 0x344C5250u;  // Encodes "PRL4".
    constexpr DWORD PARTY_EXTENSION_VERSION = 4u;

    constexpr int PARTY_SELECT_PAGE_SLOTS = 16;
    constexpr int PARTY_SELECTION_RECORD_COUNT = MAX_NPC_SLOTS;
    constexpr int LAST_PARTY_SELECT_PAGE_BASE = MAX_NPC_SLOTS - PARTY_SELECT_PAGE_SLOTS;
    constexpr int PARTY_SELECT_SLOT_STRIDE = 0x478;
    constexpr int PANEL_PARTY_SELECTION_DATA_BASE_OFFSET = 0x84;

    // Expands the selection records to two fixed 16-slot pages.
    constexpr DWORD STOCK_PARTY_SELECTION_PANEL_SIZE = 0x50B4u;
    constexpr int PANEL_TAIL_SHIFT = EXTRA_NPC_SLOTS * PARTY_SELECT_SLOT_STRIDE;
    constexpr int PANEL_PAGE_CONTROLS_BASE_OFFSET =
        static_cast<int>(STOCK_PARTY_SELECTION_PANEL_SIZE) + PANEL_TAIL_SHIFT;
    constexpr int PARTY_SELECTION_PAGE_CONTROL_COUNT = 2;
    constexpr int GUI_BUTTON_OBJECT_SIZE = 0x1D0;
    constexpr DWORD EXTENDED_PARTY_SELECTION_PANEL_SIZE =
        PANEL_PAGE_CONTROLS_BASE_OFFSET +
        PARTY_SELECTION_PAGE_CONTROL_COUNT * GUI_BUTTON_OBJECT_SIZE;

    // Relies on normal construction and destruction for all 32 records.
    constexpr int DATA_BUTTON_OFFSET = 0x04;
    // Defines the native unavailable- and portrait-label offsets.
    constexpr int DATA_NOT_AVAILABLE_LABEL_OFFSET = 0x1D8;
    constexpr int DATA_CHARACTER_LABEL_OFFSET = 0x320;
    constexpr int DATA_FLAGS_OFFSET = 0x468;
    constexpr std::array<int, 3> PARTY_SELECTION_RECORD_CONTROL_OFFSETS = {
        DATA_CHARACTER_LABEL_OFFSET,
        DATA_BUTTON_OFFSET,
        DATA_NOT_AVAILABLE_LABEL_OFFSET,
    };
    constexpr int PANEL_FORCED_MODE_OFFSET = 0x70;
    constexpr int PANEL_CURRENT_PORTRAIT_OFFSET = 0x48F8 + PANEL_TAIL_SHIFT;
    constexpr int PANEL_ACCEPT_BUTTON_OFFSET = 0x4728 + PANEL_TAIL_SHIFT;
    constexpr int GUI_NAVIGATION_UP = 0;
    constexpr std::size_t GUI_BUTTON_SET_ACTIVE_VTABLE_INDEX = 0x40 / sizeof(void*);
    constexpr DWORD PANEL_SLOT_ENABLED_FLAG = 0x1u;
    constexpr DWORD PANEL_SLOT_FORCED_FLAG = 0x4u;
    constexpr int GUI_CONTROL_FLAGS_OFFSET = 0x48;
    constexpr DWORD GUI_CONTROL_VISIBLE_FLAG = 0x2u;
    constexpr DWORD GUI_CONTROL_MOUSE_IGNORE_FLAG = 0x20u;
    constexpr int GUI_CONTROL_CLICK_EVENT = 0x27;
    constexpr int SELECTABILITY_CONSUMED_RESULT_FLAG = 0x100;

    constexpr DWORD ADDRESS_GET_CURRENT_PARTY_TABLE = 0x0064C2F0u;

    // Keeps native footer callbacks and adds dedicated page controls.
    constexpr int PARTY_SELECTION_PAGE_PREV_INDEX = 0;
    constexpr int PARTY_SELECTION_PAGE_NEXT_INDEX = 1;
    constexpr DWORD ADDRESS_GUI_PANEL_STOP_LOAD_FROM_LAYOUT = 0x0090DA20u;
    constexpr DWORD ADDRESS_PARTY_SELECTION_BUTTON_SET_SELECTED = 0x0058A930u;
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
    constexpr DWORD ADDRESS_CEXOSTRING_CTOR_CSTR = 0x00605680u;
    constexpr DWORD ADDRESS_CEXOSTRING_DTOR = 0x00605890u;
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

    // Requires partyselect_p.gui with 32 records and page controls.

    struct PartySlotSnapshot
    {
        DWORD objectId = NPC_OBJECT_SENTINEL;
        DWORD available = 0;
        BYTE selectable = 1;
        int influence = DEFAULT_INFLUENCE;
    };

    struct ExtendedPartyTableState
    {
        std::array<PartySlotSnapshot, EXTRA_NPC_SLOTS> slots{};
        int leaderSlot = -1;
    };

#pragma pack(push, 1)
    struct PersistedPartySlot
    {
        DWORD available;
        int influence;
        BYTE selectable;
        BYTE reserved;
    };

    struct PersistedPartyExtension
    {
        DWORD magic;
        int leaderSlot;
        PersistedPartySlot slots[EXTRA_NPC_SLOTS];
    };
#pragma pack(pop)

    ExtendedPartyTableState gExtendedPartyState;

    using GetNPCObjectFn = DWORD(__thiscall*)(void*, int, int, int);
    using AddGameInProgressFn = void(__thiscall*)(void*);
    using RemoveGameInProgressFn = void(__thiscall*)(void*);
    using ClientGetCreatureByObjectIdFn = void* (__thiscall*)(void*, DWORD);
    using GffReadFieldVoidFn = void* (__thiscall*)(void*, void*, void*, DWORD, char*, int*, void*);
    using GffWriteFieldDwordFn = int(__thiscall*)(void*, void*, DWORD, char*);
    using GffWriteFieldVoidFn = int(__thiscall*)(void*, void*, const void*, DWORD, char*);
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
        return gExtendedPartyState.slots[
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

    BYTE* GetPartySelectionDataAt(void* panel, int logicalSlot)
    {
        return reinterpret_cast<BYTE*>(panel) + PANEL_PARTY_SELECTION_DATA_BASE_OFFSET +
               logicalSlot * PARTY_SELECT_SLOT_STRIDE;
    }

    BYTE* GetPartySelectionPageControlAt(void* panel, int controlIndex)
    {
        return reinterpret_cast<BYTE*>(panel) + PANEL_PAGE_CONTROLS_BASE_OFFSET +
               controlIndex * GUI_BUTTON_OBJECT_SIZE;
    }

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

    // Disables hit testing on visual-only portrait labels.
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

        // Preserves native-first logical slot ordering.
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
        auto getPartyTable = GameFunction<GetCurrentPartyTableFn>(ADDRESS_GET_CURRENT_PARTY_TABLE);
        return getPartyTable(server);
    }

    void* GetServerCreatureByObjectId(void* server, DWORD objectId)
    {
        if (objectId == NPC_OBJECT_SENTINEL) {
            return nullptr;
        }
        auto getCreature = GameFunction<GetCreatureByObjectIdFn>(ADDRESS_GET_CREATURE_BY_OBJECT_ID);
        return getCreature(server, objectId);
    }

    void* GetCurrentPlayerCreature(void* server)
    {
        auto getPlayerId = GameFunction<ServerGetPlayerCreatureIdFn>(
            ADDRESS_SERVER_GET_PLAYER_CREATURE_ID);
        return GetServerCreatureByObjectId(server, getPlayerId(server));
    }

    void* GetClientCreatureForServerObject(void* client, DWORD serverObjectId)
    {
        if (serverObjectId == NPC_OBJECT_SENTINEL) {
            return nullptr;
        }
        auto toClientId = GameFunction<ServerToClientObjectIdFn>(
            ADDRESS_SERVER_TO_CLIENT_OBJECT_ID);
        auto getClientCreature = GameFunction<ClientGetCreatureByObjectIdFn>(
            ADDRESS_CLIENT_GET_CREATURE_BY_OBJECT_ID);
        return getClientCreature(client, toClientId(client, serverObjectId));
    }

    void __fastcall PartySelectionPrevButtonCallback(void* self, void*, void*);
    void __fastcall PartySelectionNextButtonCallback(void* self, void*, void*);

    using PartySelectionPagingCallback = void(__fastcall*)(void*, void*, void*);

    struct PartySelectionPagingButtonSpec
    {
        const char* tag;
        PartySelectionPagingCallback callback;
    };

    constexpr std::array<PartySelectionPagingButtonSpec,
                         PARTY_SELECTION_PAGE_CONTROL_COUNT>
        PARTY_SELECTION_PAGING_BUTTONS = {{
            {"BTN_PAGE_PREV", &PartySelectionPrevButtonCallback},
            {"BTN_PAGE_NEXT", &PartySelectionNextButtonCallback},
        }};

    void AttachPanelControl(void* panel, void* control)
    {
        auto addControl = GameFunction<GuiPanelAddControlFn>(ADDRESS_GUI_PANEL_ADD_CONTROL);
        addControl(panel, control);
    }

    void DetachAllPartySelectionPageControls(void* panel)
    {
        // Detaches controls without destroying their record state.
        auto delIndex = GameFunction<ExoArrayListPointerDelIndexFn>(
            ADDRESS_EXO_ARRAY_LIST_POINTER_DEL_INDEX);
        for (int logicalSlot = 0;
             logicalSlot < PARTY_SELECTION_RECORD_COUNT;
             ++logicalSlot) {
            BYTE* data = GetPartySelectionDataAt(panel, logicalSlot);
            for (int controlOffset : PARTY_SELECTION_RECORD_CONTROL_OFFSETS) {
                void** controls = getObjectProperty<void**>(
                    panel,
                    GUI_PANEL_CONTROL_ARRAY_OFFSET);
                const int count = getObjectProperty<int>(
                    panel,
                    GUI_PANEL_CONTROL_COUNT_OFFSET);

                void* control = data + controlOffset;
                for (int index = 0; index < count; ++index) {
                    if (controls[index] == control) {
                        delIndex(
                            reinterpret_cast<BYTE*>(panel) +
                                GUI_PANEL_CONTROL_ARRAY_OFFSET,
                            index);
                        break;
                    }
                }
            }
        }
    }

    void AttachAllPartySelectionControlsForStockLifecycle(void* panel)
    {
        DetachAllPartySelectionPageControls(panel);

        // Reattaches every control in native layer order for cleanup.
        for (int controlOffset : PARTY_SELECTION_RECORD_CONTROL_OFFSETS) {
            for (int logicalSlot = 0;
                 logicalSlot < PARTY_SELECTION_RECORD_COUNT;
                 ++logicalSlot) {
                AttachPanelControl(panel,
                                   GetPartySelectionDataAt(panel, logicalSlot) +
                                       controlOffset);
            }
        }
    }

    void ResetPartySelectionControlState(void* panel)
    {
        auto setActiveControl = GameFunction<GuiPanelSetActiveControlFn>(
            ADDRESS_GUI_PANEL_SET_ACTIVE_CONTROL);
        auto setEnabled = GameFunction<GuiButtonSetEnabledFn>(ADDRESS_GUI_BUTTON_SET_ENABLED);
        auto setSelected = GameFunction<GuiPartySelectionButtonSetSelectedFn>(
            ADDRESS_PARTY_SELECTION_BUTTON_SET_SELECTED);
        setActiveControl(panel, nullptr, 0);

        for (int logicalSlot = 0;
             logicalSlot < PARTY_SELECTION_RECORD_COUNT;
             ++logicalSlot) {
            BYTE* data = GetPartySelectionDataAt(panel, logicalSlot);
            BYTE* button = data + DATA_BUTTON_OFFSET;
            BYTE* character = data + DATA_CHARACTER_LABEL_OFFSET;
            BYTE* unavailable = data + DATA_NOT_AVAILABLE_LABEL_OFFSET;

            // Restores constructor state before native population.
            SetControlVisible(button, true);
            SetControlHitTestEnabled(button, true);
            setEnabled(button, 1);
            SetControlVisible(character, false);
            SetControlVisible(unavailable, false);
            SetControlHitTestEnabled(character, false);
            SetControlHitTestEnabled(unavailable, false);
            setSelected(data + DATA_BUTTON_OFFSET, 0);
        }
    }

    void ShowPartySelectionPage(void* panel, int pageBase)
    {
        const int pageEnd = pageBase + PARTY_SELECT_PAGE_SLOTS;
        auto setEnabled = GameFunction<GuiButtonSetEnabledFn>(ADDRESS_GUI_BUTTON_SET_ENABLED);
        BYTE* prev = GetPartySelectionPageControlAt(panel, PARTY_SELECTION_PAGE_PREV_INDEX);
        BYTE* next = GetPartySelectionPageControlAt(panel, PARTY_SELECTION_PAGE_NEXT_INDEX);
        // Enables the fallback page control before focusing it.
        setEnabled(prev, pageBase == LAST_PARTY_SELECT_PAGE_BASE);
        setEnabled(next, pageBase == 0);

        auto setActiveControl = GameFunction<GuiPanelSetActiveControlFn>(
            ADDRESS_GUI_PANEL_SET_ACTIVE_CONTROL);
        // Clears preview focus left by OnExit.
        setActiveControl(panel, nullptr, 0);
        auto* previous = getObjectProperty<BYTE*>(panel, PANEL_CURRENT_PORTRAIT_OFFSET);
        if (previous) {
            BYTE* button = previous + DATA_BUTTON_OFFSET;
            void** vtable = *reinterpret_cast<void***>(button);
            reinterpret_cast<GuiButtonSetActiveFn>(
                vtable[GUI_BUTTON_SET_ACTIVE_VTABLE_INDEX])(button, 0);
        }
        setObjectProperty<void*>(panel, PANEL_CURRENT_PORTRAIT_OFFSET, nullptr);
        DetachAllPartySelectionPageControls(panel);

        // Attaches visual portrait labels behind interactive controls.
        for (int logicalSlot = pageBase; logicalSlot < pageEnd; ++logicalSlot) {
            BYTE* character = GetPartySelectionDataAt(panel, logicalSlot) +
                              DATA_CHARACTER_LABEL_OFFSET;
            SetControlHitTestEnabled(character, false);
            AttachPanelControl(panel, character);
        }

        // Applies recorded button eligibility only on the active page.
        void* firstInteractive = nullptr;
        for (int logicalSlot = pageBase; logicalSlot < pageEnd; ++logicalSlot) {
            BYTE* data = GetPartySelectionDataAt(panel, logicalSlot);
            BYTE* button = data + DATA_BUTTON_OFFSET;
            const DWORD flags = getObjectProperty<DWORD>(data, DATA_FLAGS_OFFSET);
            const bool interactive =
                (flags & PANEL_SLOT_ENABLED_FLAG) != 0 &&
                (flags & PANEL_SLOT_FORCED_FLAG) == 0;
            setEnabled(button, interactive);
            SetControlHitTestEnabled(button, interactive);
            AttachPanelControl(panel, button);
            if (!firstInteractive && interactive) {
                firstInteractive = button;
            }
        }

        // Hides empty extended-slot plates while preserving stock plates.
        for (int logicalSlot = pageBase; logicalSlot < pageEnd; ++logicalSlot) {
            BYTE* unavailable = GetPartySelectionDataAt(panel, logicalSlot) +
                                DATA_NOT_AVAILABLE_LABEL_OFFSET;
            const bool unavailableSlot =
                (getObjectProperty<DWORD>(unavailable, GUI_CONTROL_FLAGS_OFFSET) &
                 GUI_CONTROL_VISIBLE_FLAG) != 0;
            if (logicalSlot >= STOCK_NPC_SLOTS && unavailableSlot) {
                continue;
            }
            SetControlHitTestEnabled(unavailable, unavailableSlot);
            AttachPanelControl(panel, unavailable);
        }

        BYTE* accept = reinterpret_cast<BYTE*>(panel) + PANEL_ACCEPT_BUTTON_OFFSET;
        // Rebinds footer navigation to an attached portrait.
        GameFunction<GuiSetMoveToControlFn>(ADDRESS_GUI_SET_MOVE_TO_CONTROL)(
            accept, GUI_NAVIGATION_UP, firstInteractive);
        setEnabled(accept, firstInteractive != nullptr);
        // Focuses the alternate page control when no portrait is selectable.
        setActiveControl(panel,
            firstInteractive ? firstInteractive : (pageBase == 0 ? next : prev), 1);
    }

    void __fastcall PartySelectionPrevButtonCallback(void* self, void*, void*)
    {
        ShowPartySelectionPage(self, 0);
    }

    void __fastcall PartySelectionNextButtonCallback(void* self, void*, void*)
    {
        ShowPartySelectionPage(self, LAST_PARTY_SELECT_PAGE_BASE);
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

    void SaveCreatureToExtendedResource(int logicalSlot, void* creature)
    {
        void* gff = AllocateEngineObject(RESGFF_OBJECT_SIZE);
        char outputPath[32];
        std::snprintf(outputPath, sizeof(outputPath), "GAMEINPROGRESS:slot%02d", logicalSlot);
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
        GameFunction<CResGffWriteFileFn>(ADDRESS_CRESGFF_WRITE_FILE)(
            gff, &path, SAVED_CREATURE_RESOURCE_TYPE);
        DeleteEngineObject(gff);
        stringDtor(&path);
        stringDtor(&fileType);
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
        auto getFaction = GameFunction<CreatureGetFactionFn>(ADDRESS_CREATURE_GET_FACTION);
        void* faction = getFaction(playerCreature);
        auto addMember = GameFunction<FactionAddMemberFn>(ADDRESS_FACTION_ADD_MEMBER);
        addMember(faction, getObjectProperty<DWORD>(creature, GAME_OBJECT_ID_OFFSET), 0);
    }

    void ApplyExtendedRecruitmentExperience(void* partyTable, void* creature)
    {
        // Uses a neutral XP multiplier beyond the native table.
        const int targetExperience = getObjectProperty<int>(
            partyTable,
            PARTY_TARGET_EXPERIENCE_OFFSET);
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

        auto transferInventory = GameFunction<TransferInventoryFn>(ADDRESS_TRANSFER_INVENTORY);
        transferInventory(partyTable, creature);
        ApplyExtendedRecruitmentExperience(partyTable, creature);
        AddCreatureToPlayerFaction(creature);

        void* stats = getObjectProperty<void*>(creature, CREATURE_STATS_OFFSET);
        auto setMovementRate = GameFunction<CreatureStatsSetMovementRateFn>(
            ADDRESS_CREATURE_STATS_SET_MOVEMENT_RATE);
        setMovementRate(stats, 0);
        SaveCreatureToExtendedResource(logicalSlot, creature);

        state.available = 1;
        state.objectId = getObjectProperty<DWORD>(creature, GAME_OBJECT_ID_OFFSET);
        auto handleInfluence = GameFunction<CreatureHandleAlignmentInfluenceFn>(
            ADDRESS_CREATURE_HANDLE_ALIGNMENT_INFLUENCE);
        handleInfluence(creature);

        state.objectId = NPC_OBJECT_SENTINEL;
        return 1;
    }

    DWORD LoadExtendedCreatureFromSavedResource(void* partyTable,
                                                 void* server,
                                                 int logicalSlot,
                                                 int applyRecovery)
    {
        void* creature = AllocateEngineObject(CREATURE_OBJECT_SIZE);
        GameFunction<CreatureCtorFn>(ADDRESS_CREATURE_CTOR)(creature, NPC_OBJECT_SENTINEL, 0);
        char identity[16];
        std::snprintf(identity, sizeof(identity), "slot%02d", logicalSlot);
        GameFunction<AddGameInProgressFn>(ADDRESS_ADD_GAME_IN_PROGRESS)(partyTable);
        {
            CResRef identityRef(static_cast<const char*>(identity));
            GameFunction<CreatureLoadFromTemplateFn>(ADDRESS_CREATURE_LOAD_FROM_TEMPLATE)(
                creature, identityRef.GetPtr(), 0);
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

}  // namespace

extern "C" void __cdecl FinishPartySelectionLayout(void* panel)
{
    // Finishes page-control setup before releasing the layout GFF.
    std::memset(reinterpret_cast<BYTE*>(panel) + PANEL_PAGE_CONTROLS_BASE_OFFSET,
                0,
                EXTENDED_PARTY_SELECTION_PANEL_SIZE - PANEL_PAGE_CONTROLS_BASE_OFFSET);

    auto buttonCtor = GameFunction<GuiButtonCtorFn>(ADDRESS_GUI_BUTTON_CTOR);
    auto stringCtor = GameFunction<CExoStringCtorCStrFn>(ADDRESS_CEXOSTRING_CTOR_CSTR);
    auto stringDtor = GameFunction<CExoStringDtorFn>(ADDRESS_CEXOSTRING_DTOR);
    auto initControl = GameFunction<GuiPanelInitControlFn>(ADDRESS_GUI_PANEL_INIT_CONTROL);
    auto addEvent = GameFunction<GuiControlAddEventFn>(ADDRESS_GUI_CONTROL_ADD_EVENT);

    for (std::size_t i = 0; i < PARTY_SELECTION_PAGING_BUTTONS.size(); ++i) {
        BYTE* button = GetPartySelectionPageControlAt(
            panel,
            static_cast<int>(i));
        RawCExoString tagString;
        buttonCtor(button);
        stringCtor(&tagString, PARTY_SELECTION_PAGING_BUTTONS[i].tag);
        initControl(panel, button, &tagString, 1, 1);
        stringDtor(&tagString);
        addEvent(button,
                 GUI_CONTROL_CLICK_EVENT,
                 panel,
                 funcAddr(PARTY_SELECTION_PAGING_BUTTONS[i].callback));
    }

    GameFunction<GuiPanelStopLoadFromLayoutFn>(ADDRESS_GUI_PANEL_STOP_LOAD_FROM_LAYOUT)(panel);
}

extern "C" void __cdecl PreparePartySelectionDestructor(void* panel)
{
    // Destroys only the two patch-owned page buttons.
    AttachAllPartySelectionControlsForStockLifecycle(panel);
    auto buttonDtor = GameFunction<GuiButtonDtorFn>(ADDRESS_GUI_BUTTON_DTOR);
    for (int i = PARTY_SELECTION_PAGE_CONTROL_COUNT - 1; i >= 0; --i) {
        buttonDtor(GetPartySelectionPageControlAt(panel, i));
    }
}

extern "C" void __cdecl WriteExtendedPartyPersistenceFromFrame(void* frameBase)
{
    auto* ebp = reinterpret_cast<BYTE*>(frameBase);
    void* gff = *reinterpret_cast<void**>(ebp - 0x3C);
    void* parentStruct = reinterpret_cast<void*>(ebp - 0x2C);
    PersistedPartyExtension blob;
    blob.magic = PARTY_EXTENSION_MAGIC;
    blob.leaderSlot = gExtendedPartyState.leaderSlot;
    for (std::size_t i = 0; i < gExtendedPartyState.slots.size(); ++i) {
        const PartySlotSnapshot& slot = gExtendedPartyState.slots[i];
        blob.slots[i] = {slot.available, slot.influence, slot.selectable, 1};
    }

    char versionLabel[] = "PRL32_VER";
    char dataLabel[] = "PRL32_DATA";
    GameFunction<GffWriteFieldDwordFn>(ADDRESS_GFF_WRITE_FIELD_DWORD)(
        gff, parentStruct, PARTY_EXTENSION_VERSION, versionLabel);
    GameFunction<GffWriteFieldVoidFn>(ADDRESS_GFF_WRITE_FIELD_VOID)(
        gff, parentStruct, &blob, sizeof(blob), dataLabel);

}

extern "C" void __cdecl ReadExtendedPartyPersistenceFromFrame(void* frameBase)
{
    auto* ebp = reinterpret_cast<BYTE*>(frameBase);
    void* gff = *reinterpret_cast<void**>(ebp - 0x74);
    void* parentStruct = reinterpret_cast<void*>(ebp - 0x64);
    char dataLabel[] = "PRL32_DATA";
    PersistedPartyExtension blob;
    int blobSuccess;
    GameFunction<GffReadFieldVoidFn>(ADDRESS_GFF_READ_FIELD_VOID)(
        gff, parentStruct, &blob, sizeof(blob), dataLabel, &blobSuccess, nullptr);
    if (!blobSuccess) {
        return;  // Keeps reset state for saves without extension data.
    }

    for (std::size_t i = 0; i < gExtendedPartyState.slots.size(); ++i) {
        const PersistedPartySlot& persisted = blob.slots[i];
        PartySlotSnapshot& slot = gExtendedPartyState.slots[i];
        slot.objectId = NPC_OBJECT_SENTINEL;
        slot.available = persisted.available;
        slot.selectable = persisted.selectable;
        slot.influence = persisted.influence;
    }
    gExtendedPartyState.leaderSlot = blob.leaderSlot;

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

extern "C" void __cdecl PrepareSetPartyLeader(
    void* partyTable,
    const DWORD* objectId)
{
    const int logicalSlot = FindLogicalNPCSlotByObjectId(
        partyTable,
        *objectId);
    gExtendedPartyState.leaderSlot =
        IsExtendedSlot(logicalSlot) ? logicalSlot : -1;
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
    for (PartySlotSnapshot& slot : gExtendedPartyState.slots) {
        slot.objectId = NPC_OBJECT_SENTINEL;
    }
}

extern "C" void __cdecl ResetExtendedPartySidecar()
{
    gExtendedPartyState = {};
}

extern "C" void __cdecl PreparePartySelectionOnPanelAdded(void* panel)
{
    AttachAllPartySelectionControlsForStockLifecycle(panel);
    ResetPartySelectionControlState(panel);
}

extern "C" void __cdecl FinishPartySelectionOnPanelAdded(void* frameBase)
{
    auto* ebp = reinterpret_cast<BYTE*>(frameBase);
    void* panel = *reinterpret_cast<void**>(ebp - 0xDC);
    ShowPartySelectionPage(panel, 0);
}

extern "C" void __cdecl PreparePartySelectionOnPanelRemoved(void* panel)
{
    AttachAllPartySelectionControlsForStockLifecycle(panel);
}

extern "C" void __cdecl FinishPartySelectionOnPanelRemoved(void* frameBase)
{
    auto* ebp = reinterpret_cast<BYTE*>(frameBase);
    void* panel = *reinterpret_cast<void**>(ebp - 0x24);
    for (int logicalSlot = 0;
         logicalSlot < PARTY_SELECTION_RECORD_COUNT;
         ++logicalSlot) {
        BYTE* data = GetPartySelectionDataAt(panel, logicalSlot);
        const DWORD flags = getObjectProperty<DWORD>(data, DATA_FLAGS_OFFSET) &
                            ~PANEL_SLOT_FORCED_FLAG;
        setObjectProperty<DWORD>(data, DATA_FLAGS_OFFSET, flags);
    }
    setObjectProperty<DWORD>(panel, PANEL_FORCED_MODE_OFFSET, 0u);

    ResetPartySelectionControlState(panel);
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
    if (IsStockSlot(getObjectProperty<int>(
            partyTable,
            OFFSET_PARTY_LEADER_SLOT))) {
        return 0;
    }

    const DWORD objectId = *reinterpret_cast<DWORD*>(ebp + 0x08);
    const int slot = FindLogicalNPCSlotByObjectId(partyTable, objectId);
    return IsExtendedSlot(slot) && slot == gExtendedPartyState.leaderSlot;
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
