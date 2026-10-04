#include "Common.h"
#include "GameAPI/GameVersion.h"
#include "GameAPI/CAppManager.h"
#include "GameAPI/CClientExoApp.h"
#include "GameAPI/CServerExoApp.h"
#include "GameAPI/CGameObject.h"
#include "GameAPI/CResGFF.h"
#include "GameAPI/CResRef.h"
#include "GameAPI/CSWSCreature.h"
#include "GameAPI/CSWCCreature.h"
#include "GameAPI/CSWGuiBorderParams.h"
#include "GameAPI/CSWGuiButton.h"
#include "GameAPI/CSWGuiControl.h"
#include "GameAPI/CSWGuiPanel.h"

#include <algorithm>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>

namespace
{
    constexpr int STOCK_NPC_SLOTS = 9;
    constexpr int MAX_NPC_SLOTS = 32;
    constexpr int EXTRA_NPC_SLOTS = MAX_NPC_SLOTS - STOCK_NPC_SLOTS;
    constexpr int PRIMARY_MEMBER_CAPACITY = 2;

    // Loads stock layouts from the selected KPM address database.
    int OFFSET_PARTY_MEMBER_COUNT = -1;
    int OFFSET_PARTY_MEMBER_SLOTS = -1;
    int OFFSET_NPC_OBJECT_IDS = -1;
    int OFFSET_NPC_AVAILABLE = -1;
    int OFFSET_NPC_SELECTABLE = -1;
    int OFFSET_PARTY_LEADER_SLOT = -1;
    int OFFSET_PLAYER_CHARACTER_SLOT = -1;

    constexpr DWORD NPC_OBJECT_SENTINEL = 0x7F000000u;

    // Defines build-specific globals that are absent from the database.
    constexpr DWORD ADDRESS_EFFECT_TYPE_WORD = 0x0074668Cu;
    constexpr DWORD ADDRESS_EFFECT_SUBTYPE_WORD = 0x007473B8u;

    unsigned int CREATURE_OBJECT_SIZE = 0;
    unsigned int RESGFF_OBJECT_SIZE = 0;
    unsigned int GAME_EFFECT_OBJECT_SIZE = 0;
    constexpr unsigned short SAVED_CREATURE_RESOURCE_TYPE = 0x07EBu;
    int CREATURE_POST_LOAD_STATE_OFFSET = -1;
    constexpr int CLIENT_CREATURE_DEFERRED_DELETE_OFFSET = 0xE4;
    int GAME_EFFECT_TYPE_OFFSET = -1;
    int GAME_EFFECT_SUBTYPE_FLAGS_OFFSET = -1;
    int OBJECT_POSITION_OFFSET = -1;
    int OBJECT_ORIENTATION_OFFSET = -1;
    int CREATURE_PATHFIND_INFO_OFFSET = -1;
    constexpr WORD GAME_EFFECT_SUBTYPE_MASK = 0x7u;

    constexpr int PARTY_SELECT_PAGE_SLOTS = 16;
    constexpr int LAST_PARTY_SELECT_PAGE_BASE =
        MAX_NPC_SLOTS - PARTY_SELECT_PAGE_SLOTS;
    constexpr int PARTY_SELECTION_VIEW_RECORD_COUNT = PARTY_SELECT_PAGE_SLOTS;
    constexpr int PARTY_SELECTION_PAGE_CONTROL_COUNT = 2;
    constexpr int PARTY_SELECTION_STATE_CAPACITY = 2;
    int PARTY_SELECT_SLOT_STRIDE = -1;
    int PANEL_PARTY_SELECTION_DATA_BASE_OFFSET = -1;
    int GUI_BUTTON_OBJECT_SIZE = -1;

    int DATA_BUTTON_OFFSET = -1;
    int DATA_NOT_AVAILABLE_LABEL_OFFSET = -1;
    int DATA_CHARACTER_LABEL_OFFSET = -1;
    constexpr int DATA_BUTTON_BACK_POINTER_OFFSET = 0x58;
    constexpr int DATA_BUTTON_SELECTED_OFFSET = 0x1C4;
    constexpr int DATA_BUTTON_INIT_PARAMS_OFFSET = 0xF4;
    constexpr int DATA_DEFAULT_COLOR_OFFSET = 0x104;
    constexpr int DATA_CHARACTER_TEXTURE_PARAMS_OFFSET = 0x378;
    constexpr int DATA_ALPHA_OFFSET = 0x8C;
    constexpr int DATA_SELECTION_ALPHA_OFFSET = 0x384;
    constexpr int DATA_FLAGS_OFFSET = 0x448;
    constexpr int DATA_OBJECT_INDEX_OFFSET = 0x44C;
    constexpr int DATA_LOGICAL_SLOT_OFFSET = 0x450;
    constexpr DWORD PANEL_SLOT_ENABLED_FLAG = 0x1u;
    constexpr DWORD PANEL_SLOT_HAS_OBJECT_FLAG = 0x2u;
    constexpr DWORD PANEL_SLOT_FORCED_FLAG = 0x4u;
    constexpr int PANEL_SELECTED_COUNT_OFFSET = 0x68;
    constexpr int PANEL_FORCED_MODE_OFFSET = 0x6C;
    constexpr int PANEL_MODE_OFFSET = 0x70;
    int PANEL_DONE_BUTTON_OFFSET = -1;
    int PANEL_CURRENT_PORTRAIT_OFFSET = -1;
    constexpr int GUI_NAVIGATION_UP = 0;
    constexpr int GUI_NAVIGATION_DOWN = 2;
    constexpr int GUI_CONTROL_PRIMARY_EVENT = 0x27;
    constexpr int GUI_CONTROL_CLICK_EVENT = 0x2D;
    constexpr DWORD ADDRESS_PARTY_SELECTION_HOVER_HANDLER = 0x0060E760u;
    constexpr int GUI_CONTROL_EVENT_OWNER_OFFSET = 0x4C;

    int PARTY_SELECTION_RECORD_CONTROL_OFFSETS[3] = {};

    struct PartySlotSnapshot
    {
        DWORD objectId;
        DWORD available;
        BYTE selectable;
    };


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

    PartySlotSnapshot gExtendedPartySlots[EXTRA_NPC_SLOTS];
    PartySelectionViewState gPartySelectionViews[PARTY_SELECTION_STATE_CAPACITY] = {};

    typedef DWORD RawCExoString[2];

    using GetCreatureByObjectIdFn = void* (__thiscall*)(void*, DWORD);
    using AddGameInProgressFn = void(__thiscall*)(void*);
    using RemoveGameInProgressFn = void(__thiscall*)(void*);
    using TransferInventoryFn = void(__thiscall*)(void*, void*);
    using OperatorNewFn = void* (__cdecl*)(unsigned int);
    using CreatureCtorFn = void* (__thiscall*)(void*, DWORD, int);
    using CreatureLoadFromTemplateFn = int(__thiscall*)(void*, const void*, int);
    using CreatureSaveCreatureFn = int(__thiscall*)(void*, void*, void*);
    using CreatureForceEquipClothingFn = void(__thiscall*)(void*);
    using ClientObjectSetFadeStateFn = void(__thiscall*)(void*, BYTE);
    using ClientObjectSetDesiredFadeStateFn =
        void(__thiscall*)(void*, BYTE, int, DWORD, DWORD);
    using CExoStringCtorCStrFn = void* (__thiscall*)(void*, const char*);
    using CExoStringDtorFn = void(__thiscall*)(void*);
    using CResGffCtorFn = void* (__thiscall*)(void*);
    using CResGffCreateFileFn =
        int(__thiscall*)(void*, void*, const void*, const void*);
    using CResGffWriteFileFn =
        int(__thiscall*)(void*, const void*, unsigned short);
    using ScalarDeletingDestructorFn = void* (__thiscall*)(void*, unsigned int);
    using ObjectStatusVirtualFn = short(__thiscall*)(void*, int);
    using GameEffectCtorFn = void* (__thiscall*)(void*, int);
    using ObjectApplyEffectFn = void(__thiscall*)(void*, void*, int, int);
    using GuiSetMoveToControlFn = void(__thiscall*)(void*, int, void*);
    using GuiButtonCtorFn = void* (__thiscall*)(void*);
    using GuiButtonDtorFn = void(__thiscall*)(void*);
    using GuiPartySelectionButtonSetSelectedFn =
        void(__thiscall*)(void*, int);
    using PartySelectionRecordCtorFn = void* (__thiscall*)(void*);
    using PartySelectionRecordDtorFn = void(__thiscall*)(void*);
    using GuiBorderSetPulsingAlphaFn = void(__thiscall*)(void*, int, int, int);
    using ServerGetPartyTableFn = void* (__thiscall*)(void*);
    using ObjectGetPortraitFn = void* (__thiscall*)(void*, void*);
    using GuiBorderSetFillImageFn = void(__thiscall*)(void*, const void*, int);
    using PartySelectionUpdateCountFn = void(__thiscall*)(void*);
    using ClientGetPartyFn = void* (__thiscall*)(void*);
    using PartyGetCharacterFn = void* (__thiscall*)(void*, int);
    using PartyGetIndexFn = int(__thiscall*)(void*, DWORD);
    using ServerToClientObjectIdFn = DWORD(__thiscall*)(void*, DWORD);
    using ClientCreatureSetInPartyFn = void(__thiscall*)(void*, int);
    using ClientCreatureGetServerCreatureFn = void* (__thiscall*)(void*);
    using ServerCreatureSetInPartyFn = void(__thiscall*)(void*, int, int);
    using PartyTableRemoveMemberFn = int(__thiscall*)(void*, int);
    using PartyTableSpawnNpcFn = DWORD(__thiscall*)(
        void*, int, int, const void*, const void*, int);
    using PartyTableAddMemberFn = int(__thiscall*)(void*, int, DWORD);
    using ObjectClearAllActionsFn = void(__thiscall*)(void*, int);
    using PathfindResetWaypointDataFn = void(__thiscall*)(void*);
    using CreatureGetVisibleListElementFn = int(__thiscall*)(void*, DWORD);
    using CreatureAddToVisibleListFn = void(__thiscall*)(
        void*, DWORD, int, int, int, int);
    using PartyRecalculateFollowPointFn = void(__thiscall*)(void*);

