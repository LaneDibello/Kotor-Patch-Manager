#include <array>
#include <cstddef>
#include <cstdint>

namespace {

constexpr int MAX_ACTION_BUTTON_COUNT = 9;
constexpr std::uint32_t CONTROL_VISIBLE = 0x0002;
constexpr std::uint32_t PANEL_IGNORED_BY_INPUT = 0x0600;

constexpr std::uint32_t DIK_UP = 0xC8;
constexpr std::uint32_t DIK_DOWN = 0xD0;
constexpr std::uint32_t DIK_LEFT = 0xCB;
constexpr std::uint32_t DIK_RIGHT = 0xCD;
constexpr std::uint32_t DIK_RETURN = 0x1C;
constexpr std::uint32_t DIK_ESCAPE = 0x01;
constexpr std::uint32_t DIK_DELETE = 0xD3;
constexpr std::uint32_t DIK_SPACE = 0x39;
constexpr std::uint32_t KEY_PRESSED = 0x80;

constexpr std::uint32_t PENDING_UP = 1 << 0;
constexpr std::uint32_t PENDING_DOWN = 1 << 1;
constexpr std::uint32_t PENDING_LEFT = 1 << 2;
constexpr std::uint32_t PENDING_RIGHT = 1 << 3;
constexpr std::uint32_t PENDING_ACTIVATE = 1 << 4;
constexpr std::uint32_t PENDING_CONTEXT_CANCEL = 1 << 5;
constexpr std::uint32_t PENDING_CONTROLLER_X = 1 << 6;
constexpr std::uint32_t PENDING_CONTROLLER_LEFT = 1 << 7;
constexpr std::uint32_t PENDING_CONTROLLER_RIGHT = 1 << 8;
constexpr std::uint32_t PENDING_K1_JOURNAL_ITEMS = 1 << 9;
constexpr std::uint32_t PENDING_K1_JOURNAL_TOGGLE = 1 << 10;
constexpr std::uint32_t PENDING_K1_JOURNAL_SORT = 1 << 11;
constexpr std::uint32_t PENDING_K1_PAZAAK_SWITCH_GRID = 1 << 12;
constexpr std::uint32_t PENDING_K1_PAZAAK_PLAY = 1 << 13;
constexpr std::uint32_t PENDING_K1_PAZAAK_RETURN = 1 << 14;
constexpr std::uint32_t PENDING_K1_PAZAAK_INITIAL_FOCUS = 1 << 15;
constexpr std::uint32_t PENDING_K1_PAZAAK_GAME_LEFT = 1 << 16;
constexpr std::uint32_t PENDING_K1_PAZAAK_GAME_RIGHT = 1 << 17;
constexpr std::uint32_t PENDING_K1_PAZAAK_GAME_PLAY_CARD = 1 << 18;
constexpr std::uint32_t PENDING_K1_PARTY_BUTTON_ROW = 1 << 19;
constexpr std::uint32_t PENDING_K1_PARTY_GRID_RETURN = 1 << 20;
constexpr std::uint32_t PENDING_K1_PAZAAK_GAME_FLIP_DOWN = 1 << 21;
constexpr std::uint32_t PENDING_K1_PAZAAK_GAME_FLIP_UP = 1 << 22;
constexpr std::uint32_t PENDING_K1_PAZAAK_GAME_FLIP_CARD = 1 << 23;
constexpr std::uint32_t PENDING_K1_FEATS_SELECT = 1 << 24;
constexpr std::uint32_t PENDING_K1_FEATS_ACCEPT = 1 << 25;
constexpr std::uint32_t PENDING_K1_POWERS_SELECT = 1 << 26;
constexpr std::uint32_t PENDING_ACTION_BAR_INPUT =
    PENDING_UP | PENDING_DOWN | PENDING_LEFT | PENDING_RIGHT | PENDING_ACTIVATE;
constexpr std::uint32_t PENDING_CONTEXT_INPUT =
    PENDING_CONTEXT_CANCEL | PENDING_CONTROLLER_X |
    PENDING_CONTROLLER_LEFT | PENDING_CONTROLLER_RIGHT |
    PENDING_K1_JOURNAL_ITEMS | PENDING_K1_JOURNAL_TOGGLE |
    PENDING_K1_JOURNAL_SORT | PENDING_K1_PAZAAK_SWITCH_GRID |
    PENDING_K1_PAZAAK_PLAY | PENDING_K1_PAZAAK_RETURN |
    PENDING_K1_PAZAAK_INITIAL_FOCUS |
    PENDING_K1_PAZAAK_GAME_LEFT | PENDING_K1_PAZAAK_GAME_RIGHT |
    PENDING_K1_PAZAAK_GAME_PLAY_CARD |
    PENDING_K1_PARTY_BUTTON_ROW | PENDING_K1_PARTY_GRID_RETURN |
    PENDING_K1_PAZAAK_GAME_FLIP_DOWN |
    PENDING_K1_PAZAAK_GAME_FLIP_UP |
    PENDING_K1_PAZAAK_GAME_FLIP_CARD |
    PENDING_K1_FEATS_SELECT | PENDING_K1_FEATS_ACCEPT |
    PENDING_K1_POWERS_SELECT;
constexpr std::uint32_t PENDING_K1_MENU_INPUT =
    PENDING_CONTROLLER_X | PENDING_K1_JOURNAL_ITEMS |
    PENDING_K1_JOURNAL_TOGGLE | PENDING_K1_JOURNAL_SORT |
    PENDING_K1_PAZAAK_SWITCH_GRID | PENDING_K1_PAZAAK_PLAY |
    PENDING_K1_PAZAAK_RETURN | PENDING_K1_PAZAAK_INITIAL_FOCUS |
    PENDING_K1_PAZAAK_GAME_LEFT | PENDING_K1_PAZAAK_GAME_RIGHT |
    PENDING_K1_PAZAAK_GAME_PLAY_CARD |
    PENDING_K1_PARTY_BUTTON_ROW | PENDING_K1_PARTY_GRID_RETURN |
    PENDING_K1_PAZAAK_GAME_FLIP_DOWN |
    PENDING_K1_PAZAAK_GAME_FLIP_UP |
    PENDING_K1_PAZAAK_GAME_FLIP_CARD |
    PENDING_K1_FEATS_SELECT | PENDING_K1_FEATS_ACCEPT |
    PENDING_K1_POWERS_SELECT;

constexpr int K2_GUI_CONTROLLER_X_EVENT = 0x29;
constexpr int K2_GUI_CONTROLLER_LEFT_EVENT = 0x2F;
constexpr int K2_GUI_CONTROLLER_RIGHT_EVENT = 0x30;
constexpr int K1_GUI_CONTROLLER_X_EVENT = 0x29;
constexpr int K1_JOURNAL_TOGGLE_EVENT = 0x2A;
constexpr int K1_JOURNAL_SORT_EVENT = 0x2B;
constexpr std::uintptr_t K1_ABILITIES_PANEL_VTABLE = 0x00755E50;
constexpr std::uintptr_t K1_CHARACTER_PANEL_VTABLE = 0x00756100;
constexpr std::uintptr_t K1_INVENTORY_PANEL_VTABLE = 0x007564E0;
constexpr std::uintptr_t K1_JOURNAL_PANEL_VTABLE = 0x00751960;
constexpr std::uintptr_t K1_MAP_PANEL_VTABLE = 0x00754830;
constexpr std::uintptr_t K1_MESSAGES_PANEL_VTABLE = 0x0074FD18;
constexpr std::uintptr_t K1_LEVEL_UP_PANEL_VTABLE = 0x00759568;
constexpr std::uintptr_t K1_POWERS_PANEL_VTABLE = 0x00759780;
constexpr std::uintptr_t K1_FEATS_PANEL_VTABLE = 0x007598B0;
constexpr std::uintptr_t K1_PAZAAK_SETUP_PANEL_VTABLE = 0x007532E8;
constexpr std::uintptr_t K1_PAZAAK_GAME_PANEL_VTABLE = 0x00753358;
constexpr std::uintptr_t K1_PARTY_SELECT_PANEL_VTABLE = 0x00756D28;
constexpr std::ptrdiff_t K1_PAZAAK_AVAILABLE_OFFSET = 0x01A4;
constexpr int K1_PAZAAK_AVAILABLE_COUNT = 18;
constexpr int K1_PAZAAK_AVAILABLE_ROWS = 6;
constexpr std::ptrdiff_t K1_PAZAAK_CHOSEN_OFFSET = 0x501C;
constexpr int K1_PAZAAK_CHOSEN_COUNT = 10;
constexpr int K1_PAZAAK_CHOSEN_COLUMNS = 2;
constexpr std::ptrdiff_t K1_PAZAAK_PLAY_OFFSET = 0x7108;
constexpr std::ptrdiff_t K1_PAZAAK_CARD_STRIDE = 0x031C;
constexpr std::ptrdiff_t K1_PAZAAK_HAND_OFFSET = 0x2DE0;
constexpr int K1_PAZAAK_HAND_COUNT = 4;
constexpr std::ptrdiff_t K1_PAZAAK_FLIP_OFFSET = 0x62BC;
constexpr std::ptrdiff_t K1_PAZAAK_FLIP_STRIDE = 0x01C4;
constexpr std::ptrdiff_t K1_PAZAAK_END_TURN_OFFSET = 0x6C4C;
constexpr std::uintptr_t K1_PAZAAK_PLAY_CARD_CALLBACK = 0x0067EF80;
constexpr std::uintptr_t K1_PAZAAK_FLIP_CARD_CALLBACK = 0x0067DDA0;
constexpr std::ptrdiff_t K1_PARTY_GRID_OFFSET = 0x007C;
constexpr std::ptrdiff_t K1_PARTY_GRID_STRIDE = 0x0454;
constexpr int K1_PARTY_GRID_COUNT = 9;
constexpr int K1_PARTY_BOTTOM_ROW_START = 6;
constexpr std::ptrdiff_t K1_PARTY_DONE_OFFSET = 0x28B0;
constexpr std::ptrdiff_t K1_PARTY_BACK_OFFSET = 0x36F4;
constexpr std::ptrdiff_t K1_FEATS_ACCEPT_OFFSET = 0x0CEC;
constexpr std::ptrdiff_t K1_FEATS_SELECT_OFFSET = 0x1238;
constexpr std::uintptr_t K1_FEATS_ACCEPT_CALLBACK = 0x00624BC0;
constexpr std::uintptr_t K1_FEATS_SELECT_CALLBACK = 0x00624BA0;
constexpr std::ptrdiff_t K1_POWERS_SELECT_OFFSET = 0x1470;
constexpr std::uintptr_t K1_POWERS_SELECT_CALLBACK = 0x00624BC0;
constexpr std::uintptr_t K2_INVENTORY_PANEL_VTABLE = 0x00992994;
constexpr std::uintptr_t K2_JOURNAL_PANEL_VTABLE = 0x00992274;
constexpr std::uintptr_t K1_MOVIE_PLAYER_POINTER = 0x007A3CF4;
constexpr std::uintptr_t K1_CANCEL_MOVIE = 0x00404C40;
constexpr std::uint32_t WM_KEYDOWN_MESSAGE = 0x0100;
constexpr std::uint32_t VK_ESCAPE_KEY = 0x1B;
constexpr std::uint32_t VK_DELETE_KEY = 0x2E;

struct BufferedInputRecord {
    std::uint32_t offset;
    std::uint32_t value;
};

struct GameConfig {
    std::ptrdiff_t panelManagerOffset;
    std::ptrdiff_t panelActiveControlOffset;
    std::ptrdiff_t panelFlagsOffset;
    std::ptrdiff_t targetActionMenuOffset;
    std::ptrdiff_t personalActionsOffset;
    std::ptrdiff_t actionGroupSize;
    std::array<std::ptrdiff_t, 4> actionControlOffsets;
    int personalActionCount;
    std::uintptr_t inGameMessagePanelVtable;
    std::uintptr_t inGameFadePanelVtable;
    std::uintptr_t inGamePausePanelVtable;
    std::uintptr_t keyboardDeviceIndex;
    std::uintptr_t getIsSelectable;
    std::uintptr_t setActiveControl;
    std::uintptr_t getControlAt;
    std::uintptr_t personalPrevious;
    std::uintptr_t personalNext;
    std::uintptr_t targetPrevious;
    std::uintptr_t targetNext;
    std::uintptr_t activate;
    std::uintptr_t cancelLastAction;
    std::uintptr_t handleGuiInputEvent;
};

constexpr std::ptrdiff_t MANAGER_PANEL_LIST_OFFSET = 0x0088;
constexpr std::ptrdiff_t MANAGER_PANEL_COUNT_OFFSET = 0x008C;
constexpr std::ptrdiff_t MANAGER_MODAL_LIST_OFFSET = 0x0094;
constexpr std::ptrdiff_t MANAGER_MODAL_COUNT_OFFSET = 0x0098;
constexpr std::ptrdiff_t TARGET_ACTIONS_OFFSET = 0x0054;
constexpr std::ptrdiff_t TARGET_ACTION_LIST_COUNT_OFFSET = 0x0004;
constexpr std::ptrdiff_t TARGET_ACTION_LIST_SIZE = 0x000C;
constexpr int TARGET_ACTION_COUNT = 3;

constexpr GameConfig K1_CONFIG = {
    0x18, 0x1C, 0x44, 0xBC, 0x772C, 0x71C,
    {0x000, 0x1C4, 0x388, 0x54C},
    4,
    0x0074FC60,
    0,
    0,
    0x0074D3C8,
    0x004189D0,
    0x0040A630,
    0x0040ABE0,
    0x0068AF70,
    0x0068AFE0,
    0x006884B0,
    0x00688520,
    0x0068B970,
    0x00688790,
    0x0040C8E0,
};

constexpr GameConfig K2_CONFIG = {
    0x1C, 0x20, 0x48, 0xCC, 0x733C, 0x750,
    {0x000, 0x1D0, 0x3A0, 0x570},
    6,
    0x0098E1EC,
    0x0099313C,
    0x00993334,
    0x00997514,
    0x00917340,
    0x0090D080,
    0x0051CB20,
    0x005233B0,
    0x00523460,
    0x00523510,
    0x005235C0,
    0x00522E30,
    0x00523AE0,
    0x0090EF20,
};

using ActionButtons = std::array<void*, MAX_ACTION_BUTTON_COUNT>;
using GetIsSelectableFn = bool(__thiscall*)(void*);
using SetActiveControlFn = void(__thiscall*)(void*, void*, int);
using K1GetControlAtFn = int(__thiscall*)(
    void*, int, int, void**, void**, int);
using K2GetControlAtFn = void*(__thiscall*)(void*, int, int);
using ActionCallbackFn = void(__thiscall*)(void*, void*);
using CancelLastActionFn = void(__thiscall*)(void*);
using CancelMovieFn = void(__thiscall*)(void*, int, int);
using HandleInputEventFn = void(__thiscall*)(void*, int, int);
using K1PazaakPlayCardFn = void(__thiscall*)(void*, void*);
using K1PazaakFlipCardFn = void(__thiscall*)(void*, void*);

std::uint32_t g_pendingInput = 0;
void* g_mainInterface = nullptr;
void* g_k1PazaakReturnControl = nullptr;
bool g_k1SuppressPazaakFlipEnterRelease = false;

void* OffsetPointer(void* base, std::ptrdiff_t offset)
{
    return reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(base) + offset);
}

