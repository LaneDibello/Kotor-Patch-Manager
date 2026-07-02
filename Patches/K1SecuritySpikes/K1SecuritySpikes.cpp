// K1SecuritySpikes.cpp
// Source-only KotorPatchManager DLL patch for KOTOR1 GOG 1.03.

#if defined(_WIN64) || !defined(_M_IX86)
#error K1SecuritySpikes must be compiled as a 32-bit x86 DLL.
#endif

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef unsigned int   usize;
typedef int            BOOL;
typedef void*          HANDLE;
typedef void*          HMODULE;
typedef void*          LPVOID;
typedef const void*    LPCVOID;
typedef const char*    LPCSTR;
typedef char*          LPSTR;
typedef u32            DWORD;

#define WINAPI __stdcall
#define TRUE 1
#define FALSE 0
#define DLL_PROCESS_DETACH 0u
#define DLL_PROCESS_ATTACH 1u
#define PAGE_EXECUTE_READWRITE 0x40u

extern "C" {
__declspec(dllimport) BOOL    WINAPI VirtualProtect(LPVOID, usize, DWORD, DWORD*);
__declspec(dllimport) BOOL    WINAPI FlushInstructionCache(HANDLE, LPCVOID, usize);
__declspec(dllimport) HANDLE  WINAPI GetCurrentProcess(void);
__declspec(dllimport) HMODULE WINAPI GetModuleHandleA(LPCSTR);
__declspec(dllimport) DWORD   WINAPI GetEnvironmentVariableA(LPCSTR, LPSTR, DWORD);
__declspec(dllimport) void    WINAPI OutputDebugStringA(LPCSTR);
__declspec(dllimport) BOOL    WINAPI DisableThreadLibraryCalls(HMODULE);
}

void DoorSecuritySpikePostIconStub();
void PlaceableSecuritySpikePostIconStub();
void MenuActionUseSecuritySpike_Thunk();