    // Uses raw GameAPI bindings when wrappers cannot preserve engine semantics.
    GetCreatureByObjectIdFn nativeServerGetCreatureByObjectId;
    OperatorNewFn nativeOperatorNew;
    CreatureCtorFn nativeCreatureCtor;
    GameEffectCtorFn nativeGameEffectCtor;
    ObjectApplyEffectFn nativeObjectApplyEffect;
    CExoStringDtorFn nativeCexostringDtor;
    AddGameInProgressFn nativeAddGameInProgress;
    CreatureLoadFromTemplateFn nativeCreatureLoadFromTemplate;
    RemoveGameInProgressFn nativeRemoveGameInProgress;
    CResGffCtorFn nativeCresgffCtor;
    CExoStringCtorCStrFn nativeCexostringCtorCstr;
    CResGffCreateFileFn nativeCresgffCreateFile;
    CreatureSaveCreatureFn nativeCreatureSaveCreature;
    CResGffWriteFileFn nativeCresgffWriteFile;
    ClientObjectSetFadeStateFn nativeClientObjectSetFadeState;
    ClientObjectSetDesiredFadeStateFn nativeClientObjectSetDesiredFadeState;
    GuiPartySelectionButtonSetSelectedFn nativePartySelectionButtonSetSelected;
    GuiSetMoveToControlFn nativeGuiSetMoveToControl;
    GuiButtonCtorFn nativeGuiButtonCtor;
    GuiButtonDtorFn nativeGuiButtonDtor;
    PartySelectionRecordCtorFn nativePartySelectionRecordCtor;
    PartySelectionRecordDtorFn nativePartySelectionRecordDtor;
    GuiBorderSetPulsingAlphaFn nativeGuiBorderSetPulsingAlpha;
    ServerGetPartyTableFn nativeServerGetPartyTable;
    ObjectGetPortraitFn nativeObjectGetPortrait;
    GuiBorderSetFillImageFn nativeGuiBorderSetFillImage;
    PartySelectionUpdateCountFn nativePartySelectionUpdateCount;
    ClientGetPartyFn nativeClientGetParty;
    PartyGetCharacterFn nativePartyGetCharacter;
    PartyGetIndexFn nativePartyGetIndex;
    ServerToClientObjectIdFn nativeServerToClientObjectId;
    ClientCreatureSetInPartyFn nativeClientCreatureSetInParty;
    ClientCreatureGetServerCreatureFn nativeClientCreatureGetServerCreature;
    ServerCreatureSetInPartyFn nativeServerCreatureSetInParty;
    PartyTableRemoveMemberFn nativePartyTableRemoveMember;
    PartyTableSpawnNpcFn nativePartyTableSpawnNpc;
    PartyTableAddMemberFn nativePartyTableAddMember;
    ObjectClearAllActionsFn nativeObjectClearAllActions;
    PathfindResetWaypointDataFn nativePathfindResetWaypointData;
    CreatureGetVisibleListElementFn nativeCreatureGetVisibleListElement;
    CreatureAddToVisibleListFn nativeCreatureAddToVisibleList;
    PartyRecalculateFollowPointFn nativePartyRecalculateFollowPoint;
    void* nativePartySelectionOnToggled;
    void* nativePartySelectionOnEnter;
    TransferInventoryFn nativeTransferInventory;
    CreatureForceEquipClothingFn nativeCreatureForceEquipClothing;
    ScalarDeletingDestructorFn nativeCreatureDeletingDestructor;
    ScalarDeletingDestructorFn nativeResGffDeletingDestructor;
    ObjectStatusVirtualFn nativeCreatureGetCurrentHitPoints;

    template <typename Fn>
    Fn RequireFunctionAddress(const char* className, const char* functionName)
    {
        void* address = GameVersion::GetFunctionAddress(className, functionName);
        if (!address) {
            throw GameVersionException(
                std::string("Invalid function: ") + className + "::" + functionName);
        }
        return reinterpret_cast<Fn>(address);
    }

    int RequireOffset(const char* className, const char* memberName)
    {
        const int offset = GameVersion::GetOffset(className, memberName);
        if (offset < 0) {
            throw GameVersionException(
                std::string("Invalid offset: ") + className + "::" + memberName);
        }
        return offset;
    }

    unsigned int RequireClassSize(const char* className)
    {
        const int size = GameVersion::GetClassSize(className);
        if (size <= 0) {
            throw GameVersionException(std::string("Invalid class size: ") + className);
        }
        return static_cast<unsigned int>(size);
    }

