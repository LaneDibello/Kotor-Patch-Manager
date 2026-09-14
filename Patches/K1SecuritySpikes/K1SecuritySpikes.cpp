// K1SecuritySpikes.cpp
// Source-only KotorPatchManager patch for KOTOR1 1.03.
//
// KOTOR1 ships Security Spikes as items and implements the mechanic on the server:
// CSWSObject::AIActionUnlockObject looks up the item that came with the unlock action, adds
// its property bonus to the character's Security rank, and spends one spike from the stack.
// Nothing in the interface ever sends an item. The stock Security entry on a locked door or
// container calls SendPlayerToServerInput_UnlockObject with OBJECT_INVALID in the item slot,
// so the spikes sit in the inventory with no way to use them.
//
// This patch supplies the missing menu entries. Both hooks sit at the tail of the Security
// block in CSWCPlaceable::GetTargetActions and CSWCDoor::GetTargetActions, so they run only
// once the game has decided the object is locked and the character can use Security. One
// action is appended per spike the character carries, tagged with that spike's object id,
// and choosing it sends the unlock message with the id in place of OBJECT_INVALID.
//
// Doors only. HandlePlayerToServerInputMessage forwards the item id for a door but replaces
// it with OBJECT_INVALID for a placeable at 0x00525C9C, so a spike used on a container is
// resolved as a plain Security check and is not spent.

#if defined(_WIN64) || (defined(_M_IX86) == 0 && defined(__i386__) == 0)
#error K1SecuritySpikes must be compiled as a 32-bit x86 module.
#endif

#include "Common.h"
#include "GameAPI/GameVersion.h"

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef unsigned int   usize;

void __fastcall MenuActionUseSecuritySpike_Thunk(void*, void*, u32, void*);