void* ReadPointer(void* base, std::ptrdiff_t offset)
{
    return *reinterpret_cast<void**>(OffsetPointer(base, offset));
}

int ReadInt(void* base, std::ptrdiff_t offset)
{
    return *reinterpret_cast<int*>(OffsetPointer(base, offset));
}

int ActionButtonCount(const GameConfig& config)
{
    return TARGET_ACTION_COUNT + config.personalActionCount;
}

bool IsIgnoredByInput(const GameConfig& config, void* panel)
{
    return (ReadInt(panel, config.panelFlagsOffset) & PANEL_IGNORED_BY_INPUT) != 0;
}

bool IsGameplayHudActive(
    const GameConfig& config,
    void* manager,
    void* mainInterface)
{
    void** modalPanels = static_cast<void**>(
        ReadPointer(manager, MANAGER_MODAL_LIST_OFFSET));
    int modalCount = ReadInt(manager, MANAGER_MODAL_COUNT_OFFSET);
    for (int i = modalCount - 1; i >= 0; --i) {
        if (!IsIgnoredByInput(config, modalPanels[i])) {
            return false;
        }
    }

    void** panels = static_cast<void**>(
        ReadPointer(manager, MANAGER_PANEL_LIST_OFFSET));
    int panelCount = ReadInt(manager, MANAGER_PANEL_COUNT_OFFSET);
    for (int i = panelCount - 1; i >= 0; --i) {
        void* panel = panels[i];
        if (IsIgnoredByInput(config, panel)) {
            continue;
        }
        if (panel == mainInterface) {
            return true;
        }
        std::uintptr_t vtable = *reinterpret_cast<std::uintptr_t*>(panel);
        if (vtable != config.inGameMessagePanelVtable &&
            vtable != config.inGameFadePanelVtable &&
            vtable != config.inGamePausePanelVtable) {
            return false;
        }
    }
    return false;
}

