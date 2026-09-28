// Implements Vriff's 32-slot roster patch for KOTOR I GOG 1.03.
// Pairs with kotor1-gog-103.hooks.toml.

#include "Common.h"
#include "GameAPI/GameVersion.h"
#include "GameAPI/CAppManager.h"
#include "GameAPI/CClientExoApp.h"
#include "GameAPI/CServerExoApp.h"
#include "GameAPI/CGuiInGame.h"
#include "GameAPI/CGameObject.h"
#include "GameAPI/CResGFF.h"
#include "GameAPI/CResRef.h"
#include "GameAPI/CSWSCreature.h"
#include "GameAPI/CSWCCreature.h"
#include "GameAPI/CSWGuiButton.h"
#include "GameAPI/CSWGuiPanel.h"

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>

namespace
{
    constexpr int STOCK_NPC_SLOTS = 9;
    constexpr int MAX_NPC_SLOTS = 32;
    constexpr int EXTRA_NPC_SLOTS = MAX_NPC_SLOTS - STOCK_NPC_SLOTS;
    constexpr int PRIMARY_MEMBER_CAPACITY = 2;

    constexpr const char* EXPECTED_K1_SHA =
        "9C10E0450A6EECA417E036E3CDE7474FED1F0A92AAB018446D156944DEA91435";

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
    constexpr WORD GAME_EFFECT_SUBTYPE_MASK = 0x7u;

    constexpr DWORD PARTY_EXTENSION_MAGIC = 0x314B5250u;  // Encodes "PRK1".
    constexpr DWORD PARTY_EXTENSION_VERSION = 1u;

    constexpr int PARTY_SELECT_PAGE_SLOTS = 16;
    constexpr int LAST_PARTY_SELECT_PAGE_BASE =
        MAX_NPC_SLOTS - PARTY_SELECT_PAGE_SLOTS;
    constexpr int PARTY_SELECTION_RECORD_COUNT = MAX_NPC_SLOTS;
    int PARTY_SELECT_SLOT_STRIDE = -1;
    int PANEL_PARTY_SELECTION_DATA_BASE_OFFSET = -1;
    unsigned int STOCK_PARTY_SELECTION_PANEL_SIZE = 0;
    int PANEL_TAIL_SHIFT = 0;
    int GUI_BUTTON_OBJECT_SIZE = -1;
    constexpr int PARTY_SELECTION_PAGE_CONTROL_COUNT = 2;
    int PANEL_PAGE_CONTROLS_BASE_OFFSET = -1;
    unsigned int EXTENDED_PARTY_SELECTION_PANEL_SIZE = 0;

    int DATA_BUTTON_OFFSET = -1;
    int DATA_NOT_AVAILABLE_LABEL_OFFSET = -1;
    int DATA_CHARACTER_LABEL_OFFSET = -1;
    constexpr int DATA_FLAGS_OFFSET = 0x448;
    constexpr int DATA_OBJECT_INDEX_OFFSET = 0x44C;
    constexpr DWORD PANEL_SLOT_ENABLED_FLAG = 0x1u;
    constexpr DWORD PANEL_SLOT_FORCED_FLAG = 0x4u;
    constexpr int PANEL_FORCED_MODE_OFFSET = 0x6C;
    constexpr int PANEL_MODE_OFFSET = 0x70;
    int PANEL_ACCEPT_BUTTON_OFFSET = -1;
    int PANEL_CURRENT_PORTRAIT_OFFSET = -1;
    constexpr int GUI_NAVIGATION_UP = 0;
    constexpr int GUI_NAVIGATION_DOWN = 2;
    constexpr int GUI_CONTROL_EVENT_OWNER_OFFSET = 0x4C;
    constexpr int GUI_CONTROL_PRIMARY_EVENT = 0x27;
    constexpr int GUI_CONTROL_CLICK_EVENT = 0x2D;
    constexpr int IN_GAME_GUI_PARTY_SELECTION_STATE_OFFSET = 0x30;

    int PARTY_SELECTION_RECORD_CONTROL_OFFSETS[3] = {};

    struct PartySlotSnapshot
    {
        DWORD objectId;
        DWORD available;
        BYTE selectable;
    };

#pragma pack(push, 1)
    struct PersistedPartySlot
    {
        DWORD available;
        BYTE selectable;
        BYTE reserved;
    };