namespace {

constexpr u32 OBJECT_ID_INVALID = 0x7F000000u;

// The game puts a small constant in an action's id field for its own entries, 0x3F3 for
// Security and 0x3F5 for Bash. A spike entry stores the item's object id there instead and
// sets bit 30, so a later pass over the list can tell its own entries apart.
constexpr u32 ACTION_ITEM_FLAG = 0x40000000u;
constexpr u32 ACTION_ITEM_MASK = 0xBFFFFFFFu;

// Marks an item as a spike. The u16 at +0x06 of the property is the bonus the server adds
// to the Security rank, so finding the property is also the test for whether an item counts.
constexpr u16 SECURITY_SPIKE_PROPERTY_TYPE = 0x25u;

// CSWSItem field. The database carries no offsets for the item classes.
constexpr u32 CSWSITEM_LOCALIZED_NAME_STRREF_OFFSET = 0x284u;

// dialog.tlk names for the two spike items, used when the item has no name of its own.
constexpr u32 TLK_SECURITY_SPIKE = 5980u;
constexpr u32 TLK_SECURITY_SPIKE_TUNNELER = 5982u;

struct CExoString {
    char* text;
    u32 allocation_size;
};

struct CResRef {
    char value[16];
};

struct CSWGuiInterfaceAction {
    CExoString label;
    u32 action_id;
    u32 action_function[4];
    u32 target_object_id;
    CResRef icon;
    u32 flags;
    u32 field_34;
};

struct CSWGuiInterfaceActionList {
    CSWGuiInterfaceAction* data;
    int size;
    int capacity;
};

typedef void* (__thiscall *GetSWCMessageFn)(void* client);
typedef void  (__thiscall *SendUnlockObjectFn)(void* message, u32 target_id, u32 item_id);
typedef void  (__thiscall *ClearAllActionsFn)(void* creature);
typedef void* (__thiscall *GetServerObjectFn)(void* client_object);
typedef void* (__thiscall *GetItemRepositoryFn)(void* server_creature, int repository_type);
typedef void* (__thiscall *GetItemFn)(void* repository, int index);
typedef u32   (__thiscall *GetItemObjectIdFn)(void* repository, int index);
typedef CResRef* (__thiscall *GetIconFn)(void* server_item, CResRef* out_icon);
typedef CExoString* (__thiscall *GetGUIStringFn)(void* client, CExoString* out_text, u32 strref);
typedef int   (__thiscall *GetPropertyByTypeFn)(void* server_item, void** out_property, u16 property_type, u16 sub_type);
typedef void* (__thiscall *SetSizeFn)(CSWGuiInterfaceActionList* list, int size);
typedef CExoString* (__thiscall *StringFromCStrFn)(CExoString* str, const char* text);
typedef void  (__thiscall *StringDestructorFn)(CExoString* str);
typedef CExoString* (__thiscall *StringAssignFn)(CExoString* dst, const CExoString* src);

void** g_app_manager = 0;
GetSWCMessageFn g_get_swc_message = 0;
GetGUIStringFn g_get_gui_string = 0;
SendUnlockObjectFn g_send_unlock_object = 0;
ClearAllActionsFn g_clear_all_actions = 0;
GetServerObjectFn g_get_server_object = 0;
GetItemRepositoryFn g_get_item_repository = 0;
GetItemFn g_item_list_get_item = 0;
GetItemObjectIdFn g_item_list_get_item_id = 0;
GetIconFn g_get_icon = 0;
GetPropertyByTypeFn g_get_property_by_type = 0;
SetSizeFn g_set_size = 0;
StringFromCStrFn g_string_from_cstr = 0;
StringDestructorFn g_string_destroy = 0;
StringAssignFn g_string_assign = 0;

// A lookup is a database query, so everything resolves once at load and the rest of the
// patch calls through the pointers. CExoString::Destructor is a thunk in the database;
// _2 is the body the game itself calls.
bool ResolveGameAddresses() {
    g_app_manager = (void**)GameVersion::GetGlobalPointer("APP_MANAGER_PTR");

    return g_app_manager
        && GameVersion::ResolveFunction(g_get_swc_message, "CClientExoApp", "GetSWCMessage")
        && GameVersion::ResolveFunction(g_get_gui_string, "CClientExoApp", "GetGUIString")
        && GameVersion::ResolveFunction(g_send_unlock_object, "CSWCMessage", "SendPlayerToServerInput_UnlockObject")
        && GameVersion::ResolveFunction(g_clear_all_actions, "CSWCObject", "ClearAllActions")
        && GameVersion::ResolveFunction(g_get_server_object, "CSWCObject", "GetServerObject")
        && GameVersion::ResolveFunction(g_get_item_repository, "CSWSCreature", "GetItemRepository")
        && GameVersion::ResolveFunction(g_item_list_get_item, "CItemRepository", "ItemListGetItem")
        && GameVersion::ResolveFunction(g_item_list_get_item_id, "CItemRepository", "ItemListGetItemObjectID")
        && GameVersion::ResolveFunction(g_get_icon, "CSWSItem", "GetIcon")
        && GameVersion::ResolveFunction(g_get_property_by_type, "CSWSItem", "GetPropertyByType")
        && GameVersion::ResolveFunction(g_set_size, "CExoArrayList__", "SetSize")
        && GameVersion::ResolveFunction(g_string_from_cstr, "CExoString", "CStrConstructor")
        && GameVersion::ResolveFunction(g_string_destroy, "CExoString", "Destructor_2")
        && GameVersion::ResolveFunction(g_string_assign, "CExoString", "operator=_2");
}

void* ReadPtr(void* base, u32 offset) {
    return base ? *(void**)((u8*)base + offset) : 0;
}

u32 ReadU32(void* base, u32 offset) {
    return base ? *(u32*)((u8*)base + offset) : 0u;
}

u16 ReadU16(void* base, u32 offset) {
    return base ? *(u16*)((u8*)base + offset) : 0u;
}

int ValidObjectId(u32 object_id) {
    return object_id != 0u && object_id != OBJECT_ID_INVALID && object_id != 0xFFFFFFFFu;
}

void* ClientExoApp(void) {
    return ReadPtr(*g_app_manager, 0x04u);  // CAppManager::client
}

int SetActionLabelFromStrRef(CSWGuiInterfaceAction* action, u32 strref) {
    CExoString temp;
    void* client;

    if (!action || strref == 0xFFFFFFFFu) {
        return 0;
    }

    client = ClientExoApp();
    if (!client) {
        return 0;
    }

    temp.text = 0;
    temp.allocation_size = 0u;
    g_get_gui_string(client, &temp, strref);
    if (!temp.text || temp.text[0] == '\0') {
        g_string_destroy(&temp);
        return 0;
    }

    g_string_assign(&action->label, &temp);
    g_string_destroy(&temp);
    return 1;
}

void SetActionLabel(CSWGuiInterfaceAction* action, void* item, u16 bonus) {
    u32 strref;
    CExoString temp;

    if (!action) {
        return;
    }

    strref = ReadU32(item, CSWSITEM_LOCALIZED_NAME_STRREF_OFFSET);
    if (SetActionLabelFromStrRef(action, strref)) {
        return;
    }

    if (SetActionLabelFromStrRef(
            action,
            bonus >= 10u ? TLK_SECURITY_SPIKE_TUNNELER : TLK_SECURITY_SPIKE)) {
        return;
    }

    temp.text = 0;
    temp.allocation_size = 0u;
    g_string_from_cstr(&temp, bonus >= 10u ? "Security Spike Tunneler" : "Security Spike");
    g_string_assign(&action->label, &temp);
    g_string_destroy(&temp);
}

u16 SecuritySpikeBonus(void* item) {
    void* property = 0;

    if (!item) {
        return 0u;
    }

    if (g_get_property_by_type(item, &property, SECURITY_SPIKE_PROPERTY_TYPE, 0) == 0 || !property) {
        return 0u;
    }

    return ReadU16(property, 0x06u);
}

int AlreadyAdded(CSWGuiInterfaceActionList* actions, u32 item_id, u32 target_id) {
    u32 wanted_action_id = item_id | ACTION_ITEM_FLAG;
    u32 callback = (u32)(usize)&::MenuActionUseSecuritySpike_Thunk;

    if (!actions || !actions->data || actions->size <= 0 || actions->size > 256) {
        return 0;
    }

    for (int i = 0; i < actions->size; ++i) {
        CSWGuiInterfaceAction* action = &actions->data[i];
        if (action->action_id == wanted_action_id &&
            action->action_function[0] == callback &&
            action->target_object_id == target_id) {
            return 1;
        }
    }

    return 0;
}

void AddSpikeAction(CSWGuiInterfaceActionList* actions, u32 target_id, void* item, u32 item_id, u16 bonus) {
    int index;
    CSWGuiInterfaceAction* action;

    if (!actions || !ValidObjectId(target_id) || !ValidObjectId(item_id) ||
        AlreadyAdded(actions, item_id, target_id)) {
        return;
    }

    index = actions->size;
    if (index < 0 || index > 255) {
        return;
    }

    g_set_size(actions, index + 1);
    if (!actions->data || actions->size <= index) {
        return;
    }

    action = &actions->data[index];
    SetActionLabel(action, item, bonus);
    action->action_id = item_id | ACTION_ITEM_FLAG;
    action->action_function[0] = (u32)(usize)&::MenuActionUseSecuritySpike_Thunk;
    action->action_function[1] = 0u;
    action->action_function[2] = 0u;
    action->action_function[3] = 0u;
    action->target_object_id = target_id;

    g_get_icon(item, &action->icon);
}

void AppendSecuritySpikes(CSWGuiInterfaceActionList* actions, void* client_creature) {
    u32 target_id;
    void* server_creature;
    void* repository;
    int count;

    if (!actions || !actions->data || actions->size <= 0 || actions->size > 256 || !client_creature) {
        return;
    }

    target_id = actions->data[actions->size - 1].target_object_id;
    if (!ValidObjectId(target_id)) {
        return;
    }

    server_creature = g_get_server_object(client_creature);
    if (!server_creature) {
        return;
    }

    // Repository 1 is the inventory, the same one the server empties the spike from.
    repository = g_get_item_repository(server_creature, 1);
    if (!repository) {
        return;
    }

    count = *(int*)((u8*)repository + 0x10u);  // CItemRepository item count
    if (count <= 0) {
        return;
    }
    if (count > 512) {
        count = 512;
    }

    for (int i = 0; i < count; ++i) {
        void* item = g_item_list_get_item(repository, i);
        u16 bonus = SecuritySpikeBonus(item);
        if (bonus == 0u) {
            continue;
        }
        AddSpikeAction(actions, target_id, item, g_item_list_get_item_id(repository, i), bonus);
    }
}

void UseSecuritySpike(void* target_object, u32 action_id, void* creature) {
    u32 item_id;
    u32 target_id;
    void* client;
    void* message;

    if (!target_object) {
        return;
    }

    item_id = action_id & ACTION_ITEM_MASK;
    if (!ValidObjectId(item_id)) {
        return;
    }

    if (creature) {
        g_clear_all_actions(creature);
    }

    client = ClientExoApp();
    if (!client) {
        return;
    }

    message = g_get_swc_message(client);
    if (!message) {
        return;
    }

    target_id = ReadU32(target_object, 0x04u);  // CSWCObject::object.id
    if (!ValidObjectId(target_id)) {
        return;
    }

    g_send_unlock_object(message, target_id, item_id);
}

} // namespace