bool IsK2FilterPanel(void* panel)
{
    std::uintptr_t vtable = *reinterpret_cast<std::uintptr_t*>(panel);
    return vtable == K2_INVENTORY_PANEL_VTABLE ||
        vtable == K2_JOURNAL_PANEL_VTABLE ||
        vtable == K2_CONFIG.inGameMessagePanelVtable;
}

void* FindK2FilterPanel(void* manager)
{
    void** modalPanels = static_cast<void**>(
        ReadPointer(manager, MANAGER_MODAL_LIST_OFFSET));
    int modalCount = ReadInt(manager, MANAGER_MODAL_COUNT_OFFSET);
    for (int i = modalCount - 1; i >= 0; --i) {
        if (!IsIgnoredByInput(K2_CONFIG, modalPanels[i]) &&
            IsK2FilterPanel(modalPanels[i])) {
            return modalPanels[i];
        }
    }

    void** panels = static_cast<void**>(
        ReadPointer(manager, MANAGER_PANEL_LIST_OFFSET));
    int panelCount = ReadInt(manager, MANAGER_PANEL_COUNT_OFFSET);
    for (int i = panelCount - 1; i >= 0; --i) {
        if (!IsIgnoredByInput(K2_CONFIG, panels[i]) &&
            IsK2FilterPanel(panels[i])) {
            return panels[i];
        }
    }
    return nullptr;
}

bool IsK1MenuPanel(void* panel)
{
    std::uintptr_t vtable = *reinterpret_cast<std::uintptr_t*>(panel);
    return vtable == K1_ABILITIES_PANEL_VTABLE ||
        vtable == K1_CHARACTER_PANEL_VTABLE ||
        vtable == K1_INVENTORY_PANEL_VTABLE ||
        vtable == K1_JOURNAL_PANEL_VTABLE ||
        vtable == K1_MAP_PANEL_VTABLE ||
        vtable == K1_MESSAGES_PANEL_VTABLE ||
        vtable == K1_LEVEL_UP_PANEL_VTABLE ||
        vtable == K1_POWERS_PANEL_VTABLE ||
        vtable == K1_FEATS_PANEL_VTABLE ||
        vtable == K1_PAZAAK_SETUP_PANEL_VTABLE ||
        vtable == K1_PAZAAK_GAME_PANEL_VTABLE ||
        vtable == K1_PARTY_SELECT_PANEL_VTABLE;
}

void* FindK1MenuPanel(void* manager)
{
    void** modalPanels = static_cast<void**>(
        ReadPointer(manager, MANAGER_MODAL_LIST_OFFSET));
    int modalCount = ReadInt(manager, MANAGER_MODAL_COUNT_OFFSET);
    for (int i = modalCount - 1; i >= 0; --i) {
        if (IsIgnoredByInput(K1_CONFIG, modalPanels[i])) {
            continue;
        }
        return IsK1MenuPanel(modalPanels[i])
            ? modalPanels[i]
            : nullptr;
    }

    void** panels = static_cast<void**>(
        ReadPointer(manager, MANAGER_PANEL_LIST_OFFSET));
    int panelCount = ReadInt(manager, MANAGER_PANEL_COUNT_OFFSET);
    for (int i = panelCount - 1; i >= 0; --i) {
        if (!IsIgnoredByInput(K1_CONFIG, panels[i]) &&
            IsK1MenuPanel(panels[i])) {
            return panels[i];
        }
    }
    return nullptr;
}

bool HasK1BlockingModal(void* manager, void* panel)
{
    void** modalPanels = static_cast<void**>(
        ReadPointer(manager, MANAGER_MODAL_LIST_OFFSET));
    int modalCount = ReadInt(manager, MANAGER_MODAL_COUNT_OFFSET);
    for (int i = modalCount - 1; i >= 0; --i) {
        if (IsIgnoredByInput(K1_CONFIG, modalPanels[i])) {
            continue;
        }
        return modalPanels[i] != panel;
    }
    return false;
}

int FindK1PazaakControlIndex(
    void* panel,
    void* control,
    std::ptrdiff_t offset,
    int count)
{
    std::uintptr_t first =
        reinterpret_cast<std::uintptr_t>(OffsetPointer(panel, offset));
    std::uintptr_t current = reinterpret_cast<std::uintptr_t>(control);
    if (current < first) {
        return -1;
    }

    std::uintptr_t delta = current - first;
    if (delta % K1_PAZAAK_CARD_STRIDE != 0) {
        return -1;
    }

    int index = static_cast<int>(delta / K1_PAZAAK_CARD_STRIDE);
    return index < count ? index : -1;
}

bool IsK1PazaakGridControl(void* panel, void* control)
{
    return FindK1PazaakControlIndex(
               panel,
               control,
               K1_PAZAAK_AVAILABLE_OFFSET,
               K1_PAZAAK_AVAILABLE_COUNT) >= 0 ||
        FindK1PazaakControlIndex(
               panel,
               control,
               K1_PAZAAK_CHOSEN_OFFSET,
               K1_PAZAAK_CHOSEN_COUNT) >= 0;
}