    bool InitializePatchApi()
    {
        if (!GameVersion::Initialize()) {
            return false;
        }
        OFFSET_PARTY_MEMBER_COUNT = RequireOffset("CSWPartyTable", "pt_num_members");
        OFFSET_PARTY_MEMBER_SLOTS = RequireOffset("CSWPartyTable", "pt_member_ids");
        OFFSET_NPC_OBJECT_IDS = OFFSET_PARTY_MEMBER_SLOTS +
            PRIMARY_MEMBER_CAPACITY * static_cast<int>(sizeof(DWORD));
        OFFSET_NPC_AVAILABLE = RequireOffset("CSWPartyTable", "pt_avail_npcs");
        OFFSET_NPC_SELECTABLE = RequireOffset("CSWPartyTable", "pt_selected_npcs");
        OFFSET_PARTY_LEADER_SLOT = RequireOffset("CSWPartyTable", "pt_leader_id");
        OFFSET_PLAYER_CHARACTER_SLOT = RequireOffset("CSWPartyTable", "pt_controlled_npc");

        CREATURE_OBJECT_SIZE = RequireClassSize("CSWSCreature");
        RESGFF_OBJECT_SIZE = RequireClassSize("CResGFF");
        GAME_EFFECT_OBJECT_SIZE = RequireClassSize("CGameEffect");
        CREATURE_POST_LOAD_STATE_OFFSET = RequireOffset("CSWSObject", "is_raiseable");
        GAME_EFFECT_TYPE_OFFSET = RequireOffset("CGameEffect", "type");
        GAME_EFFECT_SUBTYPE_FLAGS_OFFSET = RequireOffset("CGameEffect", "subtype");
        OBJECT_POSITION_OFFSET = RequireOffset("CSWSObject", "position");
        OBJECT_ORIENTATION_OFFSET = RequireOffset("CSWSObject", "orientation");
        CREATURE_PATHFIND_INFO_OFFSET =
            RequireOffset("CSWSCreature", "path_find_info");

        PARTY_SELECT_SLOT_STRIDE = static_cast<int>(RequireClassSize("CSWGuiPartySelectionData"));
        PANEL_PARTY_SELECTION_DATA_BASE_OFFSET =
            RequireOffset("CSWGuiPartySelection", "party_data");
        GUI_BUTTON_OBJECT_SIZE = static_cast<int>(RequireClassSize("CSWGuiButton"));
        DATA_BUTTON_OFFSET = RequireOffset("CSWGuiPartySelectionData", "npc_button");
        DATA_NOT_AVAILABLE_LABEL_OFFSET =
            RequireOffset("CSWGuiPartySelectionData", "portrait_label");
        DATA_CHARACTER_LABEL_OFFSET =
            RequireOffset("CSWGuiPartySelectionData", "character_label");
        PANEL_DONE_BUTTON_OFFSET =
            RequireOffset("CSWGuiPartySelection", "done_button");
        const int acceptButtonOffset =
            RequireOffset("CSWGuiPartySelection", "accept_button");
        PANEL_CURRENT_PORTRAIT_OFFSET =
            acceptButtonOffset + GUI_BUTTON_OBJECT_SIZE;
        PARTY_SELECTION_RECORD_CONTROL_OFFSETS[0] = DATA_CHARACTER_LABEL_OFFSET;
        PARTY_SELECTION_RECORD_CONTROL_OFFSETS[1] = DATA_BUTTON_OFFSET;
        PARTY_SELECTION_RECORD_CONTROL_OFFSETS[2] = DATA_NOT_AVAILABLE_LABEL_OFFSET;

        nativeServerGetCreatureByObjectId = RequireFunctionAddress<GetCreatureByObjectIdFn>("CServerExoApp", "GetCreatureByGameObjectID");
        nativeOperatorNew = RequireFunctionAddress<OperatorNewFn>("Global", "operator_new_2");
        nativeCreatureCtor = RequireFunctionAddress<CreatureCtorFn>("CSWSCreature", "Constructor");
        nativeGameEffectCtor = RequireFunctionAddress<GameEffectCtorFn>("CGameEffect", "Constructor");
        nativeObjectApplyEffect = RequireFunctionAddress<ObjectApplyEffectFn>("CSWSObject", "ApplyEffect");
        nativeCexostringDtor = RequireFunctionAddress<CExoStringDtorFn>("CExoString", "Destructor_2");
        nativeAddGameInProgress = RequireFunctionAddress<AddGameInProgressFn>("CSWPartyTable", "AddGameInProgress");
        nativeCreatureLoadFromTemplate = RequireFunctionAddress<CreatureLoadFromTemplateFn>("CSWSCreature", "LoadFromTemplate");
        nativeRemoveGameInProgress = RequireFunctionAddress<RemoveGameInProgressFn>("CSWPartyTable", "RemoveGameInProgress");
        nativeCresgffCtor = RequireFunctionAddress<CResGffCtorFn>("CResGFF", "Constructor");
        nativeCexostringCtorCstr = RequireFunctionAddress<CExoStringCtorCStrFn>("CExoString", "CStrConstructor");
        nativeCresgffCreateFile = RequireFunctionAddress<CResGffCreateFileFn>("CResGFF", "CreateGFFFile");
        nativeCreatureSaveCreature = RequireFunctionAddress<CreatureSaveCreatureFn>("CSWSCreature", "SaveCreature");
        nativeCresgffWriteFile = RequireFunctionAddress<CResGffWriteFileFn>("CResGFF", "WriteGFFFile");
        nativeClientObjectSetFadeState = RequireFunctionAddress<ClientObjectSetFadeStateFn>("CSWCObject", "SetFadeState");
        nativeClientObjectSetDesiredFadeState = RequireFunctionAddress<ClientObjectSetDesiredFadeStateFn>("CSWCObject", "SetDesiredFadeState");
        nativePartySelectionButtonSetSelected = RequireFunctionAddress<GuiPartySelectionButtonSetSelectedFn>("CSWGuiPartySelectionButton", "SetSelected");
        nativeGuiSetMoveToControl = RequireFunctionAddress<GuiSetMoveToControlFn>("CSWGuiNavigable", "SetMoveToControl");
        nativeGuiButtonCtor = RequireFunctionAddress<GuiButtonCtorFn>("CSWGuiButton", "Constructor");
        nativeGuiButtonDtor = RequireFunctionAddress<GuiButtonDtorFn>("CSWGuiButton", "Destructor");
        nativePartySelectionRecordCtor = RequireFunctionAddress<PartySelectionRecordCtorFn>("CSWGuiPartySelectionData", "Constructor");
        nativePartySelectionRecordDtor = RequireFunctionAddress<PartySelectionRecordDtorFn>("CSWGuiPartySelectionData", "Destructor");
        nativeGuiBorderSetPulsingAlpha = RequireFunctionAddress<GuiBorderSetPulsingAlphaFn>("CSWGuiBorderParams", "SetPulsingAlpha");
        nativeServerGetPartyTable = RequireFunctionAddress<ServerGetPartyTableFn>("CServerExoApp", "GetPartyTable");
        nativeObjectGetPortrait = RequireFunctionAddress<ObjectGetPortraitFn>("CSWSObject", "GetPortrait");
        nativeGuiBorderSetFillImage = RequireFunctionAddress<GuiBorderSetFillImageFn>("CSWGuiBorderParams", "SetFillImage");
        nativePartySelectionUpdateCount = RequireFunctionAddress<PartySelectionUpdateCountFn>("CSWGuiPartySelection", "UpdateCount");
        nativeClientGetParty = RequireFunctionAddress<ClientGetPartyFn>(
            "CClientExoApp", "GetSWParty");
        nativePartyGetCharacter = RequireFunctionAddress<PartyGetCharacterFn>(
            "CSWParty", "GetCharacter");
        nativePartyGetIndex = RequireFunctionAddress<PartyGetIndexFn>(
            "CSWParty", "GetIndex");
        nativeServerToClientObjectId =
            RequireFunctionAddress<ServerToClientObjectIdFn>(
                "CClientExoAppInternal", "ServerToClientObjectId");
        nativeClientCreatureSetInParty =
            RequireFunctionAddress<ClientCreatureSetInPartyFn>(
                "CSWCCreature", "SetInParty");
        nativeClientCreatureGetServerCreature =
            RequireFunctionAddress<ClientCreatureGetServerCreatureFn>(
                "CSWCCreature", "GetServerCreature");
        nativeServerCreatureSetInParty =
            RequireFunctionAddress<ServerCreatureSetInPartyFn>(
                "CSWSCreature", "SetInParty");
        nativePartyTableRemoveMember =
            RequireFunctionAddress<PartyTableRemoveMemberFn>(
                "CSWPartyTable", "RemoveMember");
        nativePartyTableSpawnNpc = RequireFunctionAddress<PartyTableSpawnNpcFn>(
            "CSWPartyTable", "SpawnNPC");
        nativePartyTableAddMember = RequireFunctionAddress<PartyTableAddMemberFn>(
            "CSWPartyTable", "AddMember");
        nativeObjectClearAllActions = RequireFunctionAddress<ObjectClearAllActionsFn>(
            "CSWSObject", "ClearAllActions");
        nativePathfindResetWaypointData =
            RequireFunctionAddress<PathfindResetWaypointDataFn>(
                "CPathfindInformation", "ResetWayPointData");
        nativeCreatureGetVisibleListElement =
            RequireFunctionAddress<CreatureGetVisibleListElementFn>(
                "CSWSCreature", "GetVisibleListElement");
        nativeCreatureAddToVisibleList =
            RequireFunctionAddress<CreatureAddToVisibleListFn>(
                "CSWSCreature", "AddToVisibleList");
        nativePartyRecalculateFollowPoint =
            RequireFunctionAddress<PartyRecalculateFollowPointFn>(
                "CSWParty", "RecaulateFollowPoint");
        nativePartySelectionOnToggled = GameVersion::GetFunctionAddress("CSWGuiPartySelection", "OnToggled");
        nativePartySelectionOnEnter = GameVersion::GetFunctionAddress("CSWGuiPartySelection", "OnEnter");
        if (!nativePartySelectionOnToggled || !nativePartySelectionOnEnter) {
            throw GameVersionException("Missing party-selection event handlers");
        }
        nativeTransferInventory = RequireFunctionAddress<TransferInventoryFn>("CSWPartyTable", "TransferInventory");
        nativeCreatureForceEquipClothing = RequireFunctionAddress<CreatureForceEquipClothingFn>("CSWSCreature", "ForceEquipClothing");
        nativeCreatureDeletingDestructor = RequireFunctionAddress<ScalarDeletingDestructorFn>("CSWSCreature", "Destructor_2");
        nativeResGffDeletingDestructor = RequireFunctionAddress<ScalarDeletingDestructorFn>("CResGFF", "Destructor_2");
        nativeCreatureGetCurrentHitPoints = RequireFunctionAddress<ObjectStatusVirtualFn>("CSWSObject", "GetCurrentHitPoints");
        return true;
    }