// Both hook sites land here. The creature is the party member whose action menu is open. A
// stack parameter arrives as the slot's address, hence the indirection on it. Returning
// non-zero takes the hook's consumed exit.
extern "C" int __cdecl K1AppendSecuritySpikes(void* actions, void* const* creature_slot) {
    if (actions && creature_slot) {
        AppendSecuritySpikes((CSWGuiInterfaceActionList*)actions, *creature_slot);
    }
    return 1;
}

// The game stores this in its action structure and calls it __thiscall. __fastcall has the
// same shape once EDX is ignored, as in Common/MemberFunctionThunk.h.
void __fastcall MenuActionUseSecuritySpike_Thunk(void* target_object, void* /*edx*/,
                                                 u32 action_id, void* creature) {
    UseSecuritySpike(target_object, action_id, creature);
}

extern "C" BOOL WINAPI DllMain(HMODULE module, DWORD reason, LPVOID reserved) {
    (void)reserved;

    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        if (!GameVersion::Initialize()) {
            debugLog("[K1SecuritySpikes] GameVersion::Initialize failed.\n");
            return FALSE;
        }
        if (!ResolveGameAddresses()) {
            debugLog("[K1SecuritySpikes] The address database is missing an entry this patch needs.\n");
            return FALSE;
        }
    } else if (reason == DLL_PROCESS_DETACH) {
        GameVersion::Reset();
    }

    return TRUE;
}