    struct PersistedPartyExtension
    {
        DWORD magic;
        PersistedPartySlot slots[EXTRA_NPC_SLOTS];
    };
#pragma pack(pop)

    PartySlotSnapshot gExtendedPartySlots[EXTRA_NPC_SLOTS];

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
    using PartySelectionAcceptFn = void(__thiscall*)(void*);

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
    PartySelectionAcceptFn nativePartySelectionAccept;
    TransferInventoryFn nativeTransferInventory;
    CreatureForceEquipClothingFn nativeCreatureForceEquipClothing;
    ScalarDeletingDestructorFn nativeCreatureDeletingDestructor;
    ScalarDeletingDestructorFn nativeResGffDeletingDestructor;
    ObjectStatusVirtualFn nativeCreatureGetCurrentHitPoints;

    template <typename Fn>
    Fn RequireFunctionAddress(const char* className, const char* functionName)
    {
        void* address = GameVersion::GetFunctionAddress(className, functionName);
        const uintptr_t raw = reinterpret_cast<uintptr_t>(address);
        if (!address || raw > 0x7FFFFFFFu) {
            throw GameVersionException(
                std::string("Invalid function: ") + className + "::" + functionName);
        }
        return reinterpret_cast<Fn>(address);
    }

    int RequireOffset(const char* className, const char* memberName)
    {
        if (!GameVersion::HasOffset(className, memberName)) {
            throw GameVersionException(
                std::string("Missing offset: ") + className + "::" + memberName);
        }
        const int offset = GameVersion::GetOffset(className, memberName);
        if (offset < 0) {
            throw GameVersionException(
                std::string("Invalid offset: ") + className + "::" + memberName);
        }
        return offset;
    }

