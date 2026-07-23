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
constexpr std::uint32_t KEY_PRESSED = 0x80;

constexpr std::uint32_t PENDING_UP = 1 << 0;
constexpr std::uint32_t PENDING_DOWN = 1 << 1;
constexpr std::uint32_t PENDING_LEFT = 1 << 2;
constexpr std::uint32_t PENDING_RIGHT = 1 << 3;
constexpr std::uint32_t PENDING_ACTIVATE = 1 << 4;

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
};

using ActionButtons = std::array<void*, MAX_ACTION_BUTTON_COUNT>;
using GetIsSelectableFn = bool(__thiscall*)(void*);
using SetActiveControlFn = void(__thiscall*)(void*, void*, int);
using K1GetControlAtFn = int(__thiscall*)(
    void*, int, int, void**, void**, int);
using K2GetControlAtFn = void*(__thiscall*)(void*, int, int);
using ActionCallbackFn = void(__thiscall*)(void*, void*);

std::uint32_t g_pendingInput = 0;
void* g_mainInterface = nullptr;

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

std::uint32_t DirectionBit(std::uint32_t keyOffset)
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

    void* mainInterface = g_mainInterface;
    void* manager = mainInterface
        ? ReadPointer(mainInterface, config.panelManagerOffset)
        : nullptr;
    if (!manager || !IsGameplayHudActive(config, manager, mainInterface)) {
        g_pendingInput = 0;
        return;
    }

    if ((input->value & KEY_PRESSED) != 0) {
        g_pendingInput |= DirectionBit(input->offset);
    }
}

void UpdateActionBarControls(const GameConfig& config, void* mainInterface)
{
    g_mainInterface = mainInterface;
    std::uint32_t pending = g_pendingInput;
    g_pendingInput = 0;

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
    const BufferedInputRecord* input,
    int inputDevice)
{
    CaptureActionBarInput(K1_CONFIG, input, inputDevice);
}

extern "C" void __cdecl CaptureActionBarInputK2(
    const BufferedInputRecord* input,
    const void* getEventsFrame)
{
    const int inputDevice = *reinterpret_cast<const int*>(
        static_cast<const std::uint8_t*>(getEventsFrame) - 0x10);
    CaptureActionBarInput(K2_CONFIG, input, inputDevice);
}

extern "C" void __cdecl UpdateActionBarControlsK1(void* mainInterface)
{
    UpdateActionBarControls(K1_CONFIG, mainInterface);
}

extern "C" void __cdecl UpdateActionBarControlsK2(void* mainInterface)
{
    UpdateActionBarControls(K2_CONFIG, mainInterface);
}

extern "C" void __cdecl ClearActionBarControlsK2(void* mainInterface)
{
    if (g_mainInterface == mainInterface) {
        g_mainInterface = nullptr;
        g_pendingInput = 0;
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