namespace {

#define KOTOR1_GOG_SHA256 \
    "9C10E0450A6EECA417E036E3CDE7474FED1F0A92AAB018446D156944DEA91435"

constexpr u32 GAME_PREFERRED_BASE = 0x00400000u;
constexpr u32 APP_MANAGER_PTR = 0x007A39FCu;

constexpr u32 VA_DOOR_POST_SECURITY_ICON = 0x00683919u;
constexpr u32 VA_DOOR_TARGET_EXIT        = 0x00683B86u;
constexpr u32 VA_PLACE_POST_SECURITY_ICON = 0x00684559u;
constexpr u32 VA_PLACE_TARGET_EXIT        = 0x00684634u;

constexpr u32 CClientExoApp_GetSWCMessage = 0x005ED6F0u;
constexpr u32 CClientExoApp_GetTlkString = 0x005EDEB0u;
constexpr u32 CSWCMessage_SendUnlockObject = 0x00677DE0u;
constexpr u32 CSWCObject_ClearAllActions = 0x0063D470u;
constexpr u32 CSWCObject_GetServerObject = 0x0063D4B0u;
constexpr u32 CSWSCreature_GetItemRepository = 0x004EF770u;
constexpr u32 CItemRepository_GetItem = 0x005560B0u;
constexpr u32 CItemRepository_GetItemObjectID = 0x005560E0u;
constexpr u32 CSWSItem_GetIcon = 0x005556B0u;
constexpr u32 CSWSItem_GetPropertyByTypeExists = 0x005539C0u;
constexpr u32 CExoArrayList_SetSize = 0x004FDDD0u;
constexpr u32 CExoString_FromCStr = 0x005E5A90u;
constexpr u32 CExoString_Destructor = 0x005E5C20u;
constexpr u32 CExoString_Assign = 0x005E5C50u;

constexpr u32 OBJECT_ID_INVALID = 0x7F000000u;
constexpr u32 ACTION_ITEM_FLAG = 0x40000000u;
constexpr u32 ACTION_ITEM_MASK = 0xBFFFFFFFu;
constexpr u16 SECURITY_SPIKE_PROPERTY_TYPE = 0x25u;
constexpr u32 CSWSITEM_LOCALIZED_NAME_STRREF_OFFSET = 0x284u;
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

struct JumpPatch {
    u8* target;
    u8 original[5];
    int installed;
};

typedef void* (__thiscall *GetSWCMessageFn)(void* client);
typedef void  (__thiscall *SendUnlockObjectFn)(void* message, u32 target_id, u32 item_id);
typedef void  (__thiscall *ClearAllActionsFn)(void* creature);
typedef void* (__thiscall *GetServerObjectFn)(void* client_object);
typedef void* (__thiscall *GetItemRepositoryFn)(void* server_creature, int repository_type);
typedef void* (__thiscall *GetItemFn)(void* repository, int index);
typedef u32   (__thiscall *GetItemObjectIdFn)(void* repository, int index);
typedef CResRef* (__thiscall *GetIconFn)(void* server_item, CResRef* out_icon);
typedef CExoString* (__thiscall *GetTlkStringFn)(void* client, CExoString* out_text, u32 strref);
typedef int   (__thiscall *GetPropertyByTypeExistsFn)(void* server_item, void** out_property, u16 property_type, u16 sub_type);
typedef void* (__thiscall *SetSizeFn)(CSWGuiInterfaceActionList* list, int size);
typedef CExoString* (__thiscall *StringFromCStrFn)(CExoString* str, const char* text);
typedef void  (__thiscall *StringDestructorFn)(CExoString* str);
typedef CExoString* (__thiscall *StringAssignFn)(CExoString* dst, const CExoString* src);

u8* g_game_base = 0;
JumpPatch g_door_patch = {};
JumpPatch g_placeable_patch = {};

u8* GameAddress(u32 preferred_va) {
    return g_game_base + (preferred_va - GAME_PREFERRED_BASE);
}

void Log(const char* text) {
    OutputDebugStringA(text);
}

char UpperAscii(char ch) {
    return (ch >= 'a' && ch <= 'z') ? (char)(ch - ('a' - 'A')) : ch;
}

int StringEqualsNoCase(const char* left, const char* right) {
    if (!left || !right) {
        return 0;
    }
    while (*left && *right) {
        if (UpperAscii(*left) != UpperAscii(*right)) {
            return 0;
        }
        ++left;
        ++right;
    }
    return *left == '\0' && *right == '\0';
}

int IsSupportedVersion(void) {
    char hash[80];
    DWORD length;

    hash[0] = '\0';
    length = GetEnvironmentVariableA("KOTOR_VERSION_SHA", hash, (DWORD)sizeof(hash));
    return length > 0u && length < (DWORD)sizeof(hash) &&
        StringEqualsNoCase(hash, KOTOR1_GOG_SHA256);
}

void CopyBytes(u8* dst, const u8* src, u32 count) {
    for (u32 i = 0u; i < count; ++i) {
        dst[i] = src[i];
    }
}

int BytesEqual(const u8* left, const u8* right, u32 count) {
    for (u32 i = 0u; i < count; ++i) {
        if (left[i] != right[i]) {
            return 0;
        }
    }
    return 1;
}

void WriteU32(u8* dst, u32 value) {
    dst[0] = (u8)(value & 0xffu);
    dst[1] = (u8)((value >> 8) & 0xffu);
    dst[2] = (u8)((value >> 16) & 0xffu);
    dst[3] = (u8)((value >> 24) & 0xffu);
}

void WriteRelativeJump(u8* source, const void* destination) {
    u32 source_address = (u32)(usize)source;
    u32 destination_address = (u32)(usize)destination;
    source[0] = 0xE9u;
    WriteU32(source + 1, destination_address - (source_address + 5u));
}

int InstallJump(JumpPatch* patch, u32 target_va, void* destination, const u8 expected[5]) {
    DWORD old_protection = 0u;
    DWORD ignored = 0u;

    if (!patch || !destination || !expected) {
        return 0;
    }

    patch->target = GameAddress(target_va);
    patch->installed = 0;
    if (!BytesEqual(patch->target, expected, 5u)) {
        Log("[K1SecuritySpikes] Hook-byte verification failed.\n");
        return 0;
    }

    CopyBytes(patch->original, patch->target, 5u);
    if (!VirtualProtect(patch->target, 5u, PAGE_EXECUTE_READWRITE, &old_protection)) {
        Log("[K1SecuritySpikes] VirtualProtect failed.\n");
        return 0;
    }

    WriteRelativeJump(patch->target, destination);
    FlushInstructionCache(GetCurrentProcess(), patch->target, 5u);
    VirtualProtect(patch->target, 5u, old_protection, &ignored);
    patch->installed = 1;
    return 1;
}

void RemoveJump(JumpPatch* patch) {
    DWORD old_protection = 0u;
    DWORD ignored = 0u;

    if (!patch || !patch->installed) {
        return;
    }

    if (VirtualProtect(patch->target, 5u, PAGE_EXECUTE_READWRITE, &old_protection)) {
        CopyBytes(patch->target, patch->original, 5u);
        FlushInstructionCache(GetCurrentProcess(), patch->target, 5u);
        VirtualProtect(patch->target, 5u, old_protection, &ignored);
    }

    patch->target = 0;
    patch->installed = 0;
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
    void* app_manager = *(void**)GameAddress(APP_MANAGER_PTR);
    return ReadPtr(app_manager, 0x04u);
}

int SetActionLabelFromStrRef(CSWGuiInterfaceAction* action, u32 strref) {
    CExoString temp;
    void* client;
    GetTlkStringFn get_text;
    StringAssignFn assign;
    StringDestructorFn destroy;

    if (!action || strref == 0xFFFFFFFFu) {
        return 0;
    }

    client = ClientExoApp();
    if (!client) {
        return 0;
    }

    temp.text = 0;
    temp.allocation_size = 0u;
    get_text = (GetTlkStringFn)GameAddress(CClientExoApp_GetTlkString);
    assign = (StringAssignFn)GameAddress(CExoString_Assign);
    destroy = (StringDestructorFn)GameAddress(CExoString_Destructor);

    get_text(client, &temp, strref);
    if (!temp.text || temp.text[0] == '\0') {
        destroy(&temp);
        return 0;
    }

    assign(&action->label, &temp);
    destroy(&temp);
    return 1;
}

void SetActionLabel(CSWGuiInterfaceAction* action, void* item, u16 bonus) {
    u32 strref;
    CExoString temp;
    StringFromCStrFn from_cstr;
    StringAssignFn assign;
    StringDestructorFn destroy;

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
    from_cstr = (StringFromCStrFn)GameAddress(CExoString_FromCStr);
    assign = (StringAssignFn)GameAddress(CExoString_Assign);
    destroy = (StringDestructorFn)GameAddress(CExoString_Destructor);

    from_cstr(&temp, bonus >= 10u ? "Security Spike Tunneler" : "Security Spike");
    assign(&action->label, &temp);
    destroy(&temp);
}

u16 SecuritySpikeBonus(void* item) {
    void* property = 0;
    GetPropertyByTypeExistsFn has_property;

    if (!item) {
        return 0u;
    }

    has_property = (GetPropertyByTypeExistsFn)GameAddress(CSWSItem_GetPropertyByTypeExists);
    if (has_property(item, &property, SECURITY_SPIKE_PROPERTY_TYPE, 0) == 0 || !property) {
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
    SetSizeFn set_size;
    GetIconFn get_icon;

    if (!actions || !ValidObjectId(target_id) || !ValidObjectId(item_id) ||
        AlreadyAdded(actions, item_id, target_id)) {
        return;
    }

    index = actions->size;
    if (index < 0 || index > 255) {
        return;
    }

    set_size = (SetSizeFn)GameAddress(CExoArrayList_SetSize);
    set_size(actions, index + 1);
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

    get_icon = (GetIconFn)GameAddress(CSWSItem_GetIcon);
    get_icon(item, &action->icon);
}

void AppendSecuritySpikes(CSWGuiInterfaceActionList* actions, void* client_creature) {
    u32 target_id;
    void* server_creature;
    void* repository;
    int count;
    GetServerObjectFn get_server_object;
    GetItemRepositoryFn get_repository;
    GetItemFn get_item;
    GetItemObjectIdFn get_item_id;

    if (!actions || !actions->data || actions->size <= 0 || actions->size > 256 || !client_creature) {
        return;
    }

    target_id = actions->data[actions->size - 1].target_object_id;
    if (!ValidObjectId(target_id)) {
        return;
    }

    get_server_object = (GetServerObjectFn)GameAddress(CSWCObject_GetServerObject);
    server_creature = get_server_object(client_creature);
    if (!server_creature) {
        return;
    }

    get_repository = (GetItemRepositoryFn)GameAddress(CSWSCreature_GetItemRepository);
    repository = get_repository(server_creature, 1);
    if (!repository) {
        return;
    }

    count = *(int*)((u8*)repository + 0x10u);
    if (count <= 0) {
        return;
    }
    if (count > 512) {
        count = 512;
    }

    get_item = (GetItemFn)GameAddress(CItemRepository_GetItem);
    get_item_id = (GetItemObjectIdFn)GameAddress(CItemRepository_GetItemObjectID);

    for (int i = 0; i < count; ++i) {
        void* item = get_item(repository, i);
        u16 bonus = SecuritySpikeBonus(item);
        if (bonus == 0u) {
            continue;
        }
        AddSpikeAction(actions, target_id, item, get_item_id(repository, i), bonus);
    }
}

void UseSecuritySpike(void* target_object, u32 action_id, void* creature) {
    u32 item_id;
    u32 target_id;
    void* client;
    void* message;
    GetSWCMessageFn get_message;
    SendUnlockObjectFn send_unlock;

    if (!target_object) {
        return;
    }

    item_id = action_id & ACTION_ITEM_MASK;
    if (!ValidObjectId(item_id)) {
        return;
    }

    if (creature) {
        ClearAllActionsFn clear_actions = (ClearAllActionsFn)GameAddress(CSWCObject_ClearAllActions);
        clear_actions(creature);
    }

    client = ClientExoApp();
    if (!client) {
        return;
    }

    get_message = (GetSWCMessageFn)GameAddress(CClientExoApp_GetSWCMessage);
    message = get_message(client);
    if (!message) {
        return;
    }

    target_id = ReadU32(target_object, 0x04u);
    if (!ValidObjectId(target_id)) {
        return;
    }

    send_unlock = (SendUnlockObjectFn)GameAddress(CSWCMessage_SendUnlockObject);
    send_unlock(message, target_id, item_id);
}

void RemoveAllHooks(void) {
    RemoveJump(&g_placeable_patch);
    RemoveJump(&g_door_patch);
}

int InstallAllHooks(void) {
    static const u8 expected_door_jump[5] = { 0xE9u, 0x68u, 0x02u, 0x00u, 0x00u };
    static const u8 expected_placeable_jump[5] = { 0xE9u, 0xD6u, 0x00u, 0x00u, 0x00u };

    if (!InstallJump(&g_door_patch, VA_DOOR_POST_SECURITY_ICON,
            (void*)::DoorSecuritySpikePostIconStub, expected_door_jump)) {
        RemoveAllHooks();
        return 0;
    }

    if (!InstallJump(&g_placeable_patch, VA_PLACE_POST_SECURITY_ICON,
            (void*)::PlaceableSecuritySpikePostIconStub, expected_placeable_jump)) {
        RemoveAllHooks();
        return 0;
    }

    Log("[K1SecuritySpikes] Hooks installed.\n");
    return 1;
}

} // namespace

extern "C" void __cdecl K1AppendSecuritySpikes(void* actions, void* client_creature) {
    AppendSecuritySpikes((CSWGuiInterfaceActionList*)actions, client_creature);
}

void __declspec(naked) DoorSecuritySpikePostIconStub() {
    __asm {
        pushad
        push dword ptr [esp + 94h]
        push esi
        call K1AppendSecuritySpikes
        add  esp, 8
        popad
        mov  eax, 00683B86h
        jmp  eax
    }
}

void __declspec(naked) PlaceableSecuritySpikePostIconStub() {
    __asm {
        pushad
        push dword ptr [esp + 7Ch]
        push esi
        call K1AppendSecuritySpikes
        add  esp, 8
        popad
        mov  eax, 00684634h
        jmp  eax
    }
}

void __declspec(naked) MenuActionUseSecuritySpike_Thunk() {
    __asm {
        push dword ptr [esp + 8]
        push dword ptr [esp + 8]
        push ecx
        call UseSecuritySpike
        add  esp, 12
        ret  8
    }
}

extern "C" BOOL WINAPI DllMain(HMODULE module, DWORD reason, LPVOID reserved) {
    (void)reserved;

    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        g_game_base = (u8*)GetModuleHandleA(0);
        if (!g_game_base || !IsSupportedVersion()) {
            Log("[K1SecuritySpikes] Unsupported executable.\n");
            return FALSE;
        }
        if (!InstallAllHooks()) {
            Log("[K1SecuritySpikes] Initialization failed.\n");
            return FALSE;
        }
    } else if (reason == DLL_PROCESS_DETACH) {
        RemoveAllHooks();
    }

    return TRUE;
}