int FindK1PazaakHandIndex(void* panel, void* control)
{
    return FindK1PazaakControlIndex(
        panel,
        control,
        K1_PAZAAK_HAND_OFFSET,
        K1_PAZAAK_HAND_COUNT);
}

int FindK1PazaakFlipIndex(void* panel, void* control)
{
    std::uintptr_t first = reinterpret_cast<std::uintptr_t>(
        OffsetPointer(panel, K1_PAZAAK_FLIP_OFFSET));
    std::uintptr_t current = reinterpret_cast<std::uintptr_t>(control);
    if (current < first) {
        return -1;
    }

    std::uintptr_t delta = current - first;
    if (delta % K1_PAZAAK_FLIP_STRIDE != 0) {
        return -1;
    }

    int index = static_cast<int>(delta / K1_PAZAAK_FLIP_STRIDE);
    return index < K1_PAZAAK_HAND_COUNT ? index : -1;
}

int FindK1PartyGridIndex(void* panel, void* control)
{
    std::uintptr_t first = reinterpret_cast<std::uintptr_t>(
        OffsetPointer(panel, K1_PARTY_GRID_OFFSET));
    std::uintptr_t current = reinterpret_cast<std::uintptr_t>(control);
    if (current < first) {
        return -1;
    }

    std::uintptr_t delta = current - first;
    if (delta % K1_PARTY_GRID_STRIDE != 0) {
        return -1;
    }

    int index = static_cast<int>(delta / K1_PARTY_GRID_STRIDE);
    return index < K1_PARTY_GRID_COUNT ? index : -1;
}

bool IsK1SelectableControl(void* control)
{
    return control &&
        (ReadInt(control, K1_CONFIG.panelFlagsOffset) & CONTROL_VISIBLE) != 0 &&
        reinterpret_cast<GetIsSelectableFn>(
            K1_CONFIG.getIsSelectable)(control);
}

void DispatchPanelInput(void* panel, int event)
{
    std::uintptr_t vtable = *reinterpret_cast<std::uintptr_t*>(panel);
    std::uintptr_t callback =
        *reinterpret_cast<std::uintptr_t*>(vtable + 0x3C);
    reinterpret_cast<HandleInputEventFn>(callback)(panel, event, 1);
}

ActionButtons GetActionButtons(const GameConfig& config, void* mainInterface)
{
    ActionButtons buttons{};
    void* targetActions = OffsetPointer(
        mainInterface,
        config.targetActionMenuOffset + TARGET_ACTIONS_OFFSET);

    for (int i = 0; i < TARGET_ACTION_COUNT; ++i) {
        buttons[i] = OffsetPointer(targetActions, i * config.actionGroupSize);
    }

    void* personalActions = OffsetPointer(
        mainInterface,
        config.personalActionsOffset);
    for (int i = 0; i < config.personalActionCount; ++i) {
        buttons[TARGET_ACTION_COUNT + i] =
            OffsetPointer(personalActions, i * config.actionGroupSize);
    }

    return buttons;
}

int FindButton(
    const GameConfig& config,
    const ActionButtons& buttons,
    void* button)
{
    for (int i = 0; i < ActionButtonCount(config); ++i) {
        if (buttons[i] == button) {
            return i;
        }
    }
    return -1;
}

bool IsActionControl(
    const GameConfig& config,
    const ActionButtons& buttons,
    void* control)
{
    for (int i = 0; i < ActionButtonCount(config); ++i) {
        for (std::ptrdiff_t offset : config.actionControlOffsets) {
            if (OffsetPointer(buttons[i], offset) == control) {
                return true;
            }
        }
    }
    return false;
}

int FindSelectableButton(
    const GameConfig& config,
    void* mainInterface,
    const ActionButtons& buttons,
    int start,
    int direction)
{
    const int buttonCount = ActionButtonCount(config);
    const auto getIsSelectable =
        reinterpret_cast<GetIsSelectableFn>(config.getIsSelectable);
    int index = start;
    for (int count = 0; count < buttonCount; ++count) {
        index = (index + direction + buttonCount) % buttonCount;
        if (index < TARGET_ACTION_COUNT) {
            void* targetMenu = OffsetPointer(
                mainInterface,
                config.targetActionMenuOffset);
            if (ReadInt(
                    targetMenu,
                    index * TARGET_ACTION_LIST_SIZE +
                        TARGET_ACTION_LIST_COUNT_OFFSET) <= 0) {
                continue;
            }
        }
        if ((ReadInt(buttons[index], config.panelFlagsOffset) & CONTROL_VISIBLE) != 0 &&
            getIsSelectable(buttons[index])) {
            return index;
        }
    }
    return -1;
}

void MoveFocus(
    const GameConfig& config,
    void* mainInterface,
    const ActionButtons& buttons,
    int activeIndex,
    int direction)
{
    const int buttonCount = ActionButtonCount(config);
    int start = activeIndex;
    if (start < 0) {
        start = direction > 0 ? buttonCount - 1 : 0;
    }

    int nextIndex = FindSelectableButton(
        config,
        mainInterface,
        buttons,
        start,
        direction);
    if (nextIndex >= 0) {
        reinterpret_cast<SetActiveControlFn>(config.setActiveControl)(
            mainInterface,
            buttons[nextIndex],
            1);
    }
}

void CycleAction(
    const GameConfig& config,
    void* mainInterface,
    const ActionButtons& buttons,
    int activeIndex,
    bool next)
{
    if (activeIndex < 0) {
        return;
    }

    std::uintptr_t callback;
    if (activeIndex < TARGET_ACTION_COUNT) {
        callback = next ? config.targetNext : config.targetPrevious;
    } else {
        callback = next ? config.personalNext : config.personalPrevious;
    }

    reinterpret_cast<ActionCallbackFn>(callback)(
        mainInterface,
        buttons[activeIndex]);
}

std::uint32_t InputBit(std::uint32_t keyOffset)
{
    switch (keyOffset) {
    case DIK_UP:
        return PENDING_UP;
    case DIK_DOWN:
        return PENDING_DOWN;
    case DIK_LEFT:
        return PENDING_LEFT;
    case DIK_RIGHT:
        return PENDING_RIGHT;
    case DIK_RETURN:
        return PENDING_ACTIVATE;
    default:
        return 0;
    }
}

void CaptureActionBarInput(
    const GameConfig& config,
    const BufferedInputRecord* input,
    int inputDevice)
{
    if (!input || inputDevice != *reinterpret_cast<int*>(config.keyboardDeviceIndex)) {
        return;
    }

    if ((input->value & KEY_PRESSED) != 0 &&
        input->offset == DIK_DELETE &&
        config.handleGuiInputEvent != 0) {
        g_pendingInput |= PENDING_CONTEXT_CANCEL;
        return;
    }

    void* mainInterface = g_mainInterface;
    void* manager = mainInterface
        ? ReadPointer(mainInterface, config.panelManagerOffset)
        : nullptr;
    if (!manager || !IsGameplayHudActive(config, manager, mainInterface)) {
        g_pendingInput &= PENDING_CONTEXT_INPUT;
        return;
    }

    if ((input->value & KEY_PRESSED) != 0) {
        g_pendingInput |= InputBit(input->offset);
    }
}