    bool IsStockSlot(int slot)
    {
        return slot >= 0 && slot < STOCK_NPC_SLOTS;
    }

    bool IsValidLogicalSlot(int slot)
    {
        return slot >= 0 && slot < MAX_NPC_SLOTS;
    }

    PartySlotSnapshot& GetExtendedSlotState(int slot)
    {
        return gExtendedPartySlots[slot - STOCK_NPC_SLOTS];
    }

    void ResetExtendedPartyState()
    {
        for (PartySlotSnapshot& slot : gExtendedPartySlots) {
            slot = {NPC_OBJECT_SENTINEL, 0u, 1u};
        }
    }

    void* GetServerApplication()
    {
        CAppManager manager;
        std::unique_ptr<CServerExoApp> server(manager.GetServer());
        return server->GetPtr();
    }

    void* GetClientApplication()
    {
        CAppManager manager;
        std::unique_ptr<CClientExoApp> client(manager.GetClient());
        return client->GetPtr();
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
        void* clientParty = nativeClientGetParty(client);
        if (!clientParty) {
            return -1;
        }
        const DWORD clientObjectId = nativeServerToClientObjectId(
            client,
            serverObjectId);
        return nativePartyGetIndex(clientParty, clientObjectId);
    }

    void* GetCurrentPartyTable()
    {
        return nativeServerGetPartyTable(GetServerApplication());
    }

    void* GetServerCreatureByObjectId(void* server, DWORD objectId)
    {
        if (objectId == NPC_OBJECT_SENTINEL) {
            return nullptr;
        }
        return nativeServerGetCreatureByObjectId(server, objectId);
    }

    DWORD GetLogicalNPCObjectId(void* partyTable, int logicalSlot)
    {
        if (!IsValidLogicalSlot(logicalSlot)) {
            return NPC_OBJECT_SENTINEL;
        }
        if (IsStockSlot(logicalSlot)) {
            return getObjectProperty<DWORD>(
                partyTable,
                OFFSET_NPC_OBJECT_IDS + logicalSlot * 4);
        }
        return GetExtendedSlotState(logicalSlot).objectId;
    }

    void SetLogicalNPCObjectId(
        void* partyTable,
        int logicalSlot,
        DWORD objectId)
    {
        if (!IsValidLogicalSlot(logicalSlot)) {
            return;
        }
        if (IsStockSlot(logicalSlot)) {
            setObjectProperty<DWORD>(
                partyTable,
                OFFSET_NPC_OBJECT_IDS + logicalSlot * 4,
                objectId);
        } else {
            GetExtendedSlotState(logicalSlot).objectId = objectId;
        }
    }

    DWORD GetLogicalNPCAvailability(void* partyTable, int logicalSlot)
    {
        if (!IsValidLogicalSlot(logicalSlot)) {
            return 0;
        }
        if (IsStockSlot(logicalSlot)) {
            return getObjectProperty<DWORD>(
                partyTable,
                OFFSET_NPC_AVAILABLE + logicalSlot * 4);
        }
        return GetExtendedSlotState(logicalSlot).available;
    }

    void SetLogicalNPCAvailability(
        void* partyTable,
        int logicalSlot,
        DWORD available)
    {
        if (!IsValidLogicalSlot(logicalSlot)) {
            return;
        }
        if (IsStockSlot(logicalSlot)) {
            setObjectProperty<DWORD>(
                partyTable,
                OFFSET_NPC_AVAILABLE + logicalSlot * 4,
                available);
        } else {
            GetExtendedSlotState(logicalSlot).available = available;
        }
    }

    void* GetExistingNpcCreature(
        void* partyTable,
        void* server,
        int logicalSlot)
    {
        // Reuses live creatures without materializing missing ones.
        return GetServerCreatureByObjectId(
            server,
            GetLogicalNPCObjectId(partyTable, logicalSlot));
    }

    int FindLogicalNPCSlotByObjectId(void* partyTable, DWORD objectId)
    {
        if (objectId == NPC_OBJECT_SENTINEL) {
            return -1;
        }
        for (int slot = 0; slot < MAX_NPC_SLOTS; ++slot) {
            if (GetLogicalNPCObjectId(partyTable, slot) == objectId) {
                return slot;
            }
        }
        return -1;
    }

    bool IsLogicalSlotPrimaryMember(void* partyTable, int logicalSlot)
    {
        if (!IsValidLogicalSlot(logicalSlot)) {
            return false;
        }
        const int count = getObjectProperty<int>(partyTable, OFFSET_PARTY_MEMBER_COUNT);
        for (int index = 0; index < count; ++index) {
            if (getObjectProperty<int>(
                    partyTable,
                    OFFSET_PARTY_MEMBER_SLOTS + index * 4) == logicalSlot) {
                return true;
            }
        }
        return false;
    }

    void DeleteEngineObject(
        void* object,
        ScalarDeletingDestructorFn deletingDestructor)
    {
        if (object && deletingDestructor) {
            deletingDestructor(object, 1u);
        }
    }

    void ApplyPostLoadCreatureRecovery(void* creature, int applyRecovery)
    {
        if (applyRecovery == 0) {
            return;
        }
        const short status = nativeCreatureGetCurrentHitPoints(creature, 0);
        if (status > 0) {
            return;
        }

        setObjectProperty<DWORD>(
            creature,
            CREATURE_POST_LOAD_STATE_OFFSET,
            1u);
        void* effect = nativeOperatorNew(GAME_EFFECT_OBJECT_SIZE);
        nativeGameEffectCtor(effect, 1);
        setObjectProperty<WORD>(
            effect,
            GAME_EFFECT_TYPE_OFFSET,
            *reinterpret_cast<const WORD*>(ADDRESS_EFFECT_TYPE_WORD));
        WORD subtype = getObjectProperty<WORD>(
            effect,
            GAME_EFFECT_SUBTYPE_FLAGS_OFFSET);
        subtype = static_cast<WORD>(
            (subtype & static_cast<WORD>(~GAME_EFFECT_SUBTYPE_MASK)) |
            (*reinterpret_cast<const WORD*>(ADDRESS_EFFECT_SUBTYPE_WORD) &
             GAME_EFFECT_SUBTYPE_MASK));
        setObjectProperty<WORD>(
            effect,
            GAME_EFFECT_SUBTYPE_FLAGS_OFFSET,
            subtype);
        nativeObjectApplyEffect(
            creature,
            effect,
            0,
            0);
    }

    void* LoadSavedCreatureResource(void* partyTable, int logicalSlot)
    {
        void* creature = nativeOperatorNew(CREATURE_OBJECT_SIZE);
        nativeCreatureCtor(creature, NPC_OBJECT_SENTINEL, 0);
        char identity[16];
        // Builds lowercase, zero-padded CResRef identifiers.
        std::snprintf(identity, sizeof(identity), "availnpc%d", logicalSlot);
        nativeAddGameInProgress(partyTable);
        int loadResult = 0;
        {
            CResRef identityRef(identity);
            loadResult = nativeCreatureLoadFromTemplate(
                creature,
                identityRef.GetPtr(),
                0);
        }
        if (loadResult == 0) {
            DeleteEngineObject(creature, nativeCreatureDeletingDestructor);
            return nullptr;
        }
        nativeRemoveGameInProgress(partyTable);
        return creature;
    }

    void SaveCreatureResource(int logicalSlot, void* creature)
    {
        void* gff = nativeOperatorNew(RESGFF_OBJECT_SIZE);
        nativeCresgffCtor(gff);
        char outputPath[48];
        std::snprintf(outputPath, sizeof(outputPath), "GAMEINPROGRESS:AVAILNPC%d", logicalSlot);
        RawCExoString fileType;
        RawCExoString path;
        CResStruct root;
        nativeCexostringCtorCstr(&fileType, "UTC ");
        nativeCexostringCtorCstr(&path, outputPath);
        nativeCresgffCreateFile(gff, &root, &fileType, nullptr);
        nativeCreatureSaveCreature(creature, gff, &root);
        nativeCresgffWriteFile(gff, &path, SAVED_CREATURE_RESOURCE_TYPE);
        DeleteEngineObject(gff, nativeResGffDeletingDestructor);
        nativeCexostringDtor(&path);
        nativeCexostringDtor(&fileType);
    }