    unsigned int RequireClassSize(const char* className)
    {
        if (!GameVersion::HasClass(className)) {
            throw GameVersionException(std::string("Missing class size: ") + className);
        }
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
        if (GameVersion::GetTitle() != GameTitle::KOTOR1 ||
            GameVersion::GetVersionSha() != EXPECTED_K1_SHA) {
            OutputDebugStringA("[K1 Party Roster Limit 32] Unsupported GameAPI version\n");
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

        PARTY_SELECT_SLOT_STRIDE = static_cast<int>(RequireClassSize("CSWGuiPartySelectionData"));
        PANEL_PARTY_SELECTION_DATA_BASE_OFFSET =
            RequireOffset("CSWGuiPartySelection", "party_data");
        STOCK_PARTY_SELECTION_PANEL_SIZE = RequireClassSize("CSWGuiPartySelection");
        GUI_BUTTON_OBJECT_SIZE = static_cast<int>(RequireClassSize("CSWGuiButton"));
        DATA_BUTTON_OFFSET = RequireOffset("CSWGuiPartySelectionData", "npc_button");
        DATA_NOT_AVAILABLE_LABEL_OFFSET =
            RequireOffset("CSWGuiPartySelectionData", "portrait_label");
        DATA_CHARACTER_LABEL_OFFSET =
            RequireOffset("CSWGuiPartySelectionData", "character_label");
        PANEL_TAIL_SHIFT = EXTRA_NPC_SLOTS * PARTY_SELECT_SLOT_STRIDE;
        PANEL_PAGE_CONTROLS_BASE_OFFSET =
            static_cast<int>(STOCK_PARTY_SELECTION_PANEL_SIZE) + PANEL_TAIL_SHIFT;
        EXTENDED_PARTY_SELECTION_PANEL_SIZE = static_cast<unsigned int>(
            PANEL_PAGE_CONTROLS_BASE_OFFSET +
            PARTY_SELECTION_PAGE_CONTROL_COUNT * GUI_BUTTON_OBJECT_SIZE);
        PANEL_ACCEPT_BUTTON_OFFSET =
            RequireOffset("CSWGuiPartySelection", "accept_button") + PANEL_TAIL_SHIFT;
        PANEL_CURRENT_PORTRAIT_OFFSET =
            RequireOffset("CSWGuiPartySelection", "accept_button") +
            GUI_BUTTON_OBJECT_SIZE + PANEL_TAIL_SHIFT;
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
        nativePartySelectionAccept = RequireFunctionAddress<PartySelectionAcceptFn>("CSWGuiPartySelection", "AcceptParty");
        nativeTransferInventory = RequireFunctionAddress<TransferInventoryFn>("CSWPartyTable", "TransferInventory");
        nativeCreatureForceEquipClothing = RequireFunctionAddress<CreatureForceEquipClothingFn>("CSWSCreature", "ForceEquipClothing");
        nativeCreatureDeletingDestructor = RequireFunctionAddress<ScalarDeletingDestructorFn>("CSWSCreature", "Destructor_2");
        nativeResGffDeletingDestructor = RequireFunctionAddress<ScalarDeletingDestructorFn>("CResGFF", "Destructor_2");
        nativeCreatureGetCurrentHitPoints = RequireFunctionAddress<ObjectStatusVirtualFn>("CSWSObject", "GetCurrentHitPoints");
        return true;
    }

    bool IsStockSlot(int slot)
    {
        return slot < STOCK_NPC_SLOTS;
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

    void* GetInGameGui()
    {
        CAppManager manager;
        std::unique_ptr<CClientExoApp> client(manager.GetClient());
        std::unique_ptr<CGuiInGame> gui(client->GetInGameGui());
        return gui->GetPtr();
    }

    void* GetServerApplication()
    {
        CAppManager manager;
        std::unique_ptr<CServerExoApp> server(manager.GetServer());
        return server->GetPtr();
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
        CResRef identityRef(identity);
        nativeCreatureLoadFromTemplate(creature, identityRef.GetPtr(), 0);
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

    BYTE* GetPartySelectionDataAt(void* panel, int logicalSlot)
    {
        return reinterpret_cast<BYTE*>(panel) +
            PANEL_PARTY_SELECTION_DATA_BASE_OFFSET +
            logicalSlot * PARTY_SELECT_SLOT_STRIDE;
    }

    BYTE* GetPartySelectionPageControlAt(void* panel, int controlIndex)
    {
        return reinterpret_cast<BYTE*>(panel) +
            PANEL_PAGE_CONTROLS_BASE_OFFSET +
            controlIndex * GUI_BUTTON_OBJECT_SIZE;
    }

    void DetachAllPartySelectionRecordControls(CSWGuiPanel& panelView)
    {
        void* panel = panelView.GetPtr();
        std::unique_ptr<CExoArrayList<CSWGuiControl*>> controls(panelView.GetControls());
        for (int slot = 0; slot < PARTY_SELECTION_RECORD_COUNT; ++slot) {
            BYTE* data = GetPartySelectionDataAt(panel, slot);
            for (int controlOffset : PARTY_SELECTION_RECORD_CONTROL_OFFSETS) {
                CSWGuiControl control(data + controlOffset);
                controls->DeleteAt(controls->IndexOf(&control));
            }
        }
    }

    void AttachAllPartySelectionControlsForStockLifecycle(CSWGuiPanel& panelView)
    {
        void* panel = panelView.GetPtr();
        DetachAllPartySelectionRecordControls(panelView);
        for (int controlOffset : PARTY_SELECTION_RECORD_CONTROL_OFFSETS) {
            for (int slot = 0; slot < PARTY_SELECTION_RECORD_COUNT; ++slot) {
                CSWGuiControl control(GetPartySelectionDataAt(panel, slot) + controlOffset);
                panelView.AddControl(&control);
            }
        }
    }

    void RebindPartySelectionPageNavigation(void* panel, void* target)
    {
        BYTE* accept = reinterpret_cast<BYTE*>(panel) + PANEL_ACCEPT_BUTTON_OFFSET;
        nativeGuiSetMoveToControl(GetPartySelectionPageControlAt(panel, 0), GUI_NAVIGATION_UP, target);
        nativeGuiSetMoveToControl(GetPartySelectionPageControlAt(panel, 1), GUI_NAVIGATION_UP, target);
        nativeGuiSetMoveToControl(accept, GUI_NAVIGATION_UP, target);
        nativeGuiSetMoveToControl(accept, GUI_NAVIGATION_DOWN, target);
    }

    void ClearPartySelectionPortraitContext(void* panel)
    {
        // Clears portrait state without changing selected companions.
        void* previous = getObjectProperty<void*>(panel, PANEL_CURRENT_PORTRAIT_OFFSET);
        if (previous) {
            // Calls the native portrait-button SetActive method.
            CSWGuiButton(previous).SetActive(0);
        }
        setObjectProperty<void*>(panel, PANEL_CURRENT_PORTRAIT_OFFSET, nullptr);
    }

    void ShowPartySelectionPage(void* panel, int pageBase)
    {
        // Enables the destination page before restoring focus.
        CSWGuiButton(GetPartySelectionPageControlAt(panel, 0)).SetEnabled(
            pageBase != 0);
        CSWGuiButton(GetPartySelectionPageControlAt(panel, 1)).SetEnabled(
            pageBase != LAST_PARTY_SELECT_PAGE_BASE);

        CSWGuiPanel panelView(panel);
        panelView.SetActiveControl(nullptr, 0);
        ClearPartySelectionPortraitContext(panel);
        DetachAllPartySelectionRecordControls(panelView);
        const int pageEnd = pageBase + PARTY_SELECT_PAGE_SLOTS;

        for (int slot = pageBase; slot < pageEnd; ++slot) {
            CSWGuiControl character(GetPartySelectionDataAt(panel, slot) +
                DATA_CHARACTER_LABEL_OFFSET);
            character.SetControlBitFlag(5, true);
            panelView.AddControl(&character);
        }

        void* firstInteractive = nullptr;
        for (int slot = pageBase; slot < pageEnd; ++slot) {
            BYTE* data = GetPartySelectionDataAt(panel, slot);
            BYTE* button = data + DATA_BUTTON_OFFSET;
            const DWORD flags = getObjectProperty<DWORD>(
                data,
                DATA_FLAGS_OFFSET);
            const bool interactive =
                (flags & PANEL_SLOT_ENABLED_FLAG) != 0 &&
                (flags & PANEL_SLOT_FORCED_FLAG) == 0;
            CSWGuiButton buttonView(button);
            buttonView.SetEnabled(interactive);
            buttonView.SetControlBitFlag(5, !interactive);
            panelView.AddControl(&buttonView);
            if (!firstInteractive && interactive) {
                firstInteractive = button;
            }
        }

        for (int slot = pageBase; slot < pageEnd; ++slot) {
            CSWGuiControl unavailable(GetPartySelectionDataAt(panel, slot) +
                DATA_NOT_AVAILABLE_LABEL_OFFSET);
            const bool unavailableSlot = unavailable.GetControlBitFlag(1);
            if (slot >= STOCK_NPC_SLOTS && unavailableSlot) {
                continue;
            }
            unavailable.SetControlBitFlag(5, !unavailableSlot);
            panelView.AddControl(&unavailable);
        }

        RebindPartySelectionPageNavigation(panel, firstInteractive);
        BYTE* accept = reinterpret_cast<BYTE*>(panel) + PANEL_ACCEPT_BUTTON_OFFSET;
        // Sets button eligibility before restoring portrait focus.
        CSWGuiButton(accept).SetEnabled(firstInteractive != nullptr);
        if (!firstInteractive) {
            // Uses the page control when no portrait is selectable.
            firstInteractive = GetPartySelectionPageControlAt(
                panel, pageBase == 0 ? 1 : 0);
        }
        CSWGuiControl focusView(firstInteractive);
        panelView.SetActiveControl(&focusView, 1);
    }

    void __fastcall PartySelectionPrevButtonCallback(void* self, void*, void*)
    {
        ShowPartySelectionPage(self, 0);
    }

    void __fastcall PartySelectionNextButtonCallback(void* self, void*, void*)
    {
        ShowPartySelectionPage(self, LAST_PARTY_SELECT_PAGE_BASE);
    }

    void ResetPartySelectionControlState(CSWGuiPanel& panelView)
    {
        panelView.SetActiveControl(nullptr, 0);
        for (int slot = 0; slot < PARTY_SELECTION_RECORD_COUNT; ++slot) {
            BYTE* data = GetPartySelectionDataAt(panelView.GetPtr(), slot);
            CSWGuiButton button(data + DATA_BUTTON_OFFSET);
            CSWGuiControl character(data + DATA_CHARACTER_LABEL_OFFSET);
            CSWGuiControl unavailable(data + DATA_NOT_AVAILABLE_LABEL_OFFSET);
            button.SetControlBitFlag(1, true);
            button.SetControlBitFlag(5, false);
            button.SetEnabled(1);
            character.SetControlBitFlag(1, false);
            unavailable.SetControlBitFlag(1, false);
            character.SetControlBitFlag(5, true);
            unavailable.SetControlBitFlag(5, true);
            nativePartySelectionButtonSetSelected(data + DATA_BUTTON_OFFSET, 0);
        }
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
    PersistedPartyExtension blob;
    blob.magic = PARTY_EXTENSION_MAGIC;
    for (int index = 0; index < EXTRA_NPC_SLOTS; ++index) {
        const PartySlotSnapshot& slot = gExtendedPartySlots[index];
        blob.slots[index].available = slot.available;
        blob.slots[index].selectable = slot.selectable;
        blob.slots[index].reserved = 1;  // Keeps the schema 1 reserved byte.
    }
    char versionLabel[] = "PRK1_VER";
    char dataLabel[] = "PRK1_DATA";
    CResGFF resource(gff);
    auto* parent = static_cast<CResStruct*>(parentStruct);
    resource.WriteFieldDWORD(parent, PARTY_EXTENSION_VERSION, versionLabel);
    resource.WriteFieldVOID(parent, &blob, sizeof(blob), dataLabel);
}

extern "C" void __cdecl ReadExtendedPartyPersistence(
    void* gff,
    void* parentStruct)
{
    ResetExtendedPartyState();
    char dataLabel[] = "PRK1_DATA";
    CResGFF resource(gff);
    auto* parent = static_cast<CResStruct*>(parentStruct);
    PersistedPartyExtension blob;
    int success;
    resource.ReadFieldVOID(parent, &blob, sizeof(blob), dataLabel, &success, nullptr);
    if (!success) {
        return;  // Keeps reset state for saves without extension data.
    }
    for (int index = 0; index < EXTRA_NPC_SLOTS; ++index) {
        PartySlotSnapshot& slot = gExtendedPartySlots[index];
        slot.available = blob.slots[index].available;
        slot.selectable = blob.slots[index].selectable;
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
    std::memset(reinterpret_cast<BYTE*>(panel) + PANEL_PAGE_CONTROLS_BASE_OFFSET,
        0, EXTENDED_PARTY_SELECTION_PANEL_SIZE - PANEL_PAGE_CONTROLS_BASE_OFFSET);

    using PagingCallback = void(__fastcall*)(void*, void*, void*);
    struct PagingButtonSpec
    {
        const char* tag;
        PagingCallback callback;
    };
    const PagingButtonSpec buttonSpecs[
        PARTY_SELECTION_PAGE_CONTROL_COUNT] = {
        {"BTN_PAGE_PREV", &PartySelectionPrevButtonCallback},
        {"BTN_PAGE_NEXT", &PartySelectionNextButtonCallback},
    };

    CSWGuiPanel panelView(panel);

    for (int index = 0;
         index < PARTY_SELECTION_PAGE_CONTROL_COUNT;
         ++index) {
        BYTE* button = GetPartySelectionPageControlAt(panel, index);
        RawCExoString tagString;
        nativeGuiButtonCtor(button);
        nativeCexostringCtorCstr(&tagString, buttonSpecs[index].tag);
        CSWGuiButton buttonView(button);
        CExoString tagView(static_cast<void*>(&tagString));
        panelView.InitControl(&buttonView, &tagView, 1);
        nativeCexostringDtor(&tagString);
        void* const callbackAddress = funcAddr(buttonSpecs[index].callback);
        buttonView.AddEvent(GUI_CONTROL_PRIMARY_EVENT, &panelView, callbackAddress);
        buttonView.AddEvent(GUI_CONTROL_CLICK_EVENT, &panelView, callbackAddress);
    }
    panelView.StopLoadFromLayout();
}

extern "C" void __cdecl PreparePartySelectionDestructor(void* panel)
{
    CSWGuiPanel panelView(panel);
    AttachAllPartySelectionControlsForStockLifecycle(panelView);
    for (int index = PARTY_SELECTION_PAGE_CONTROL_COUNT - 1; index >= 0; --index) {
        nativeGuiButtonDtor(GetPartySelectionPageControlAt(panel, index));
    }
}

extern "C" void __cdecl PreparePartySelectionOnPanelAdded(void* panel)
{
    CSWGuiPanel panelView(panel);
    ResetPartySelectionControlState(panelView);
    AttachAllPartySelectionControlsForStockLifecycle(panelView);
}

extern "C" void __cdecl FinishPartySelectionOnPanelAdded(void* panel)
{
    ShowPartySelectionPage(panel, 0);
}

extern "C" void __cdecl PreparePartySelectionOnPanelRemoved(void* panel)
{
    CSWGuiPanel panelView(panel);
    AttachAllPartySelectionControlsForStockLifecycle(panelView);
}

extern "C" void __cdecl FinishPartySelectionOnPanelRemoved(void* panel)
{
    CSWGuiPanel panelView(panel);
    DetachAllPartySelectionRecordControls(panelView);
    ResetPartySelectionControlState(panelView);
}

extern "C" void __cdecl AdjustPartySelectionAfterObjectRemoval(
    void* panel,
    const int* removedIndex)
{
    for (int slot = 0; slot < PARTY_SELECTION_RECORD_COUNT; ++slot) {
        BYTE* data = GetPartySelectionDataAt(panel, slot);
        const int objectIndex = getObjectProperty<int>(data, DATA_OBJECT_INDEX_OFFSET);
        if (objectIndex > *removedIndex) {
            setObjectProperty<int>(data, DATA_OBJECT_INDEX_OFFSET, objectIndex - 1);
        }
    }
}

extern "C" void __cdecl ClearPartySelectionForcedFlags(void* panel)
{
    // Clears extended slots before resuming native stock-slot cleanup.
    for (int slot = STOCK_NPC_SLOTS; slot < PARTY_SELECTION_RECORD_COUNT; ++slot) {
        BYTE* data = GetPartySelectionDataAt(panel, slot);
        setObjectProperty<DWORD>(data, DATA_FLAGS_OFFSET,
            getObjectProperty<DWORD>(data, DATA_FLAGS_OFFSET) & ~PANEL_SLOT_FORCED_FLAG);
    }
    setObjectProperty<DWORD>(panel, PANEL_FORCED_MODE_OFFSET, 0u);
}

extern "C" void __cdecl PartySelectionOnDone32(
    void* panel,
    void* const* control)
{
    if (getObjectProperty<void*>(*control, GUI_CONTROL_EVENT_OWNER_OFFSET) == nullptr) {
        return;
    }
    bool clearGuiState = false;
    if (getObjectProperty<int>(panel, PANEL_MODE_OFFSET) != 0) {
        int enabledCount = 0;
        int forcedCount = 0;
        for (int slot = 0; slot < PARTY_SELECTION_RECORD_COUNT; ++slot) {
            const DWORD flags = getObjectProperty<DWORD>(
                GetPartySelectionDataAt(panel, slot), DATA_FLAGS_OFFSET);
            enabledCount += (flags & PANEL_SLOT_ENABLED_FLAG) != 0;
            forcedCount += (flags & PANEL_SLOT_FORCED_FLAG) != 0;
        }
        clearGuiState = enabledCount == 0 || forcedCount == PRIMARY_MEMBER_CAPACITY;
    }
    // Accepts the selection without opening the native confirmation prompt.
    nativePartySelectionAccept(panel);
    if (clearGuiState) {
        setObjectProperty<DWORD>(GetInGameGui(), IN_GAME_GUI_PARTY_SELECTION_STATE_OFFSET, 0u);
    }
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

// Initializes after KPM supplies the version hash and address database.
extern "C" int __stdcall DllMain(void*, DWORD reason, void*)
{
    if (reason == DLL_PROCESS_ATTACH) {
        try {
            if (InitializePatchApi()) {
                return 1;
            }
        } catch (const std::exception& error) {
            debugLog("[K1 Party Roster Limit 32] GameAPI initialization failed: %s\n", error.what());
        } catch (...) {
            OutputDebugStringA("[K1 Party Roster Limit 32] GameAPI initialization failed\n");
        }
        GameVersion::Reset(true);
        return 0;
    }
    if (reason == DLL_PROCESS_DETACH) {
        GameVersion::Shutdown();
    }
    return 1;
}