void UpdateActionBarControls(const GameConfig& config, void* mainInterface)
{
    g_mainInterface = mainInterface;
    std::uint32_t pending = g_pendingInput & PENDING_ACTION_BAR_INPUT;
    g_pendingInput &= PENDING_CONTEXT_INPUT;

    if (!mainInterface || pending == 0) {
        return;
    }

    void* manager = ReadPointer(mainInterface, config.panelManagerOffset);
    if (!manager || !IsGameplayHudActive(config, manager, mainInterface)) {
        return;
    }

    ActionButtons buttons = GetActionButtons(config, mainInterface);
    void* activeControl = ReadPointer(
        mainInterface,
        config.panelActiveControlOffset);
    int activeIndex = FindButton(config, buttons, activeControl);

    if ((pending & PENDING_LEFT) != 0) {
        MoveFocus(config, mainInterface, buttons, activeIndex, -1);
    }
    if ((pending & PENDING_RIGHT) != 0) {
        MoveFocus(config, mainInterface, buttons, activeIndex, 1);
    }
    if ((pending & PENDING_UP) != 0) {
        CycleAction(config, mainInterface, buttons, activeIndex, false);
    }
    if ((pending & PENDING_DOWN) != 0) {
        CycleAction(config, mainInterface, buttons, activeIndex, true);
    }
    if ((pending & PENDING_ACTIVATE) != 0 && activeIndex >= 0) {
        reinterpret_cast<ActionCallbackFn>(config.activate)(
            mainInterface,
            buttons[activeIndex]);
    }
}

void ClearActionBarKeyboardFocus(
    const GameConfig& config,
    void* hoveredPanel,
    void* hoveredControl)
{
    void* mainInterface = g_mainInterface;
    if (!mainInterface || hoveredPanel != mainInterface) {
        return;
    }

    ActionButtons buttons = GetActionButtons(config, mainInterface);
    if (!IsActionControl(config, buttons, hoveredControl)) {
        return;
    }

    void* activeControl = ReadPointer(
        mainInterface,
        config.panelActiveControlOffset);
    if (FindButton(config, buttons, activeControl) >= 0) {
        reinterpret_cast<SetActiveControlFn>(config.setActiveControl)(
            mainInterface,
            nullptr,
            1);
    }
}

} // namespace

extern "C" void __cdecl CaptureActionBarInputK1(
    BufferedInputRecord* input,
    int inputDevice)
{
    if (input &&
        inputDevice == *reinterpret_cast<int*>(K1_CONFIG.keyboardDeviceIndex) &&
        input->offset == DIK_DELETE) {
        void* mainInterface = g_mainInterface;
        void* manager = mainInterface
            ? ReadPointer(mainInterface, K1_CONFIG.panelManagerOffset)
            : nullptr;
        if (!manager ||
            !IsGameplayHudActive(K1_CONFIG, manager, mainInterface)) {
            input->offset = DIK_ESCAPE;
            return;
        }
    }

    if (input &&
        inputDevice == *reinterpret_cast<int*>(K1_CONFIG.keyboardDeviceIndex) &&
        input->offset == DIK_RETURN &&
        (input->value & KEY_PRESSED) == 0 &&
        g_k1SuppressPazaakFlipEnterRelease) {
        input->offset = 0;
        g_k1SuppressPazaakFlipEnterRelease = false;
    }

    CaptureActionBarInput(K1_CONFIG, input, inputDevice);
    if (input &&
        inputDevice == *reinterpret_cast<int*>(K1_CONFIG.keyboardDeviceIndex) &&
        (input->value & KEY_PRESSED) != 0) {
        void* mainInterface = g_mainInterface;
        void* manager = mainInterface
            ? ReadPointer(mainInterface, K1_CONFIG.panelManagerOffset)
            : nullptr;
        void* panel = manager ? FindK1MenuPanel(manager) : nullptr;
        if (!panel) {
            return;
        }

        bool isJournal =
            *reinterpret_cast<std::uintptr_t*>(panel) ==
            K1_JOURNAL_PANEL_VTABLE;
        bool isPazaak =
            *reinterpret_cast<std::uintptr_t*>(panel) ==
            K1_PAZAAK_SETUP_PANEL_VTABLE;
        bool isPazaakGame =
            *reinterpret_cast<std::uintptr_t*>(panel) ==
            K1_PAZAAK_GAME_PANEL_VTABLE;
        bool isPartySelect =
            *reinterpret_cast<std::uintptr_t*>(panel) ==
            K1_PARTY_SELECT_PANEL_VTABLE;
        bool isFeats =
            *reinterpret_cast<std::uintptr_t*>(panel) ==
            K1_FEATS_PANEL_VTABLE;
        bool isPowers =
            *reinterpret_cast<std::uintptr_t*>(panel) ==
            K1_POWERS_PANEL_VTABLE;
        bool isLevelUp =
            *reinterpret_cast<std::uintptr_t*>(panel) ==
            K1_LEVEL_UP_PANEL_VTABLE;
        if (isLevelUp) {
            return;
        }
        if (isPowers) {
            if (HasK1BlockingModal(manager, panel)) {
                return;
            }
            if (input->offset == DIK_SPACE) {
                g_pendingInput |= PENDING_K1_POWERS_SELECT;
                input->offset = 0;
                input->value = 0;
            }
            return;
        }
        if (isFeats) {
            if (HasK1BlockingModal(manager, panel)) {
                return;
            }
            if (input->offset == DIK_SPACE) {
                g_pendingInput |= PENDING_K1_FEATS_SELECT;
                input->offset = 0;
                input->value = 0;
            } else if (input->offset == DIK_RETURN) {
                g_pendingInput |= PENDING_K1_FEATS_ACCEPT;
                input->offset = 0;
                input->value = 0;
            }
            return;
        }
        if (isPartySelect) {
            if (HasK1BlockingModal(manager, panel)) {
                return;
            }

            void* activeControl = ReadPointer(
                panel,
                K1_CONFIG.panelActiveControlOffset);
            int gridIndex =
                FindK1PartyGridIndex(panel, activeControl);
            std::uint32_t pending = 0;
            if (input->offset == DIK_DOWN &&
                gridIndex >= K1_PARTY_BOTTOM_ROW_START) {
                pending = PENDING_K1_PARTY_BUTTON_ROW;
            } else if (input->offset == DIK_UP &&
                       (activeControl == OffsetPointer(
                            panel,
                            K1_PARTY_DONE_OFFSET) ||
                        activeControl == OffsetPointer(
                            panel,
                            K1_PARTY_BACK_OFFSET))) {
                pending = PENDING_K1_PARTY_GRID_RETURN;
            }

            if (pending != 0) {
                g_pendingInput |= pending;
                input->value = 0;
            }
            return;
        }
        if (isPazaakGame) {
            if (HasK1BlockingModal(manager, panel)) {
                return;
            }

            void* activeControl = ReadPointer(
                panel,
                K1_CONFIG.panelActiveControlOffset);
            int handIndex =
                FindK1PazaakHandIndex(panel, activeControl);
            int flipIndex =
                FindK1PazaakFlipIndex(panel, activeControl);
            std::uint32_t pending = 0;
            if (input->offset == DIK_LEFT &&
                (handIndex >= 0 ||
                 activeControl == OffsetPointer(
                     panel,
                     K1_PAZAAK_END_TURN_OFFSET))) {
                pending = PENDING_K1_PAZAAK_GAME_LEFT;
            } else if (input->offset == DIK_RIGHT &&
                       handIndex >= 0) {
                pending = PENDING_K1_PAZAAK_GAME_RIGHT;
            } else if (input->offset == DIK_DOWN &&
                       handIndex >= 0 &&
                       IsK1SelectableControl(OffsetPointer(
                           panel,
                           K1_PAZAAK_FLIP_OFFSET +
                               handIndex * K1_PAZAAK_FLIP_STRIDE))) {
                pending = PENDING_K1_PAZAAK_GAME_FLIP_DOWN;
            } else if (input->offset == DIK_UP &&
                       flipIndex >= 0) {
                pending = PENDING_K1_PAZAAK_GAME_FLIP_UP;
            } else if (input->offset == DIK_RETURN &&
                       handIndex >= 0) {
                pending = PENDING_K1_PAZAAK_GAME_PLAY_CARD;
            } else if (input->offset == DIK_RETURN &&
                       flipIndex >= 0) {
                pending = PENDING_K1_PAZAAK_GAME_FLIP_CARD;
                g_k1SuppressPazaakFlipEnterRelease = true;
            }

            if (pending != 0) {
                g_pendingInput |= pending;
                input->value = 0;
                if (pending == PENDING_K1_PAZAAK_GAME_FLIP_CARD) {
                    input->offset = 0;
                }
            }
            return;
        }
        if (isPazaak) {
            if (input->offset != DIK_UP &&
                input->offset != DIK_DOWN &&
                input->offset != DIK_LEFT &&
                input->offset != DIK_RIGHT) {
                return;
            }
            if (HasK1BlockingModal(manager, panel)) {
                return;
            }

            void* activeControl = ReadPointer(
                panel,
                K1_CONFIG.panelActiveControlOffset);
            int availableIndex = FindK1PazaakControlIndex(
                panel,
                activeControl,
                K1_PAZAAK_AVAILABLE_OFFSET,
                K1_PAZAAK_AVAILABLE_COUNT);
            int chosenIndex = FindK1PazaakControlIndex(
                panel,
                activeControl,
                K1_PAZAAK_CHOSEN_OFFSET,
                K1_PAZAAK_CHOSEN_COUNT);
            void* playControl =
                OffsetPointer(panel, K1_PAZAAK_PLAY_OFFSET);
            std::uint32_t pending = 0;

            if (availableIndex < 0 &&
                chosenIndex < 0 &&
                activeControl != playControl) {
                pending = PENDING_K1_PAZAAK_INITIAL_FOCUS;
            } else if (input->offset == DIK_RIGHT &&
                       availableIndex / K1_PAZAAK_AVAILABLE_ROWS == 2) {
                pending = PENDING_K1_PAZAAK_SWITCH_GRID;
            } else if (input->offset == DIK_LEFT &&
                       chosenIndex >= 0 &&
                       chosenIndex % K1_PAZAAK_CHOSEN_COLUMNS == 0) {
                pending = PENDING_K1_PAZAAK_SWITCH_GRID;
            } else if (input->offset == DIK_DOWN &&
                       ((availableIndex >= 0 &&
                         availableIndex % K1_PAZAAK_AVAILABLE_ROWS ==
                             K1_PAZAAK_AVAILABLE_ROWS - 1) ||
                        chosenIndex >=
                            K1_PAZAAK_CHOSEN_COUNT -
                                K1_PAZAAK_CHOSEN_COLUMNS)) {
                g_k1PazaakReturnControl = activeControl;
                pending = PENDING_K1_PAZAAK_PLAY;
            } else if (input->offset == DIK_UP &&
                       activeControl == playControl) {
                pending = PENDING_K1_PAZAAK_RETURN;
            }

            if (pending != 0) {
                g_pendingInput |= pending;
                input->value = 0;
            }
            return;
        }

        if (input->offset == DIK_SPACE && isJournal) {
            g_pendingInput |= PENDING_K1_JOURNAL_TOGGLE;
        } else if (input->offset == DIK_SPACE) {
            g_pendingInput |= PENDING_CONTROLLER_X;
        } else if ((input->offset == DIK_LEFT ||
                    input->offset == DIK_RIGHT) &&
                   isJournal) {
            g_pendingInput |= PENDING_K1_JOURNAL_SORT;
        } else if (input->offset == DIK_RETURN && isJournal) {
            g_pendingInput |= PENDING_K1_JOURNAL_ITEMS;
        }
    }
}