    DWORD ResolveLogicalNPCObject(
        void* partyTable,
        int logicalSlot,
        int createObject,
        int applyRecovery)
    {
        if (GetLogicalNPCAvailability(partyTable, logicalSlot) == 0) {
            return NPC_OBJECT_SENTINEL;
        }
        const DWORD currentObjectId =
            GetLogicalNPCObjectId(partyTable, logicalSlot);
        if (currentObjectId != NPC_OBJECT_SENTINEL || createObject == 0) {
            return currentObjectId;
        }

        void* creature = LoadSavedCreatureResource(partyTable, logicalSlot);
        if (!creature) {
            return NPC_OBJECT_SENTINEL;
        }
        ApplyPostLoadCreatureRecovery(creature, applyRecovery);
        const DWORD objectId = CGameObject(creature).GetId();
        SetLogicalNPCObjectId(partyTable, logicalSlot, objectId);
        return objectId;
    }

    int DestroyLogicalNPCObject(
        void* partyTable,
        int logicalSlot,
        int fadeClientObject)
    {
        if (!IsValidLogicalSlot(logicalSlot) ||
            IsLogicalSlotPrimaryMember(partyTable, logicalSlot)) {
            return 0;
        }
        const DWORD objectId =
            GetLogicalNPCObjectId(partyTable, logicalSlot);
        void* creature = GetServerCreatureByObjectId(
            GetServerApplication(),
            objectId);
        SetLogicalNPCObjectId(
            partyTable,
            logicalSlot,
            NPC_OBJECT_SENTINEL);
        if (!creature) {
            return 0;
        }

        if (fadeClientObject != 0) {
            CSWSCreature creatureView(creature);
            std::unique_ptr<CSWCCreature> clientView(creatureView.GetClientCreature());
            if (clientView) {
                void* clientCreature = clientView->GetPtr();
                nativeClientObjectSetFadeState(
                    clientCreature,
                    0);
                nativeClientObjectSetDesiredFadeState(
                    clientCreature,
                    0,
                    1,
                    0,
                    0);
                setObjectProperty<DWORD>(
                    clientCreature,
                    CLIENT_CREATURE_DEFERRED_DELETE_OFFSET,
                    1u);
            }
        }
        DeleteEngineObject(creature, nativeCreatureDeletingDestructor);
        return 1;
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
        nativeCexostringCtorCstr(&tagString, tag);
        CExoString tagView(static_cast<void*>(&tagString));
        CSWGuiControl controlView(destination);
        CSWGuiPanel(panel).InitControl(&controlView, &tagView, 1);
        nativeCexostringDtor(&tagString);
    }

    void SetUnavailableViewRecord(BYTE* record, int logicalSlot)
    {
        nativePartySelectionButtonSetSelected(record + DATA_BUTTON_OFFSET, 0);
        setObjectProperty<DWORD>(
            record,
            DATA_FLAGS_OFFSET,
            PANEL_SLOT_HAS_OBJECT_FLAG);
        setObjectProperty<int>(record, DATA_OBJECT_INDEX_OFFSET, -1);
        setObjectProperty<int>(record, DATA_LOGICAL_SLOT_OFFSET, logicalSlot);
        setObjectProperty<float>(record, DATA_ALPHA_OFFSET, 1.0f);
        setObjectProperty<float>(record, DATA_SELECTION_ALPHA_OFFSET, 0.25f);

        CSWGuiButton button(record + DATA_BUTTON_OFFSET);
        CSWGuiControl unavailable(record + DATA_NOT_AVAILABLE_LABEL_OFFSET);
        CSWGuiControl character(record + DATA_CHARACTER_LABEL_OFFSET);
        button.SetControlBitFlag(1, true);
        button.SetControlBitFlag(5, true);
        button.SetEnabled(0);
        unavailable.SetControlBitFlag(1, logicalSlot < STOCK_NPC_SLOTS);
        unavailable.SetControlBitFlag(5, true);
        character.SetControlBitFlag(1, false);
        character.SetControlBitFlag(5, true);
    }