extern "C" void __cdecl CaptureActionBarInputK2(
    BufferedInputRecord* input,
    const void* getEventsFrame)
{
    const int inputDevice = *reinterpret_cast<const int*>(
        static_cast<const std::uint8_t*>(getEventsFrame) - 0x10);
    if (input &&
        inputDevice == *reinterpret_cast<int*>(K2_CONFIG.keyboardDeviceIndex) &&
        input->offset == DIK_DELETE) {
        void* mainInterface = g_mainInterface;
        void* manager = mainInterface
            ? ReadPointer(mainInterface, K2_CONFIG.panelManagerOffset)
            : nullptr;
        if (!manager ||
            !IsGameplayHudActive(K2_CONFIG, manager, mainInterface)) {
            input->offset = DIK_ESCAPE;
            return;
        }
    }

    CaptureActionBarInput(K2_CONFIG, input, inputDevice);
    if (input &&
        inputDevice == *reinterpret_cast<int*>(K2_CONFIG.keyboardDeviceIndex) &&
        (input->value & KEY_PRESSED) != 0) {
        void* mainInterface = g_mainInterface;
        void* manager = mainInterface
            ? ReadPointer(mainInterface, K2_CONFIG.panelManagerOffset)
            : nullptr;
        if (!manager) {
            return;
        }
        if (input->offset == DIK_SPACE &&
            !IsGameplayHudActive(K2_CONFIG, manager, mainInterface)) {
            g_pendingInput |= PENDING_CONTROLLER_X;
        } else if (!IsGameplayHudActive(
                       K2_CONFIG,
                       manager,
                       mainInterface) &&
                   FindK2FilterPanel(manager)) {
            if (input->offset == DIK_LEFT) {
                g_pendingInput |= PENDING_CONTROLLER_LEFT;
            } else if (input->offset == DIK_RIGHT) {
                g_pendingInput |= PENDING_CONTROLLER_RIGHT;
            }
        }
    }
}

extern "C" void __cdecl UpdateActionBarControlsK1(void* mainInterface)
{
    UpdateActionBarControls(K1_CONFIG, mainInterface);

    if ((g_pendingInput & PENDING_CONTEXT_CANCEL) == 0) {
        return;
    }

    g_pendingInput &= ~PENDING_CONTEXT_CANCEL;
    void* manager = mainInterface
        ? ReadPointer(mainInterface, K1_CONFIG.panelManagerOffset)
        : nullptr;
    if (manager &&
        IsGameplayHudActive(K1_CONFIG, manager, mainInterface)) {
        reinterpret_cast<CancelLastActionFn>(K1_CONFIG.cancelLastAction)(
            mainInterface);
    }
}

extern "C" void __cdecl UpdateActionBarControlsK2(void* mainInterface)
{
    UpdateActionBarControls(K2_CONFIG, mainInterface);
}

extern "C" void __cdecl MapMovieDeleteToEscapeK2(void* movieWindowFrame)
{
    if (!movieWindowFrame) {
        return;
    }

    std::uint8_t* frame = static_cast<std::uint8_t*>(movieWindowFrame);
    std::uint32_t* message = reinterpret_cast<std::uint32_t*>(frame + 0x0C);
    std::uint32_t* key = reinterpret_cast<std::uint32_t*>(frame + 0x10);
    if (*message == WM_KEYDOWN_MESSAGE && *key == VK_DELETE_KEY) {
        *key = VK_ESCAPE_KEY;
    }
}

extern "C" void __cdecl CancelMovieOnDeleteK1(
    std::uint32_t message,
    std::uint32_t key)
{
    if (message != WM_KEYDOWN_MESSAGE || key != VK_DELETE_KEY) {
        return;
    }

    void* moviePlayer =
        *reinterpret_cast<void**>(K1_MOVIE_PLAYER_POINTER);
    if (moviePlayer) {
        reinterpret_cast<CancelMovieFn>(K1_CANCEL_MOVIE)(
            moviePlayer,
            0,
            0);
    }
}

extern "C" void __cdecl DispatchMenuInputK1(void* clientApp)
{
    if (!clientApp ||
        (g_pendingInput & PENDING_K1_MENU_INPUT) == 0) {
        return;
    }

    std::uint32_t pending = g_pendingInput & PENDING_K1_MENU_INPUT;
    g_pendingInput &= ~PENDING_K1_MENU_INPUT;
    void* mainInterface = g_mainInterface;
    void* manager = mainInterface
        ? ReadPointer(mainInterface, K1_CONFIG.panelManagerOffset)
        : nullptr;
    void* panel = manager
        ? FindK1MenuPanel(manager)
        : nullptr;
    if (!panel) {
        return;
    }

    bool isJournal =
        *reinterpret_cast<std::uintptr_t*>(panel) ==
        K1_JOURNAL_PANEL_VTABLE;
    bool isPazaak =
        *reinterpret_cast<std::uintptr_t*>(panel) ==
        K1_PAZAAK_SETUP_PANEL_VTABLE;
    bool isPazaakGame =
        *reinterpret_cast<std::uintptr_t*>(panel) ==
        K1_PAZAAK_GAME_PANEL_VTABLE;
    bool isPartySelect =
        *reinterpret_cast<std::uintptr_t*>(panel) ==
        K1_PARTY_SELECT_PANEL_VTABLE;
    bool isFeats =
        *reinterpret_cast<std::uintptr_t*>(panel) ==
        K1_FEATS_PANEL_VTABLE;
    bool isPowers =
        *reinterpret_cast<std::uintptr_t*>(panel) ==
        K1_POWERS_PANEL_VTABLE;
    if (isPowers) {
        if ((pending & PENDING_K1_POWERS_SELECT) != 0) {
            reinterpret_cast<ActionCallbackFn>(
                K1_POWERS_SELECT_CALLBACK)(
                    panel,
                    OffsetPointer(panel, K1_POWERS_SELECT_OFFSET));
        }
        return;
    }
    if (isFeats) {
        if ((pending & PENDING_K1_FEATS_SELECT) != 0) {
            reinterpret_cast<ActionCallbackFn>(
                K1_FEATS_SELECT_CALLBACK)(
                    panel,
                    OffsetPointer(panel, K1_FEATS_SELECT_OFFSET));
        }
        if ((pending & PENDING_K1_FEATS_ACCEPT) != 0) {
            reinterpret_cast<ActionCallbackFn>(
                K1_FEATS_ACCEPT_CALLBACK)(
                    panel,
                    OffsetPointer(panel, K1_FEATS_ACCEPT_OFFSET));
        }
        return;
    }
    if (isPartySelect) {
        void* target = nullptr;
        if ((pending & PENDING_K1_PARTY_BUTTON_ROW) != 0) {
            target = OffsetPointer(
                panel,
                K1_PARTY_DONE_OFFSET);
        } else if ((pending & PENDING_K1_PARTY_GRID_RETURN) != 0) {
            target = OffsetPointer(
                panel,
                K1_PARTY_GRID_OFFSET +
                    (K1_PARTY_GRID_COUNT - 1) *
                        K1_PARTY_GRID_STRIDE);
        }
        if (target) {
            reinterpret_cast<SetActiveControlFn>(
                K1_CONFIG.setActiveControl)(panel, target, 1);
        }
        return;
    }
    if (isPazaakGame) {
        void* activeControl = ReadPointer(
            panel,
            K1_CONFIG.panelActiveControlOffset);
        int handIndex =
            FindK1PazaakHandIndex(panel, activeControl);
        int flipIndex =
            FindK1PazaakFlipIndex(panel, activeControl);
        void* target = nullptr;

        if ((pending & PENDING_K1_PAZAAK_GAME_FLIP_DOWN) != 0 &&
            handIndex >= 0) {
            target = OffsetPointer(
                panel,
                K1_PAZAAK_FLIP_OFFSET +
                    handIndex * K1_PAZAAK_FLIP_STRIDE);
        } else if ((pending & PENDING_K1_PAZAAK_GAME_FLIP_UP) != 0 &&
                   flipIndex >= 0) {
            target = OffsetPointer(
                panel,
                K1_PAZAAK_HAND_OFFSET +
                    flipIndex * K1_PAZAAK_CARD_STRIDE);
        } else if ((pending & PENDING_K1_PAZAAK_GAME_LEFT) != 0) {
            int startIndex = handIndex >= 0
                ? handIndex - 1
                : K1_PAZAAK_HAND_COUNT - 1;
            for (int i = startIndex; i >= 0; --i) {
                void* card = OffsetPointer(
                    panel,
                    K1_PAZAAK_HAND_OFFSET +
                        i * K1_PAZAAK_CARD_STRIDE);
                if (IsK1SelectableControl(card)) {
                    target = card;
                    break;
                }
            }
        } else if ((pending & PENDING_K1_PAZAAK_GAME_RIGHT) != 0) {
            for (int i = handIndex + 1;
                 i < K1_PAZAAK_HAND_COUNT;
                 ++i) {
                void* card = OffsetPointer(
                    panel,
                    K1_PAZAAK_HAND_OFFSET +
                        i * K1_PAZAAK_CARD_STRIDE);
                if (IsK1SelectableControl(card)) {
                    target = card;
                    break;
                }
            }
            if (!target) {
                void* endTurn = OffsetPointer(
                    panel,
                    K1_PAZAAK_END_TURN_OFFSET);
                if (IsK1SelectableControl(endTurn)) {
                    target = endTurn;
                }
            }
        }

        if (target) {
            reinterpret_cast<SetActiveControlFn>(
                K1_CONFIG.setActiveControl)(panel, target, 1);
        }
        if ((pending & PENDING_K1_PAZAAK_GAME_PLAY_CARD) != 0 &&
            handIndex >= 0 &&
            IsK1SelectableControl(activeControl)) {
            reinterpret_cast<K1PazaakPlayCardFn>(
                K1_PAZAAK_PLAY_CARD_CALLBACK)(
                    panel,
                    activeControl);
        }
        if ((pending & PENDING_K1_PAZAAK_GAME_FLIP_CARD) != 0 &&
            flipIndex >= 0 &&
            IsK1SelectableControl(activeControl)) {
            reinterpret_cast<K1PazaakFlipCardFn>(
                K1_PAZAAK_FLIP_CARD_CALLBACK)(
                    panel,
                    activeControl);
        }
        return;
    }
    if (isPazaak) {
        if ((pending & PENDING_K1_PAZAAK_SWITCH_GRID) != 0) {
            void* activeControl = ReadPointer(
                panel,
                K1_CONFIG.panelActiveControlOffset);
            int availableIndex = FindK1PazaakControlIndex(
                panel,
                activeControl,
                K1_PAZAAK_AVAILABLE_OFFSET,
                K1_PAZAAK_AVAILABLE_COUNT);
            int chosenIndex = FindK1PazaakControlIndex(
                panel,
                activeControl,
                K1_PAZAAK_CHOSEN_OFFSET,
                K1_PAZAAK_CHOSEN_COUNT);
            void* target = nullptr;
            if (availableIndex >= 0) {
                int row = availableIndex % K1_PAZAAK_AVAILABLE_ROWS;
                int chosenRows =
                    K1_PAZAAK_CHOSEN_COUNT /
                    K1_PAZAAK_CHOSEN_COLUMNS;
                int targetIndex =
                    (row < chosenRows ? row : chosenRows - 1) *
                    K1_PAZAAK_CHOSEN_COLUMNS;
                target = OffsetPointer(
                    panel,
                    K1_PAZAAK_CHOSEN_OFFSET +
                        targetIndex * K1_PAZAAK_CARD_STRIDE);
            } else if (chosenIndex >= 0) {
                int row = chosenIndex / K1_PAZAAK_CHOSEN_COLUMNS;
                int targetIndex =
                    2 * K1_PAZAAK_AVAILABLE_ROWS + row;
                target = OffsetPointer(
                    panel,
                    K1_PAZAAK_AVAILABLE_OFFSET +
                        targetIndex * K1_PAZAAK_CARD_STRIDE);
            }
            if (target) {
                reinterpret_cast<SetActiveControlFn>(
                    K1_CONFIG.setActiveControl)(panel, target, 1);
            }
        }
        if ((pending & PENDING_K1_PAZAAK_PLAY) != 0) {
            reinterpret_cast<SetActiveControlFn>(
                K1_CONFIG.setActiveControl)(
                panel,
                OffsetPointer(panel, K1_PAZAAK_PLAY_OFFSET),
                1);
        }
        if ((pending & PENDING_K1_PAZAAK_RETURN) != 0) {
            void* target = g_k1PazaakReturnControl;
            if (!IsK1PazaakGridControl(panel, target)) {
                target =
                    OffsetPointer(panel, K1_PAZAAK_AVAILABLE_OFFSET);
            }
            reinterpret_cast<SetActiveControlFn>(
                K1_CONFIG.setActiveControl)(panel, target, 1);
        }
        if ((pending & PENDING_K1_PAZAAK_INITIAL_FOCUS) != 0) {
            reinterpret_cast<SetActiveControlFn>(
                K1_CONFIG.setActiveControl)(
                panel,
                OffsetPointer(panel, K1_PAZAAK_AVAILABLE_OFFSET),
                1);
        }
        return;
    }

    if ((pending & PENDING_CONTROLLER_X) != 0 && !isJournal) {
        DispatchPanelInput(panel, K1_GUI_CONTROLLER_X_EVENT);
    }
    if ((pending & PENDING_K1_JOURNAL_ITEMS) != 0 && isJournal) {
        DispatchPanelInput(panel, K1_GUI_CONTROLLER_X_EVENT);
    }
    if ((pending & PENDING_K1_JOURNAL_TOGGLE) != 0 && isJournal) {
        DispatchPanelInput(panel, K1_JOURNAL_TOGGLE_EVENT);
    }
    if ((pending & PENDING_K1_JOURNAL_SORT) != 0 && isJournal) {
        DispatchPanelInput(panel, K1_JOURNAL_SORT_EVENT);
    }
}