    bool GetLogicalNPCSelectability(void* partyTable, int logicalSlot)
    {
        if (!IsValidLogicalSlot(logicalSlot) ||
            GetLogicalNPCAvailability(partyTable, logicalSlot) == 0) {
            return false;
        }
        return IsStockSlot(logicalSlot)
            ? getObjectProperty<BYTE>(partyTable, OFFSET_NPC_SELECTABLE + logicalSlot) != 0
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

    bool SetAvailableViewRecord(
        BYTE* record,
        PartySelectionViewState* state,
        void* partyTable,
        int logicalSlot)
    {
        const DWORD objectId = ResolveLogicalNPCObject(
            partyTable, logicalSlot, 1, 1);
        void* creature = GetServerCreatureByObjectId(
            GetServerApplication(), objectId);
        if (!creature) {
            SetUnavailableViewRecord(record, logicalSlot);
            return false;
        }

        CResRef_struct portrait = {};
        nativeObjectGetPortrait(creature, &portrait);
        nativeGuiBorderSetFillImage(
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
        setObjectProperty<int>(record, DATA_OBJECT_INDEX_OFFSET,
            clientPartyIndex);
        setObjectProperty<int>(record, DATA_LOGICAL_SLOT_OFFSET, logicalSlot);
        setObjectProperty<float>(record, DATA_ALPHA_OFFSET, 1.0f);
        setObjectProperty<float>(record, DATA_SELECTION_ALPHA_OFFSET,
            selected ? 1.0f : 0.25f);
        nativePartySelectionButtonSetSelected(
            record + DATA_BUTTON_OFFSET, selected ? 1 : 0);

        CSWGuiButton button(record + DATA_BUTTON_OFFSET);
        CSWGuiControl unavailable(record + DATA_NOT_AVAILABLE_LABEL_OFFSET);
        CSWGuiControl character(record + DATA_CHARACTER_LABEL_OFFSET);
        button.SetControlBitFlag(1, true);
        button.SetControlBitFlag(5, !interactive);
        button.SetEnabled(interactive ? 1 : 0);
        unavailable.SetControlBitFlag(1, false);
        unavailable.SetControlBitFlag(5, true);
        character.SetControlBitFlag(1, true);
        character.SetControlBitFlag(5, true);
        return true;
    }

    void InitializeViewRecord(
        void* panel,
        PartySelectionViewState* state,
        int physicalIndex)
    {
        BYTE* record = GetViewRecord(state, physicalIndex);
        std::memset(record, 0, PARTY_SELECT_SLOT_STRIDE);
        nativePartySelectionRecordCtor(record);
        ++state->constructedRecords;

        char tag[32];
        std::snprintf(tag, sizeof(tag), "LBL_NA%d", physicalIndex);
        BindNamedControl(panel, record + DATA_NOT_AVAILABLE_LABEL_OFFSET, tag);
        std::snprintf(tag, sizeof(tag), "LBL_CHAR%d", physicalIndex);
        BindNamedControl(panel, record + DATA_CHARACTER_LABEL_OFFSET, tag);
        std::snprintf(tag, sizeof(tag), "BTN_NPC%d", physicalIndex);
        BindNamedControl(panel, record + DATA_BUTTON_OFFSET, tag);

        CSWGuiButton button(record + DATA_BUTTON_OFFSET);
        CSWGuiPanel panelView(panel);
        button.AddEvent(
            GUI_CONTROL_PRIMARY_EVENT,
            &panelView,
            nativePartySelectionOnToggled);
        button.AddEvent(
            GUI_CONTROL_CLICK_EVENT,
            &panelView,
            nativePartySelectionOnToggled);
        button.AddEvent(0, &panelView, nativePartySelectionOnEnter);
        button.AddEvent(
            1,
            &panelView,
            reinterpret_cast<void*>(ADDRESS_PARTY_SELECTION_HOVER_HANDLER));

        setObjectProperty<void*>(
            record,
            DATA_BUTTON_BACK_POINTER_OFFSET,
            record);
        std::memcpy(
            record + DATA_DEFAULT_COLOR_OFFSET,
            GetStockPartySelectionDataAt(panel, 0) + DATA_DEFAULT_COLOR_OFFSET,
            sizeof(float) * 3);
        nativeGuiBorderSetPulsingAlpha(
            record + DATA_BUTTON_INIT_PARAMS_OFFSET,
            1,
            1,
            0);
        setObjectProperty<DWORD>(record, DATA_FLAGS_OFFSET, 0u);
        setObjectProperty<int>(record, DATA_LOGICAL_SLOT_OFFSET, physicalIndex);
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
        nativeGuiButtonCtor(button);
        ++state->constructedButtons;
        BindNamedControl(panel, button, tag);
        CSWGuiButton buttonView(button);
        CSWGuiPanel panelView(panel);
        buttonView.AddEvent(
            GUI_CONTROL_PRIMARY_EVENT,
            &panelView,
            callback);
        buttonView.AddEvent(
            GUI_CONTROL_CLICK_EVENT,
            &panelView,
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
            0,
            "BTN_PAGE_PREV",
            funcAddr(PartySelectionPrevButtonCallback));
        InitializePageButton(
            panel,
            state,
            1,
            "BTN_PAGE_NEXT",
            funcAddr(PartySelectionNextButtonCallback));
        state->ready = true;
        return true;
    }

    void DetachControl(
        CExoArrayList<CSWGuiControl*>* controls,
        void* control)
    {
        CSWGuiControl controlView(control);
        const int index = controls->IndexOf(&controlView);
        if (index >= 0) {
            controls->DeleteAt(index);
        }
    }

    void DetachPartySelectionControls(
        void* panel,
        PartySelectionViewState* state,
        bool includePageButtons)
    {
        CSWGuiPanel panelView(panel);
        std::unique_ptr<CExoArrayList<CSWGuiControl*>> controls(
            panelView.GetControls());
        for (int index = 0; index < STOCK_NPC_SLOTS; ++index) {
            BYTE* record = GetStockPartySelectionDataAt(panel, index);
            for (int offset : PARTY_SELECTION_RECORD_CONTROL_OFFSETS) {
                DetachControl(controls.get(), record + offset);
            }
        }
        if (state && state->records) {
            for (int index = 0;
                 index < PARTY_SELECTION_VIEW_RECORD_COUNT;
                 ++index) {
                BYTE* record = GetViewRecord(state, index);
                for (int offset : PARTY_SELECTION_RECORD_CONTROL_OFFSETS) {
                    DetachControl(controls.get(), record + offset);
                }
            }
        }
        if (includePageButtons && state && state->pageButtons) {
            for (int index = 0;
                 index < PARTY_SELECTION_PAGE_CONTROL_COUNT;
                 ++index) {
                DetachControl(controls.get(), GetPageButton(state, index));
            }
        }
    }

    void AttachStockPartySelectionControls(void* panel)
    {
        PartySelectionViewState* state = FindPartySelectionView(panel);
        DetachPartySelectionControls(panel, state, false);
        CSWGuiPanel panelView(panel);
        for (int offset : PARTY_SELECTION_RECORD_CONTROL_OFFSETS) {
            for (int index = 0; index < STOCK_NPC_SLOTS; ++index) {
                CSWGuiControl control(
                    GetStockPartySelectionDataAt(panel, index) + offset);
                panelView.AddControl(&control);
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
        void* previous = getObjectProperty<void*>(
            panel,
            PANEL_CURRENT_PORTRAIT_OFFSET);
        if (previous) {
            CSWGuiButton(previous).SetActive(0);
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

        CSWGuiPanel panelView(panel);
        panelView.SetActiveControl(nullptr, 0);
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
            CSWGuiControl character(
                GetViewRecord(state, physicalIndex) +
                DATA_CHARACTER_LABEL_OFFSET);
            panelView.AddControl(&character);
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
            CSWGuiControl buttonControl(button);
            panelView.AddControl(&buttonControl);
            if (!firstInteractive && interactive) {
                firstInteractive = button;
            }
        }

        for (int physicalIndex = 0;
             physicalIndex < PARTY_SELECTION_VIEW_RECORD_COUNT;
             ++physicalIndex) {
            BYTE* record = GetViewRecord(state, physicalIndex);
            CSWGuiControl unavailable(
                record + DATA_NOT_AVAILABLE_LABEL_OFFSET);
            if (unavailable.GetControlBitFlag(1)) {
                panelView.AddControl(&unavailable);
            }
        }

        CSWGuiButton(GetPageButton(state, 0)).SetEnabled(pageBase != 0);
        CSWGuiButton(GetPageButton(state, 1)).SetEnabled(
            pageBase != LAST_PARTY_SELECT_PAGE_BASE);
        BYTE* done = reinterpret_cast<BYTE*>(panel) +
            PANEL_DONE_BUTTON_OFFSET;
        nativeGuiSetMoveToControl(
            GetPageButton(state, 0),
            GUI_NAVIGATION_UP,
            firstInteractive);
        nativeGuiSetMoveToControl(
            GetPageButton(state, 1),
            GUI_NAVIGATION_UP,
            firstInteractive);
        nativeGuiSetMoveToControl(
            done,
            GUI_NAVIGATION_UP,
            firstInteractive);
        nativeGuiSetMoveToControl(
            done,
            GUI_NAVIGATION_DOWN,
            firstInteractive);

        setObjectProperty<int>(
            panel,
            PANEL_SELECTED_COUNT_OFFSET,
            CountSelectedSlots(state));
        nativePartySelectionUpdateCount(panel);
        if (!firstInteractive) {
            firstInteractive = GetPageButton(
                state,
                pageBase == 0 ? 1 : 0);
        }
        CSWGuiControl focus(firstInteractive);
        panelView.SetActiveControl(&focus, 1);
    }

    void __fastcall PartySelectionPrevButtonCallback(void* self, void*, void*)
    {
        ShowPartySelectionPage(self, 0);
    }

    void __fastcall PartySelectionNextButtonCallback(void* self, void*, void*)
    {
        ShowPartySelectionPage(self, LAST_PARTY_SELECT_PAGE_BASE);
    }

    bool RemoveExtendedPartyMember(
        void* partyTable,
        int logicalSlot)
    {
        const int activeIndex = FindActivePartySlot(partyTable, logicalSlot);
        if (activeIndex < 0) {
            return true;
        }
        const DWORD objectId = GetLogicalNPCObjectId(
            partyTable,
            logicalSlot);
        const int clientPartyIndex = GetClientPartyIndexForServerObject(
            objectId);
        if (clientPartyIndex < 0) {
            return false;
        }
        void* clientParty = nativeClientGetParty(GetClientApplication());
        void* clientMember = clientParty
            ? nativePartyGetCharacter(clientParty, clientPartyIndex)
            : nullptr;
        if (!clientMember) {
            return false;
        }
        nativeClientCreatureSetInParty(clientMember, 0);
        if (void* creature = nativeClientCreatureGetServerCreature(clientMember)) {
            nativeServerCreatureSetInParty(creature, 0, 1);
        }
        return nativePartyTableRemoveMember(partyTable, logicalSlot) != 0;
    }

    bool AddExtendedPartyMember(
        void* partyTable,
        int logicalSlot)
    {
        int activeCount = getObjectProperty<int>(
            partyTable,
            OFFSET_PARTY_MEMBER_COUNT);
        if (activeCount < 0 || activeCount >= PRIMARY_MEMBER_CAPACITY) {
            return false;
        }

        CServerExoApp server;
        std::unique_ptr<CSWSCreature> player(server.GetPlayerCreature());
        void* playerCreature = player ? player->GetPtr() : nullptr;
        if (!playerCreature) {
            return false;
        }
        BYTE* playerBytes = static_cast<BYTE*>(playerCreature);
        const DWORD objectId = nativePartyTableSpawnNpc(
            partyTable,
            logicalSlot,
            1,
            playerBytes + OBJECT_POSITION_OFFSET,
            playerBytes + OBJECT_ORIENTATION_OFFSET,
            1);
        if (objectId == NPC_OBJECT_SENTINEL ||
            nativePartyTableAddMember(partyTable, logicalSlot, objectId) == 0) {
            return false;
        }

        void* creature = GetServerCreatureByObjectId(
            GetServerApplication(),
            objectId);
        if (!creature) {
            return false;
        }
        nativeObjectClearAllActions(creature, 1);
        if (void* path = getObjectProperty<void*>(
                creature,
                CREATURE_PATHFIND_INFO_OFFSET)) {
            nativePathfindResetWaypointData(path);
        }
        nativeServerCreatureSetInParty(creature, 1, 1);
        void** vtable = *reinterpret_cast<void***>(creature);
        if (vtable && vtable[0x70 / sizeof(void*)]) {
            reinterpret_cast<void(__thiscall*)(void*)>(
                vtable[0x70 / sizeof(void*)])(creature);
        }
        if (nativeCreatureGetVisibleListElement(playerCreature, objectId) == 0) {
            nativeCreatureAddToVisibleList(
                playerCreature,
                objectId,
                1,
                1,
                0,
                0);
        }
        void* clientParty = nativeClientGetParty(GetClientApplication());
        if (clientParty) {
            activeCount = getObjectProperty<int>(
                partyTable,
                OFFSET_PARTY_MEMBER_COUNT);
            void* clientMember = nativePartyGetCharacter(clientParty, activeCount);
            if (clientMember) {
                nativeClientCreatureSetInParty(clientMember, 1);
            }
        }
        return true;
    }

    void StageStockPartySelectionRecords(
        void* panel,
        PartySelectionViewState* state,
        void* partyTable)
    {
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
            nativePartySelectionButtonSetSelected(
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
            nativePartySelectionButtonSetSelected(
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
            if (void* clientParty = nativeClientGetParty(GetClientApplication())) {
                nativePartyRecalculateFollowPoint(clientParty);
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

    void DestroyPartySelectionView(void* panel)
    {
        PartySelectionViewState* state = FindPartySelectionView(panel);
        if (!state) {
            return;
        }
        DetachPartySelectionControls(panel, state, true);
        while (state->constructedButtons > 0) {
            --state->constructedButtons;
            nativeGuiButtonDtor(GetPageButton(
                state,
                static_cast<int>(state->constructedButtons)));
        }
        while (state->constructedRecords > 0) {
            --state->constructedRecords;
            nativePartySelectionRecordDtor(GetViewRecord(
                state,
                static_cast<int>(state->constructedRecords)));
        }
        FreeAligned(state->pageButtons);
        FreeAligned(state->records);
        std::memset(state, 0, sizeof(*state));
        AttachStockPartySelectionControls(panel);
    }

}  // namespace

extern "C" int __cdecl GetIsNPCAvailable32(
    void* partyTable,
    const int* logicalSlot)
{
    return GetLogicalNPCAvailability(partyTable, *logicalSlot) != 0;
}

extern "C" int __cdecl GetNumNPCAvailable32(void* partyTable)
{
    int count = 0;
    for (int slot = 0; slot < MAX_NPC_SLOTS; ++slot) {
        count += GetLogicalNPCAvailability(partyTable, slot) != 0;
    }
    return count;
}

extern "C" void __cdecl SetNPCObject32(
    void* partyTable,
    const int* logicalSlot,
    const DWORD* objectId)
{
    if (GetLogicalNPCAvailability(partyTable, *logicalSlot) != 0) {
        SetLogicalNPCObjectId(partyTable, *logicalSlot, *objectId);
    }
}

extern "C" int __cdecl GetIsMemberByObject32(
    void* partyTable,
    const DWORD* objectId)
{
    return IsLogicalSlotPrimaryMember(
        partyTable, FindLogicalNPCSlotByObjectId(partyTable, *objectId));
}

extern "C" int __cdecl GetIsAvailableByObject32(
    void* partyTable,
    const DWORD* objectId)
{
    for (int slot = 0; slot < MAX_NPC_SLOTS; ++slot) {
        if (GetLogicalNPCObjectId(partyTable, slot) == *objectId) {
            return 1;
        }
    }
    return 0;
}

extern "C" int __cdecl GetNPCSelectability32(
    void* partyTable,
    const int* logicalSlot)
{
    const int slot = *logicalSlot;
    if (GetLogicalNPCAvailability(partyTable, slot) == 0) {
        return 0xFF;
    }
    return IsStockSlot(slot)
        ? getObjectProperty<BYTE>(partyTable, OFFSET_NPC_SELECTABLE + slot)
        : GetExtendedSlotState(slot).selectable;
}

extern "C" void __cdecl SetNPCSelectability32(
    void* partyTable,
    const int* logicalSlot,
    const BYTE* value)
{
    const int slot = *logicalSlot;
    if (GetLogicalNPCAvailability(partyTable, slot) == 0) {
        return;
    }
    if (IsStockSlot(slot)) {
        setObjectProperty<BYTE>(partyTable, OFFSET_NPC_SELECTABLE + slot, *value);
    } else {
        GetExtendedSlotState(slot).selectable = *value;
    }
}

extern "C" int __cdecl KillNPCObject32(
    void* partyTable,
    const int* logicalSlot,
    const int* fadeClientObject)
{
    return DestroyLogicalNPCObject(partyTable, *logicalSlot, *fadeClientObject);
}

extern "C" int __cdecl GetNPCID32(
    void* partyTable,
    const DWORD* objectId)
{
    return FindLogicalNPCSlotByObjectId(partyTable, *objectId);
}

extern "C" void __cdecl ClearMember32(
    void* partyTable,
    const int* logicalSlot)
{
    SetLogicalNPCObjectId(partyTable, *logicalSlot, NPC_OBJECT_SENTINEL);
}

extern "C" int __cdecl GetIsLeader32(
    void* partyTable,
    const DWORD* objectId)
{
    const int logicalSlot = FindLogicalNPCSlotByObjectId(
        partyTable,
        *objectId);
    return logicalSlot >= 0 &&
        logicalSlot == getObjectProperty<int>(partyTable, OFFSET_PARTY_LEADER_SLOT);
}

extern "C" void __cdecl SetLeaderByObject32(
    void* partyTable,
    const DWORD* objectId)
{
    setObjectProperty<int>(partyTable, OFFSET_PARTY_LEADER_SLOT,
        FindLogicalNPCSlotByObjectId(partyTable, *objectId));
}

extern "C" void __cdecl SaveMember32(
    void* partyTable,
    const int* logicalSlot,
    const int* saveMode)
{
    void* creature = GetServerCreatureByObjectId(
        GetServerApplication(), GetLogicalNPCObjectId(partyTable, *logicalSlot));
    if (!creature) {
        return;
    }
    if (*saveMode != 0) {
        CSWSObject(creature).ClearAllActions(1);
    }
    SaveCreatureResource(*logicalSlot, creature);
}

extern "C" int __cdecl RemoveNPC32(
    void* partyTable,
    const int* logicalSlot)
{
    if (GetLogicalNPCAvailability(partyTable, *logicalSlot) == 0) {
        return 0;
    }
    SetLogicalNPCAvailability(partyTable, *logicalSlot, 0);
    return 1;
}

extern "C" DWORD __cdecl GetNPCObject32(
    void* partyTable,
    const int* logicalSlot,
    const int* createObject,
    const int* applyRecovery)
{
    return ResolveLogicalNPCObject(partyTable, *logicalSlot, *createObject, *applyRecovery);
}

extern "C" int __cdecl RejectUnavailableMemberSlot(
    void* partyTable,
    const int* logicalSlot)
{
    return GetLogicalNPCAvailability(partyTable, *logicalSlot) == 0;
}

extern "C" int __cdecl PrepareAddMemberSlot(
    void* partyTable,
    const int* logicalSlot,
    const DWORD* objectId)
{
    // Replaces only AddMember's nine-slot checks and storage writes.

    for (int slot = 0; slot < MAX_NPC_SLOTS; ++slot) {
        if (slot != *logicalSlot &&
            GetLogicalNPCObjectId(partyTable, slot) == *objectId) {
            return 1;
        }
    }

    const int memberCount = getObjectProperty<int>(
        partyTable,
        OFFSET_PARTY_MEMBER_COUNT);
    setObjectProperty<int>(
        partyTable,
        OFFSET_PARTY_MEMBER_SLOTS + memberCount * 4,
        *logicalSlot);
    SetLogicalNPCObjectId(partyTable, *logicalSlot, *objectId);
    return 0;
}

extern "C" void __cdecl ResetExtendedPartyTable()
{
    ResetExtendedPartyState();
}

extern "C" void __cdecl ClearExtendedPartyObjectIds()
{
    for (PartySlotSnapshot& slot : gExtendedPartySlots) {
        slot.objectId = NPC_OBJECT_SENTINEL;
    }
}

extern "C" void __cdecl WriteExtendedPartyPersistence(
    void* gff,
    void* parentStruct)
{
    CResGFF resource(gff);
    auto* parent = static_cast<CResStruct*>(parentStruct);
    CResList availabilityList{};
    char listLabel[] = "PT_AVAIL_NPCS";
    if (!resource.GetList(&availabilityList, parent, listLabel)) {
        return;
    }

    int listCount = std::max(0, resource.GetListCount(&availabilityList));
    char availabilityLabel[] = "PT_NPC_AVAIL";
    char selectabilityLabel[] = "PT_NPC_SELECT";
    for (int logicalSlot = STOCK_NPC_SLOTS;
         logicalSlot < MAX_NPC_SLOTS;
         ++logicalSlot) {
        CResStruct element{};
        if (logicalSlot < listCount) {
            if (!resource.GetListElement(
                    &element, &availabilityList, logicalSlot)) {
                continue;
            }
        } else {
            while (listCount <= logicalSlot) {
                CResStruct appended{};
                if (!resource.AddListElement(
                        &appended, &availabilityList, 0)) {
                    return;
                }
                if (listCount == logicalSlot) {
                    element = appended;
                }
                ++listCount;
            }
        }

        const PartySlotSnapshot& slot = GetExtendedSlotState(logicalSlot);
        resource.WriteFieldBYTE(
            &element,
            slot.available != 0 ? 1u : 0u,
            availabilityLabel);
        resource.WriteFieldBYTE(
            &element,
            slot.selectable != 0 ? 1u : 0u,
            selectabilityLabel);
    }
}

extern "C" void __cdecl ReadExtendedPartyPersistence(
    void* gff,
    void* parentStruct)
{
    ResetExtendedPartyState();
    CResGFF resource(gff);
    auto* parent = static_cast<CResStruct*>(parentStruct);
    CResList availabilityList{};
    char listLabel[] = "PT_AVAIL_NPCS";
    if (!resource.GetList(&availabilityList, parent, listLabel)) {
        return;
    }

    const int listCount = std::max(
        0,
        std::min(resource.GetListCount(&availabilityList), MAX_NPC_SLOTS));
    char availabilityLabel[] = "PT_NPC_AVAIL";
    char selectabilityLabel[] = "PT_NPC_SELECT";
    for (int logicalSlot = STOCK_NPC_SLOTS;
         logicalSlot < listCount;
         ++logicalSlot) {
        CResStruct element{};
        if (!resource.GetListElement(
                &element, &availabilityList, logicalSlot)) {
            continue;
        }

        int success = 0;
        PartySlotSnapshot& slot = GetExtendedSlotState(logicalSlot);
        slot.available = resource.ReadFieldBYTE(
            &element, availabilityLabel, &success, 0) != 0;
        success = 0;
        slot.selectable = resource.ReadFieldBYTE(
            &element, selectabilityLabel, &success, 1) != 0;
    }
}

extern "C" int __cdecl RejectAlreadyAvailableNpc(
    void* partyTable,
    int logicalSlot)
{
    return GetLogicalNPCAvailability(partyTable, logicalSlot) != 0;
}

extern "C" void __cdecl SetAddNpcAvailable(
    void* partyTable,
    int logicalSlot)
{
    // Marks availability after native AddNPC validation succeeds.
    SetLogicalNPCAvailability(partyTable, logicalSlot, 1);
}

extern "C" void __cdecl FinishPartySelectionLayout(void* panel)
{
    CreatePartySelectionView(panel);
    CSWGuiPanel(panel).StopLoadFromLayout();
}

extern "C" void __cdecl PreparePartySelectionDestructor(void* panel)
{
    DestroyPartySelectionView(panel);
}

extern "C" void __cdecl PreparePartySelectionOnPanelAdded(void* panel)
{
    PartySelectionViewState* state = FindPartySelectionView(panel);
    if (state) {
        state->pageBase = -1;
    }
    AttachStockPartySelectionControls(panel);
}

extern "C" void __cdecl FinishPartySelectionOnPanelAdded(void* panel)
{
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

extern "C" void __cdecl FinishPartySelectionOnPanelRemoved(void* panel)
{
    PartySelectionViewState* state = FindPartySelectionView(panel);
    if (!state) {
        return;
    }
    state->pageBase = -1;
    std::memset(state->selected, 0, sizeof(state->selected));
    std::memset(state->forced, 0, sizeof(state->forced));
    setObjectProperty<DWORD>(panel, PANEL_FORCED_MODE_OFFSET, 0u);
}

extern "C" void __cdecl PreparePartySelectionOnDone(
    void* panel,
    void* const* control)
{
    if (!control || !*control ||
        getObjectProperty<void*>(
            *control,
            GUI_CONTROL_EVENT_OWNER_OFFSET) == nullptr) {
        return;
    }

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

extern "C" void __cdecl FinishPartySelectionAccept(void* panel)
{
    FinishPartySelectionModelAccept(panel);
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

extern "C" void __cdecl AdjustPartySelectionAfterObjectRemoval(
    void* panel,
    const int* removedIndex)
{
    for (int recordIndex = 0;
         recordIndex < STOCK_NPC_SLOTS;
         ++recordIndex) {
        BYTE* record = GetStockPartySelectionDataAt(panel, recordIndex);
        const int objectIndex = getObjectProperty<int>(
            record,
            DATA_OBJECT_INDEX_OFFSET);
        if (objectIndex > *removedIndex) {
            setObjectProperty<int>(
                record,
                DATA_OBJECT_INDEX_OFFSET,
                objectIndex - 1);
        }
    }
    PartySelectionViewState* state = FindPartySelectionView(panel);
    if (!state || !state->ready) {
        return;
    }
    for (int physicalIndex = 0;
         physicalIndex < PARTY_SELECTION_VIEW_RECORD_COUNT;
         ++physicalIndex) {
        BYTE* record = GetViewRecord(state, physicalIndex);
        const int objectIndex = getObjectProperty<int>(
            record,
            DATA_OBJECT_INDEX_OFFSET);
        if (objectIndex > *removedIndex) {
            setObjectProperty<int>(
                record,
                DATA_OBJECT_INDEX_OFFSET,
                objectIndex - 1);
        }
    }
}

extern "C" void __cdecl ClearPartySelectionForcedFlags(void* panel)
{
    PartySelectionViewState* state = FindPartySelectionView(panel);
    if (state) {
        std::memset(state->forced, 0, sizeof(state->forced));
        for (int index = 0;
             index < PARTY_SELECTION_VIEW_RECORD_COUNT;
             ++index) {
            BYTE* record = GetViewRecord(state, index);
            setObjectProperty<DWORD>(
                record,
                DATA_FLAGS_OFFSET,
                getObjectProperty<DWORD>(record, DATA_FLAGS_OFFSET) &
                    ~PANEL_SLOT_FORCED_FLAG);
        }
    }
    setObjectProperty<DWORD>(panel, PANEL_FORCED_MODE_OFFSET, 0u);
}

extern "C" void* __cdecl ResolveActiveMemberCreature(
    void* partyTable,
    const int* logicalSlot)
{
    return GetServerCreatureByObjectId(
        GetServerApplication(),
        ResolveLogicalNPCObject(partyTable, *logicalSlot, 1, 1));
}

extern "C" void* __cdecl ResolveUnstealthMemberCreature(
    void* partyTable,
    int counter)
{
    const int activeIndex = counter & 0xFF;
    const int logicalSlot = getObjectProperty<int>(
        partyTable, OFFSET_PARTY_MEMBER_SLOTS + activeIndex * 4);
    return GetServerCreatureByObjectId(GetServerApplication(),
        ResolveLogicalNPCObject(partyTable, logicalSlot, 1, 1));
}

extern "C" void __cdecl StoreCreatePartyPlayerObjectId(
    void* partyTable,
    DWORD objectId)
{
    SetLogicalNPCObjectId(
        partyTable,
        getObjectProperty<int>(
            partyTable,
            OFFSET_PLAYER_CHARACTER_SLOT),
        objectId);
}

extern "C" void __cdecl TransferExistingNpcInventories(
    void* partyTable,
    void* server)
{
    for (int slot = 0; slot < MAX_NPC_SLOTS; ++slot) {
        void* creature = GetExistingNpcCreature(
            partyTable,
            server,
            slot);
        if (creature) {
            nativeTransferInventory(partyTable, creature);
        }
    }
}

extern "C" void __cdecl PostProcessExistingNpcs(
    void* partyTable,
    void* server)
{
    for (int slot = 0; slot < MAX_NPC_SLOTS; ++slot) {
        void* creature = GetExistingNpcCreature(
            partyTable,
            server,
            slot);
        if (creature) {
            nativeCreatureForceEquipClothing(creature);
        }
    }
}

extern "C" int __cdecl RejectSwitchPlayerTarget(
    void* partyTable,
    int logicalSlot)
{
    return GetLogicalNPCAvailability(partyTable, logicalSlot) == 0;
}

extern "C" DWORD __cdecl ResolveSwitchPlayerTargetObjectId(
    void* partyTable,
    int logicalSlot)
{
    // Resolves existing objects without forcing materialization.
    return ResolveLogicalNPCObject(partyTable, logicalSlot, 0, 0);
}

extern "C" void __cdecl DestroyDetachedSwitchNpc(
    void* partyTable,
    int logicalSlot)
{
    DestroyLogicalNPCObject(partyTable, logicalSlot, 1);
}

extern "C" int __stdcall DllMain(void*, DWORD reason, void*)
{
    if (reason == DLL_PROCESS_ATTACH) {
        try {
            if (InitializePatchApi()) {
                return 1;
            }
        } catch (...) {
        }
        GameVersion::Reset(true);
        return 0;
    }
    if (reason == DLL_PROCESS_DETACH) {
        GameVersion::Shutdown();
    }
    return 1;
}