extern "C" void __cdecl DispatchContextCancelK2(void* clientApp)
{
    if (!clientApp ||
        (g_pendingInput & PENDING_CONTEXT_INPUT) == 0) {
        return;
    }

    std::uint32_t pending = g_pendingInput & PENDING_CONTEXT_INPUT;
    g_pendingInput &= ~PENDING_CONTEXT_INPUT;
    void* mainInterface = g_mainInterface;
    void* manager = mainInterface
        ? ReadPointer(mainInterface, K2_CONFIG.panelManagerOffset)
        : nullptr;
    if ((pending & PENDING_CONTROLLER_X) != 0 && manager) {
        reinterpret_cast<HandleInputEventFn>(K2_CONFIG.handleGuiInputEvent)(
            manager,
            K2_GUI_CONTROLLER_X_EVENT,
            1);
    }
    void* filterPanel = manager ? FindK2FilterPanel(manager) : nullptr;
    if ((pending & PENDING_CONTROLLER_LEFT) != 0 && filterPanel) {
        DispatchPanelInput(filterPanel, K2_GUI_CONTROLLER_LEFT_EVENT);
    }
    if ((pending & PENDING_CONTROLLER_RIGHT) != 0 && filterPanel) {
        DispatchPanelInput(filterPanel, K2_GUI_CONTROLLER_RIGHT_EVENT);
    }
    if ((pending & PENDING_CONTEXT_CANCEL) != 0 &&
        manager &&
        IsGameplayHudActive(K2_CONFIG, manager, mainInterface)) {
        reinterpret_cast<CancelLastActionFn>(K2_CONFIG.cancelLastAction)(
            mainInterface);
    } else if ((pending & PENDING_CONTEXT_CANCEL) != 0 && manager) {
        reinterpret_cast<HandleInputEventFn>(K2_CONFIG.handleGuiInputEvent)(
            manager,
            DIK_ESCAPE,
            1);
    }
}

extern "C" void __cdecl ClearActionBarControlsK2(void* mainInterface)
{
    if (g_mainInterface == mainInterface) {
        g_mainInterface = nullptr;
        g_pendingInput = 0;
    }
}

extern "C" void __cdecl ClearActionBarControlsK1(void* mainInterface)
{
    if (g_mainInterface == mainInterface) {
        g_mainInterface = nullptr;
        g_pendingInput = 0;
        g_k1PazaakReturnControl = nullptr;
        g_k1SuppressPazaakFlipEnterRelease = false;
    }
}

extern "C" void __cdecl CancelActionBarKeyboardFocusOnMouseMoveK1(
    void* manager,
    int mouseX,
    int mouseY)
{
    void* mainInterface = g_mainInterface;
    if (!mainInterface ||
        ReadPointer(mainInterface, K1_CONFIG.panelManagerOffset) != manager) {
        return;
    }

    void* hoveredPanel = nullptr;
    void* hoveredControl = nullptr;
    reinterpret_cast<K1GetControlAtFn>(K1_CONFIG.getControlAt)(
        manager,
        mouseX,
        mouseY,
        &hoveredPanel,
        &hoveredControl,
        0);
    ClearActionBarKeyboardFocus(
        K1_CONFIG,
        hoveredPanel,
        hoveredControl);
}

extern "C" void __cdecl CancelActionBarKeyboardFocusOnMouseMoveK2(
    void* inputInternal)
{
    void* mainInterface = g_mainInterface;
    if (!mainInterface || !inputInternal) {
        return;
    }

    int mouseX = ReadInt(inputInternal, 0x3E4);
    int mouseY = ReadInt(mainInterface, 0x10) -
        ReadInt(inputInternal, 0x3E8);
    void* hoveredControl = reinterpret_cast<K2GetControlAtFn>(
        K2_CONFIG.getControlAt)(mainInterface, mouseX, mouseY);
    ClearActionBarKeyboardFocus(
        K2_CONFIG,
        mainInterface,
        hoveredControl);
}
