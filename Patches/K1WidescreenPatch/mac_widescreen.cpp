#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <mach/mach.h>
#include <mach/vm_map.h>
#include <cmath>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <utility>
#include <cstdint>
#include <cstddef>
#include <climits>
#include <cstring>
#include <chrono>
#include <mutex>

/*
 ==============================================================================================
  STAR WARS: KNIGHTS OF THE OLD REPUBLIC 1 (MAC STEAM / ASPYR 64-BIT PORT)
  SCALED KOTOR WIDESCREEN PATCH
 ==============================================================================================
  Resolution ownership:
  CAurInternalGL::Reshape is the authoritative source of the committed framebuffer size.
  Width and Height in swkotor.ini remain the game's requested startup mode; this module no
  longer treats a live INI edit as though the renderer had already changed modes.
 ==============================================================================================
*/
int g_targetWidth  = 800;
int g_targetHeight = 600;

// Native template dimensions used internally by the Mac 4:3 GUI layout (mipc212x9).
int g_refWidth     = 1280;
int g_refHeight    = 960;

// The shipped Aspyr executable is non-PIE, so these absolute addresses are valid only
// for the SHA-256 listed in the hook manifest.
static constexpr uintptr_t kCommittedWidthAddress = 0x1005d3b8c;
static constexpr uintptr_t kCommittedHeightAddress = 0x1005d3b90;
static uint64_t g_resolutionGeneration = 1;
static uint64_t g_layoutGeneration = 1;
static bool s_committedResolutionObserved = false;
static float s_iniMenuScale = 0.0f;
static float s_iniCombatScale = 1.60f;
static float s_iniHudScale = 1.50f;
static float s_iniFontScale = 0.0f;
static bool s_iniHDMenuTextures = false;
static void RefreshRuntimeConfiguration();

namespace ScaledFont {
void RefreshAllLoadedFonts();
}

struct Rect {
    int left, top, width, height;
};

template <typename T>
static T ClampValue(T value, T minimum, T maximum) {
    return value < minimum ? minimum : (value > maximum ? maximum : value);
}

// Readability cache is valid only within one exported hook invocation. Game objects are
// frequently destroyed and their address ranges can be unmapped or reused, so retaining a
// successful VM-region query across frames would turn stale pointers into false positives.
static thread_local vm_address_t s_readRegionStart = 0;
static thread_local vm_size_t s_readRegionSize = 0;
static thread_local vm_prot_t s_readRegionProtection = 0;

static void ResetReadableRegionCache() {
    s_readRegionStart = 0;
    s_readRegionSize = 0;
    s_readRegionProtection = 0;
}

// Safe memory readability check using macOS Mach virtual-memory regions.
bool is_readable(const void* ptr) {
    if (!ptr) return false;
    const uintptr_t u = (uintptr_t)ptr;
    if (u < 0x10000 || u > 0x7fffffffffffULL) return false;

    const vm_address_t addressValue = (vm_address_t)ptr;
    if (addressValue >= s_readRegionStart &&
        addressValue < s_readRegionStart + s_readRegionSize &&
        (s_readRegionProtection & VM_PROT_READ)) {
        return true;
    }

    vm_address_t regionAddress = addressValue;
    vm_size_t regionSize = 0;
    mach_port_t objectName = MACH_PORT_NULL;
    vm_region_basic_info_data_64_t info{};
    mach_msg_type_number_t infoCount = VM_REGION_BASIC_INFO_COUNT_64;
    const kern_return_t result = vm_region_64(
        mach_task_self(), &regionAddress, &regionSize, VM_REGION_BASIC_INFO_64,
        (vm_region_info_t)&info, &infoCount, &objectName);

    if (objectName != MACH_PORT_NULL) {
        mach_port_deallocate(mach_task_self(), objectName);
    }
    if (result != KERN_SUCCESS ||
        addressValue < regionAddress || addressValue >= regionAddress + regionSize ||
        !(info.protection & VM_PROT_READ)) {
        return false;
    }

    s_readRegionStart = regionAddress;
    s_readRegionSize = regionSize;
    s_readRegionProtection = info.protection;
    return true;
}

// Every externally-entered handler starts a fresh memory-validity epoch.
#define BEGIN_HOOK() ResetReadableRegionCache()

/*
  SetControlRect:
  Invokes the control's native virtual method SetExtent (vtable index 2)
  which safely re-positions the control, its hitboxes, textures, and child renderers.
*/
void SetControlRect(char* ctrl, Rect r) {
    if (!ctrl || !is_readable(ctrl)) return;
    void** vtable = *(void***)ctrl;
    if (vtable && is_readable(vtable)) {
        typedef void (*SetExtentFn)(void*, const Rect*);
        SetExtentFn fn = (SetExtentFn)vtable[2];
        if (fn && is_readable((void*)fn)) {
            fn(ctrl, &r);
            return;
        }
    }
    if (is_readable(ctrl + 0x8) && is_readable(ctrl + 0x17)) {
        memcpy(ctrl + 0x8, &r, sizeof(r));
    }
}

// Embedded tabs are child panels of CSWGuiInGameMenu. They must use client-relative
// input coordinates and must not receive the top-level centering transform a second time.
bool isEmbeddedInGameMenuPanel(void* vtable) {
    return vtable == (void*)0x1005ab508 || // CSWGuiInGameEquip
           vtable == (void*)0x1005a75c0 || // CSWGuiInGameInventory
           vtable == (void*)0x1005ad790 || // CSWGuiInGameCharacter
           vtable == (void*)0x1005a5f80 || // CSWGuiInGameAbilities
           vtable == (void*)0x1005aed10 || // CSWGuiInGameJournal
           vtable == (void*)0x1005ab010 || // CSWGuiInGameMap
           vtable == (void*)0x1005ae790 || // CSWGuiInGameMessages
           vtable == (void*)0x1005aba30;   // CSWGuiInGameOptions
}

// Independent roots are centered by CSWGuiPanel. The embedded tab classes above are
// deliberately excluded even though they are recognized menu panels.
bool isTopLevelMenu(void* vtable) {
    return vtable == (void*)0x1005ae6a0 || // CSWGuiInGameMenu master
           vtable == (void*)0x1005abd60 || // CSWGuiLoadScreen
           vtable == (void*)0x1005ae300 || // CSWGuiSaveLoad
           vtable == (void*)0x1005ada20 || // CSWGuiPartySelection
           vtable == (void*)0x1005ab248 || // CSWGuiInGameGalaxyMap
           vtable == (void*)0x1005a5180 || // CSWGuiUpgrade
           vtable == (void*)0x1005ad040 || // CSWGuiStore
           vtable == (void*)0x1005abc50 || // CSWGuiTitleMovies
           vtable == (void*)0x1005af890 || // CSWGuiClassSelection
           vtable == (void*)0x1005ad530 || // CSWGuiMainCharGen
           vtable == (void*)0x1005afea0 || // CSWGuiPortraitCharGen
           vtable == (void*)0x1005aac10 || // CSWGuiNameChargen
           vtable == (void*)0x1005b0950 || // CSWGuiAbilitiesCharGen
           vtable == (void*)0x1005a7820 || // CSWGuiSkillsCharGen
           vtable == (void*)0x1005adc40 || // CSWGuiFeatsCharGen
           vtable == (void*)0x1005a5a90 || // CSWGuiPazaakStart
           vtable == (void*)0x1005a5b80 || // CSWGuiPazaakGame
           vtable == (void*)0x1005abfe0 || // CSWGuiOptionsMain
           vtable == (void*)0x1005ac490 || // CSWGuiOptionsSound
           vtable == (void*)0x1005ac1c0 || // CSWGuiOptionsGraphics
           vtable == (void*)0x1005ac2b0 || // CSWGuiOptionsGraphicsAdvanced
           vtable == (void*)0x1005ac3a0 || // CSWGuiOptionsResolution
           vtable == (void*)0x1005ac580 || // CSWGuiOptionsMouse
           vtable == (void*)0x1005ac0d0 || // CSWGuiOptionsFeedback
           vtable == (void*)0x1005a76d0 || // CSWGuiInGameGameplay
           vtable == (void*)0x1005a72d0 || // CSWGuiInGameOptKeyMappings
           vtable == (void*)0x1005a5d30 || // CSWGuiInGameAutoPause options
           vtable == (void*)0x1005aad20 || // CSWGuiInGameCredits
           vtable == (void*)0x1005a4fa0 || // CSWGuiUpgradeSelection
           vtable == (void*)0x1005a5090 || // CSWGuiUpgradeItemSelect
           vtable == (void*)0x1005abb40;   // CSWGuiPowersLevelUp
}

static bool isMainMenuPanel(void* vtable) {
    return vtable == (void*)0x1005aefa0;
}

static bool isMainInterfacePanel(void* vtable) {
    return vtable == (void*)0x1005a6220;
}

// Check if a panel is one of the small chargen/level-up root panels (qorcpnl, custpnl, quickpnl, leveluppnl)
bool isSmallChargenPanel(void* vtable) {
    return vtable == (void*)0x1005a9a30 || // CSWGuiQuickOrCustomPanel (qorcpnl)
           vtable == (void*)0x1005a6960 || // CSWGuiCustomPanel (custpnl)
           vtable == (void*)0x1005adb30 || // CSWGuiQuickPanel (quickpnl)
           vtable == (void*)0x1005a9b40;   // CSWGuiLevelUpCharGen (leveluppnl)
}

// Check if a panel is a popup dialog (container, message box, pause, etc.)
bool isPopupPanel(void* vtable) {
    return vtable == (void*)0x1005ab758 || // CSWGuiContainer (Footlockers, corpses, placeable containers)
           vtable == (void*)0x1005ae880 || // CSWGuiMessageBox master vtable
           vtable == (void*)0x1005ae9a0 || // CSWGuiStatusSummary
           vtable == (void*)0x1005ad640 || // CSWGuiInGamePause
           vtable == (void*)0x1005a67c0 || // CSWGuiInGameAreaTransition
           vtable == (void*)0x1005abea0 || // CSWGuiInGameSoloModeQuery
           vtable == (void*)0x1005a59a0 || // CSWGuiWagerPopup
           vtable == (void*)0x1005a8c60 || // CSWGuiTutorialBox
           vtable == (void*)0x1005a9e18 || // CSWGuiSkillInfoBox (Feats / Skills granted popup)
           vtable == (void*)0x1005ae3f0 || // CSWGuiSaveNamePanel
           vtable == (void*)0x1005aeaa8 || // CSWGuiControllerLossBox
           vtable == (void*)0x1005aee60;   // CSWGuiExamine
}

static char* g_lastScaledHud = nullptr;

static void CorrectFloatingTargetClamp(char* hud);
static void ApplyFloatingTargetGeometry(char* owner);
static void ForgetTargetClampState(char* hud);
static void PositionBarkBubble(char* window);
static char* s_lastTargetActionMenu = nullptr;
static uint64_t s_lastTargetGeneration = 0;

/*
  The renderer commits its physical framebuffer dimensions in CAurInternalGL::Reshape.
  Those dimensions, not a live swkotor.ini edit, own the shared scale state.
*/
static bool IsValidFramebufferSize(int width, int height) {
    return width >= 640 && width <= 16384 && height >= 480 && height <= 16384;
}

static void CommitRendererResolution(int width, int height) {
    if (!IsValidFramebufferSize(width, height)) return;
    if (s_committedResolutionObserved && width == g_targetWidth && height == g_targetHeight) return;

    g_targetWidth = width;
    g_targetHeight = height;
    s_committedResolutionObserved = true;
    ++g_resolutionGeneration;
    ++g_layoutGeneration;
    g_lastScaledHud = nullptr;
    s_lastTargetGeneration = 0;
}

static void TryAdoptCommittedResolution() {
    if (!is_readable((const void*)kCommittedWidthAddress) ||
        !is_readable((const void*)kCommittedHeightAddress)) {
        return;
    }

    const int width = *(volatile int*)kCommittedWidthAddress;
    const int height = *(volatile int*)kCommittedHeightAddress;
    if (IsValidFramebufferSize(width, height) &&
        (!s_committedResolutionObserved || width != g_targetWidth || height != g_targetHeight)) {
        CommitRendererResolution(width, height);
    }
}

// Runs inside CAurInternalGL::Reshape immediately after the stock function has
// written both committed framebuffer globals. The null-context return path branches
// around this site, so only real mode commits reach the handler.
extern "C" void Hook_RendererReshape(int width, int height) {
    BEGIN_HOOK();
    CommitRendererResolution(width, height);
}

/*
  updateEngineGlobals:
  Synchronizes GUI-manager-owned coordinate canvases with the renderer-owned resolution.
  It deliberately does not overwrite the renderer globals or AurGUI viewport scratch fields.
*/
void updateEngineGlobals(char* mgr) {
    TryAdoptCommittedResolution();

    // CSWGuiManager canvas extents.
    if (mgr && is_readable(mgr)) {
        *(short*)(mgr + 0xa4) = (short)g_targetWidth;
        *(short*)(mgr + 0xa6) = (short)g_targetHeight;
    }

    // Floating Tooltip Window bounds (CSWGuiToolTipPanel via global tooltip manager).
    char** pTooltipMgr = (char**)0x100677ce0;
    if (pTooltipMgr && is_readable(pTooltipMgr)) {
        char* tooltipMgr = *pTooltipMgr;
        if (tooltipMgr && is_readable(tooltipMgr)) {
            char* tooltipWin = *(char**)(tooltipMgr + 0x60);
            if (tooltipWin && is_readable(tooltipWin)) {
                *(int*)(tooltipWin + 0x8) = 0;
                *(int*)(tooltipWin + 0xC) = 0;
                *(int*)(tooltipWin + 0x10) = g_targetWidth;
                *(int*)(tooltipWin + 0x14) = g_targetHeight;
                if (is_readable(tooltipWin + 0x100)) {
                    *(int*)(tooltipWin + 0x100) = 500;
                }
            }
        }
    }
}

static std::unordered_set<void*> s_scaledPanels;
static int s_lastScaleTargetHeight = 0;
static uint64_t s_lastScaleGeneration = 0;
static float ReadIniMenuScale() {
    RefreshRuntimeConfiguration();
    return s_iniMenuScale;
}

static float ReadIniCombatScale() {
    RefreshRuntimeConfiguration();
    return s_iniCombatScale;
}

static float ReadIniHudScale() {
    RefreshRuntimeConfiguration();
    return s_iniHudScale;
}

static bool ReadIniHDMenuTextures() {
    RefreshRuntimeConfiguration();
    return s_iniHDMenuTextures;
}

/*
  Runtime code/data patching helpers.

  Each functional update is applied as one page transaction: all affected pages are
  made writable before any operand changes, every replacement is copied, and the
  original protections are restored only after all writes complete. If protection
  restoration fails, the complete byte set is rolled back before returning failure.
*/
static std::mutex s_runtimeWriteMutex;

struct PendingRuntimeWrite {
    uintptr_t address = 0;
    std::vector<uint8_t> bytes;
};

struct RuntimePageState {
    vm_address_t page = 0;
    vm_size_t size = 0;
    vm_prot_t protection = 0;
};

static PendingRuntimeWrite MakeIntWrite(uintptr_t address, int value) {
    PendingRuntimeWrite write;
    write.address = address;
    write.bytes.resize(sizeof(value));
    memcpy(write.bytes.data(), &value, sizeof(value));
    return write;
}

static bool QueryPageState(vm_address_t page, RuntimePageState& state) {
    vm_address_t regionAddress = page;
    vm_size_t regionSize = 0;
    mach_port_t objectName = MACH_PORT_NULL;
    vm_region_basic_info_data_64_t info{};
    mach_msg_type_number_t infoCount = VM_REGION_BASIC_INFO_COUNT_64;
    const kern_return_t result = vm_region_64(
        mach_task_self(), &regionAddress, &regionSize, VM_REGION_BASIC_INFO_64,
        (vm_region_info_t)&info, &infoCount, &objectName);
    if (objectName != MACH_PORT_NULL) {
        mach_port_deallocate(mach_task_self(), objectName);
    }
    if (result != KERN_SUCCESS || page < regionAddress || page >= regionAddress + regionSize) {
        return false;
    }
    state.page = page;
    state.size = vm_page_size;
    state.protection = info.protection;
    return true;
}

static bool MakePagesWritable(const std::vector<RuntimePageState>& pages) {
    size_t changed = 0;
    for (; changed < pages.size(); ++changed) {
        const RuntimePageState& page = pages[changed];
        const vm_prot_t writable = page.protection | VM_PROT_WRITE | VM_PROT_COPY;
        if (vm_protect(mach_task_self(), page.page, page.size, FALSE, writable) != KERN_SUCCESS) {
            break;
        }
    }
    if (changed == pages.size()) return true;
    while (changed > 0) {
        --changed;
        const RuntimePageState& page = pages[changed];
        vm_protect(mach_task_self(), page.page, page.size, FALSE, page.protection);
    }
    return false;
}

static bool RestorePageProtections(const std::vector<RuntimePageState>& pages) {
    bool ok = true;
    for (const RuntimePageState& page : pages) {
        if (vm_protect(mach_task_self(), page.page, page.size, FALSE, page.protection) != KERN_SUCCESS) {
            ok = false;
        }
    }
    return ok;
}

static bool ApplyRuntimeWriteBatch(const std::vector<PendingRuntimeWrite>& writes) {
    if (writes.empty()) return true;
    std::lock_guard<std::mutex> lock(s_runtimeWriteMutex);

    struct OriginalWrite {
        uintptr_t address;
        std::vector<uint8_t> bytes;
    };
    std::vector<OriginalWrite> originals;
    originals.reserve(writes.size());

    std::vector<RuntimePageState> pages;
    const vm_size_t pageSize = vm_page_size;
    for (const PendingRuntimeWrite& write : writes) {
        if (!write.address || write.bytes.empty()) return false;
        const uintptr_t endAddress = write.address + write.bytes.size();
        if (endAddress < write.address ||
            !is_readable((const void*)write.address) ||
            !is_readable((const void*)(endAddress - 1))) {
            return false;
        }

        OriginalWrite original{ write.address, std::vector<uint8_t>(write.bytes.size()) };
        memcpy(original.bytes.data(), (const void*)write.address, original.bytes.size());
        originals.push_back(std::move(original));

        const vm_address_t firstPage = (vm_address_t)(write.address & ~((uintptr_t)pageSize - 1));
        const vm_address_t lastPage = (vm_address_t)((endAddress - 1) & ~((uintptr_t)pageSize - 1));
        for (vm_address_t page = firstPage;; page += pageSize) {
            bool known = false;
            for (const RuntimePageState& existing : pages) {
                if (existing.page == page) {
                    known = true;
                    break;
                }
            }
            if (!known) {
                RuntimePageState state{};
                if (!QueryPageState(page, state)) return false;
                pages.push_back(state);
            }
            if (page == lastPage) break;
        }
    }

    std::sort(pages.begin(), pages.end(),
        [](const RuntimePageState& a, const RuntimePageState& b) { return a.page < b.page; });
    if (!MakePagesWritable(pages)) return false;

    for (const PendingRuntimeWrite& write : writes) {
        memcpy((void*)write.address, write.bytes.data(), write.bytes.size());
        __builtin___clear_cache((char*)write.address,
                                (char*)(write.address + write.bytes.size()));
    }

    if (RestorePageProtections(pages)) return true;

    // A failed restore occurs after bytes were already changed. Re-open every page,
    // restore the complete original byte set, then return failure.
    if (!MakePagesWritable(pages)) return false;
    for (const OriginalWrite& original : originals) {
        memcpy((void*)original.address, original.bytes.data(), original.bytes.size());
        __builtin___clear_cache((char*)original.address,
                                (char*)(original.address + original.bytes.size()));
    }
    RestorePageProtections(pages);
    return false;
}

struct RuntimePatchGuard {
    uintptr_t address;
    uint8_t length;
    uint8_t expected[6];
};

// Every direct __TEXT write below is checked before the first mutation. The TOML
// already restricts the patch to one executable SHA; this second layer prevents a
// conflicting patch or a partially modified executable from being overwritten.
static bool ValidateRuntimePatchSites() {
    static int s_validationState = 0; // 0 unknown, 1 valid, -1 invalid
    if (s_validationState != 0) return s_validationState > 0;

    static const RuntimePatchGuard guards[] = {
        { 0x1002be874, 4, { 0x38, 0x00, 0x00, 0x00 } },
        { 0x1002bff71, 4, { 0x38, 0x00, 0x00, 0x00 } },
        { 0x10021c48f, 4, { 0x38, 0x00, 0x00, 0x00 } },
        { 0x1002c4758, 4, { 0x38, 0x00, 0x00, 0x00 } },
        { 0x10022f60f, 4, { 0x2a, 0x00, 0x00, 0x00 } },
        { 0x1002ff3cf, 4, { 0x2a, 0x00, 0x00, 0x00 } },
        { 0x1002be4a4, 4, { 0x2a, 0x00, 0x00, 0x00 } },
        { 0x1002be4a9, 4, { 0x15, 0x00, 0x00, 0x00 } },
        { 0x1002be4ce, 4, { 0x13, 0x00, 0x00, 0x00 } },
        { 0x1002bfbac, 4, { 0x2a, 0x00, 0x00, 0x00 } },
        { 0x1002bfbb1, 4, { 0x15, 0x00, 0x00, 0x00 } },
        { 0x1002bfbd6, 4, { 0x13, 0x00, 0x00, 0x00 } },
        { 0x1003065a2, 4, { 0x20, 0x00, 0x00, 0x00 } },
        { 0x100306879, 4, { 0xb8, 0x01, 0x00, 0x00 } },
        { 0x100306881, 4, { 0x17, 0x01, 0x00, 0x00 } },
        { 0x10030688d, 4, { 0xb7, 0x01, 0x00, 0x00 } },
        { 0x1003068ff, 4, { 0x17, 0x01, 0x00, 0x00 } },
        { 0x100307dbc, 4, { 0xb7, 0x01, 0x00, 0x00 } },
        { 0x100307e03, 4, { 0xb7, 0x01, 0x00, 0x00 } },
        // Post-TOML operands for the player-arrow structural rewrite.
        { 0x1002b54d7, 4, { 0xf0, 0xff, 0xff, 0xff } },
        { 0x1002b54e3, 4, { 0xf0, 0xff, 0xff, 0xff } },
        { 0x1002b550e, 4, { 0x20, 0x00, 0x00, 0x00 } },
        { 0x1002b5513, 4, { 0x20, 0x00, 0x00, 0x00 } },
        { 0x10049dcca, 4, { 0x80, 0xfd, 0xff, 0xff } },
        { 0x10049dcd4, 4, { 0x80, 0xfd, 0xff, 0xff } },
        { 0x10049ddb2, 4, { 0x80, 0xfd, 0xff, 0xff } },
        { 0x10049ddbc, 4, { 0x80, 0xfd, 0xff, 0xff } },
        { 0x10049e855, 4, { 0x80, 0xfd, 0xff, 0xff } },
        { 0x10049e85f, 4, { 0x80, 0xfd, 0xff, 0xff } },
        { 0x1002b4b3b, 4, { 0x80, 0xfd, 0xff, 0xff } },
        { 0x1002b4b46, 4, { 0x80, 0xfd, 0xff, 0xff } },
        { 0x1002b5615, 4, { 0x80, 0xfd, 0xff, 0xff } },
        { 0x1002b561f, 4, { 0x80, 0xfd, 0xff, 0xff } },
        { 0x10049dce8, 4, { 0x20, 0xfe, 0xff, 0xff } },
        { 0x10049dcf2, 4, { 0x20, 0xfe, 0xff, 0xff } },
        { 0x10049ddd9, 4, { 0x20, 0xfe, 0xff, 0xff } },
        { 0x10049dde3, 4, { 0x20, 0xfe, 0xff, 0xff } },
        { 0x10049e872, 4, { 0x20, 0xfe, 0xff, 0xff } },
        { 0x10049e87c, 4, { 0x20, 0xfe, 0xff, 0xff } },
        { 0x1002b4b59, 4, { 0x20, 0xfe, 0xff, 0xff } },
        { 0x1002b4b63, 4, { 0x20, 0xfe, 0xff, 0xff } },
        { 0x1002b5631, 4, { 0x20, 0xfe, 0xff, 0xff } },
        { 0x1002b563b, 4, { 0x20, 0xfe, 0xff, 0xff } },
    };

    for (const RuntimePatchGuard& guard : guards) {
        const void* memory = (const void*)guard.address;
        if (!is_readable(memory) || !is_readable((const char*)memory + guard.length - 1) ||
            memcmp(memory, guard.expected, guard.length) != 0) {
            s_validationState = -1;
            fprintf(stderr,
                    "[ScaledKotorWidescreen] Runtime patch guard failed at 0x%llx; "
                    "direct code patches are disabled.\n",
                    (unsigned long long)guard.address);
            return false;
        }
    }

    s_validationState = 1;
    return true;
}

/*
  Windows-parity list-item scaling
  --------------------------------
  The upstream Windows "Scaled List Items" patch does not scale a 56-pixel row
  by Height / 480. It uses Scaled Kotor's compact-content transform:

      result = round(baseValue * contentScale * 4 / 9)

  where contentScale is the uniform 640x480 layout scale multiplied by the
  configured additional scale factor (1.2 by default). Compact-content scaling
  is disabled below 1080 pixels of framebuffer height. Consequently, a standard
  row remains 56 pixels at 960p/982p, becomes 67 pixels at 1080p, 90 pixels at
  1440p, and 134 pixels at 2160p.

  All row geometry below is derived from immutable authored values. It is never
  derived from a control extent that CSWGuiListBox may already have redistributed.
*/
static int DivideRoundedNearest(int64_t numerator, int64_t denominator) {
    if (denominator <= 0) return (int)numerator;
    const int64_t half = denominator / 2;
    if (numerator >= 0) return (int)((numerator + half) / denominator);
    return -(int)((-numerator + half) / denominator);
}

struct UiTuningKnobs {
    // Matches ScaledKotor.ini ScaleFactor=12. Values >=10 are interpreted as
    // tenths (12 => 1.2); values 1..9 are whole multipliers, matching Windows.
    int itemScaleFactor = 12;

    // Zero selects the Windows-derived value. Positive values are exact final
    // pixels and are deliberately not multiplied a second time.
    int itemHeight = 0;
    int containerItemHeight = 0;
    int skillHeight = 0;
    int featHeight = 0;

    // -1 preserves the layout-authored list-box spacing. A non-negative value
    // explicitly enables custom padding.
    int itemPadding = -1;
    int containerPadding = -1;

    // Optional final-pixel overrides. Zero means derive from the row height or
    // retain the normally scaled list-box extent.
    int listHeight = 0;
    int iconWidth = 0;
    int iconHeight = 0;
    int iconTopOffset = 0;
    int textOffset = 0;
    int textDeduct = 0;
    int listLeftOffset = 0;
    int listTopOffset = 0;
    int listWidthOffset = 0;
    int badgeOffset = 0;      // additive X correction after native placement
    int badgeTopOffset = 0;   // additive Y correction after Windows scaling
    int itemHeightDiagnostics = 0;
};

static UiTuningKnobs s_currentKnobs;

static bool UiTuningKnobsEqual(const UiTuningKnobs& a, const UiTuningKnobs& b) {
    return a.itemScaleFactor == b.itemScaleFactor &&
        a.itemHeight == b.itemHeight &&
        a.containerItemHeight == b.containerItemHeight &&
        a.skillHeight == b.skillHeight &&
        a.featHeight == b.featHeight &&
        a.itemPadding == b.itemPadding &&
        a.containerPadding == b.containerPadding &&
        a.listHeight == b.listHeight &&
        a.iconWidth == b.iconWidth &&
        a.iconHeight == b.iconHeight &&
        a.iconTopOffset == b.iconTopOffset &&
        a.textOffset == b.textOffset &&
        a.textDeduct == b.textDeduct &&
        a.listLeftOffset == b.listLeftOffset &&
        a.listTopOffset == b.listTopOffset &&
        a.listWidthOffset == b.listWidthOffset &&
        a.badgeOffset == b.badgeOffset &&
        a.badgeTopOffset == b.badgeTopOffset &&
        a.itemHeightDiagnostics == b.itemHeightDiagnostics;
}

static time_t s_lastIniModSeconds = 0;
static long s_lastIniModNanoseconds = 0;
static off_t s_lastIniSize = -1;
static bool s_lastIniExists = false;
static bool s_runtimeConfigInitialized = false;
static int s_knobVersion = 0;
static std::chrono::steady_clock::time_point s_lastConfigPoll{};

static char* TrimIniToken(char* value) {
    if (!value) return value;
    while (*value == ' ' || *value == '\t' || *value == '\r' || *value == '\n') ++value;
    char* end = value + strlen(value);
    while (end > value && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' || end[-1] == '\n')) {
        *--end = '\0';
    }
    return value;
}

static bool FloatChanged(float a, float b) {
    return std::fabs(a - b) > 0.0001f;
}

static int ClampZeroOrRange(int value, int minimum, int maximum) {
    return value == 0 ? 0 : ClampValue(value, minimum, maximum);
}

static void RefreshRuntimeConfiguration() {
    TryAdoptCommittedResolution();

    // Hot draw hooks share this reader. Polling the file at most four times per
    // second preserves live tuning without issuing filesystem metadata calls for
    // every control scaled during a frame. Renderer commits are adopted above
    // immediately and are not subject to this throttle.
    const auto now = std::chrono::steady_clock::now();
    if (s_runtimeConfigInitialized &&
        now - s_lastConfigPoll < std::chrono::milliseconds(250)) {
        return;
    }
    s_lastConfigPoll = now;

    const char* home = getenv("HOME");
    char path[1024]{};
    bool exists = false;
    struct stat st{};
    if (home) {
        snprintf(path, sizeof(path),
                 "%s/Library/Application Support/Knights of the Old Republic/swkotor.ini",
                 home);
        exists = stat(path, &st) == 0;
    }

    const time_t mtimeSeconds = exists ? st.st_mtime : 0;
#if defined(__APPLE__)
    const long mtimeNanoseconds = exists ? st.st_mtimespec.tv_nsec : 0;
#else
    const long mtimeNanoseconds = exists ? st.st_mtim.tv_nsec : 0;
#endif
    const off_t fileSize = exists ? st.st_size : -1;
    if (s_runtimeConfigInitialized && exists == s_lastIniExists &&
        mtimeSeconds == s_lastIniModSeconds &&
        mtimeNanoseconds == s_lastIniModNanoseconds &&
        fileSize == s_lastIniSize) {
        return;
    }

    float newMenuScale = 0.0f;
    float newCombatScale = 1.60f;
    float newHudScale = 1.50f;
    float newFontScale = 0.0f;
    bool newHDMenuTextures = false;
    UiTuningKnobs newKnobs{};

    if (exists) {
        FILE* file = fopen(path, "r");
        if (file) {
            enum class Section { None, Graphics, UiTuning } section = Section::None;
            char line[512];
            while (fgets(line, sizeof(line), file)) {
                char* p = TrimIniToken(line);
                if (*p == '\0' || *p == ';' || *p == '#') continue;
                if (*p == '[') {
                    section = Section::None;
                    if (strncasecmp(p, "[Graphics Options]", 18) == 0) section = Section::Graphics;
                    else if (strncasecmp(p, "[UI Tuning]", 11) == 0) section = Section::UiTuning;
                    continue;
                }

                char* equals = strchr(p, '=');
                if (!equals) continue;
                *equals = '\0';
                char* key = TrimIniToken(p);
                char* value = TrimIniToken(equals + 1);

                if (section == Section::Graphics) {
                    // Width and Height are consumed by the game. The renderer's committed
                    // mode arrives through Hook_RendererReshape, so this module does not
                    // interpret an INI edit as a completed mode switch.
                    if (strcasecmp(key, "MenuScale") == 0) {
                        const float parsed = (float)atof(value);
                        if (parsed > 0.0f && parsed <= 10.0f) newMenuScale = parsed;
                    } else if (strcasecmp(key, "CombatScale") == 0) {
                        const float parsed = (float)atof(value);
                        if (parsed >= 0.5f && parsed <= 5.0f) newCombatScale = parsed;
                    } else if (strcasecmp(key, "HudScale") == 0 ||
                               strcasecmp(key, "PortraitScale") == 0) {
                        const float parsed = (float)atof(value);
                        if (parsed >= 0.5f && parsed <= 5.0f) newHudScale = parsed;
                    } else if (strcasecmp(key, "HDMenuTextures") == 0) {
                        newHDMenuTextures = atoi(value) != 0;
                    } else if (strcasecmp(key, "FontScale") == 0) {
                        const float parsed = (float)atof(value);
                        if (parsed > 0.0f && parsed <= 10.0f) newFontScale = parsed;
                    } else if (strcasecmp(key, "ScaleFactor") == 0) {
                        // Alias for the upstream Windows ScaledKotor.ini setting.
                        newKnobs.itemScaleFactor = atoi(value);
                    }
                } else if (section == Section::UiTuning) {
                    const int parsed = atoi(value);
                    if (strcasecmp(key, "ItemScaleFactor") == 0 || strcasecmp(key, "ScaleFactor") == 0) newKnobs.itemScaleFactor = parsed;
                    else if (strcasecmp(key, "ItemHeight") == 0) newKnobs.itemHeight = parsed;
                    else if (strcasecmp(key, "ContainerItemHeight") == 0) newKnobs.containerItemHeight = parsed;
                    else if (strcasecmp(key, "SkillHeight") == 0) newKnobs.skillHeight = parsed;
                    else if (strcasecmp(key, "FeatHeight") == 0 ||
                             strcasecmp(key, "PowerHeight") == 0) newKnobs.featHeight = parsed;
                    else if (strcasecmp(key, "ItemPadding") == 0) newKnobs.itemPadding = parsed;
                    else if (strcasecmp(key, "ContainerPadding") == 0) newKnobs.containerPadding = parsed;
                    else if (strcasecmp(key, "ListHeight") == 0) newKnobs.listHeight = parsed;
                    else if (strcasecmp(key, "IconWidth") == 0) newKnobs.iconWidth = parsed;
                    else if (strcasecmp(key, "IconHeight") == 0) newKnobs.iconHeight = parsed;
                    else if (strcasecmp(key, "IconTopOffset") == 0) newKnobs.iconTopOffset = parsed;
                    else if (strcasecmp(key, "TextOffset") == 0) newKnobs.textOffset = parsed;
                    else if (strcasecmp(key, "TextDeduct") == 0) newKnobs.textDeduct = parsed;
                    else if (strcasecmp(key, "ListLeftOffset") == 0) newKnobs.listLeftOffset = parsed;
                    else if (strcasecmp(key, "ListTopOffset") == 0) newKnobs.listTopOffset = parsed;
                    else if (strcasecmp(key, "ListWidthOffset") == 0) newKnobs.listWidthOffset = parsed;
                    else if (strcasecmp(key, "BadgeOffset") == 0 ||
                             strcasecmp(key, "BadgeXOffset") == 0) newKnobs.badgeOffset = parsed;
                    else if (strcasecmp(key, "BadgeTopOffset") == 0 ||
                             strcasecmp(key, "BadgeYOffset") == 0) newKnobs.badgeTopOffset = parsed;
                    else if (strcasecmp(key, "ItemHeightDiagnostics") == 0) newKnobs.itemHeightDiagnostics = parsed;
                }
            }
            fclose(file);
        }
    }

    newKnobs.itemScaleFactor = ClampValue(newKnobs.itemScaleFactor, 1, 100);
    newKnobs.itemHeight = ClampZeroOrRange(newKnobs.itemHeight, 16, 2048);
    newKnobs.containerItemHeight = ClampZeroOrRange(newKnobs.containerItemHeight, 16, 2048);
    newKnobs.skillHeight = ClampZeroOrRange(newKnobs.skillHeight, 12, 2048);
    newKnobs.featHeight = ClampZeroOrRange(newKnobs.featHeight, 12, 2048);
    newKnobs.itemPadding = ClampValue(newKnobs.itemPadding, -1, 255);
    newKnobs.containerPadding = ClampValue(newKnobs.containerPadding, -1, 255);
    newKnobs.listHeight = ClampValue(newKnobs.listHeight, 0, 16384);
    newKnobs.iconWidth = ClampZeroOrRange(newKnobs.iconWidth, 8, 4096);
    newKnobs.iconHeight = ClampZeroOrRange(newKnobs.iconHeight, 8, 4096);
    newKnobs.iconTopOffset = ClampValue(newKnobs.iconTopOffset, -4096, 4096);
    newKnobs.textOffset = ClampValue(newKnobs.textOffset, -4096, 8192);
    newKnobs.textDeduct = ClampValue(newKnobs.textDeduct, -4096, 8192);
    newKnobs.listLeftOffset = ClampValue(newKnobs.listLeftOffset, -4096, 4096);
    newKnobs.listTopOffset = ClampValue(newKnobs.listTopOffset, -4096, 4096);
    newKnobs.listWidthOffset = ClampValue(newKnobs.listWidthOffset, -4096, 4096);
    newKnobs.badgeOffset = ClampValue(newKnobs.badgeOffset, -4096, 4096);
    newKnobs.badgeTopOffset = ClampValue(newKnobs.badgeTopOffset, -4096, 4096);
    newKnobs.itemHeightDiagnostics = newKnobs.itemHeightDiagnostics != 0 ? 1 : 0;

    const bool knobsChanged = !UiTuningKnobsEqual(newKnobs, s_currentKnobs);
    const bool configChanged = !s_runtimeConfigInitialized ||
        FloatChanged(newMenuScale, s_iniMenuScale) ||
        FloatChanged(newCombatScale, s_iniCombatScale) ||
        FloatChanged(newHudScale, s_iniHudScale) ||
        FloatChanged(newFontScale, s_iniFontScale) ||
        newHDMenuTextures != s_iniHDMenuTextures || knobsChanged;

    s_iniMenuScale = newMenuScale;
    s_iniCombatScale = newCombatScale;
    s_iniHudScale = newHudScale;
    s_iniFontScale = newFontScale;
    s_iniHDMenuTextures = newHDMenuTextures;
    if (knobsChanged || !s_runtimeConfigInitialized) {
        s_currentKnobs = newKnobs;
        ++s_knobVersion;
    }

    s_lastIniExists = exists;
    s_lastIniModSeconds = mtimeSeconds;
    s_lastIniModNanoseconds = mtimeNanoseconds;
    s_lastIniSize = fileSize;
    s_runtimeConfigInitialized = true;

    if (configChanged) {
        ++g_layoutGeneration;
        g_lastScaledHud = nullptr;
        s_scaledPanels.clear();
    }
}

static bool CheckAndReloadUiKnobs() {
    const int oldVersion = s_knobVersion;
    RefreshRuntimeConfiguration();
    return s_knobVersion != oldVersion;
}

struct UniversalScaleState {
    int screenWidth;
    int screenHeight;
    int uiWidth;
    int uiHeight;
    int64_t scaleNumerator;
    int64_t scaleDenominator;
    int64_t contentScaleNumerator;
    int64_t contentScaleDenominator;
    bool contentScalingEnabled;
    uint64_t layoutGeneration;
};

static UniversalScaleState GetUniversalScaleState() {
    RefreshRuntimeConfiguration();

    int64_t scaleNumerator = 1;
    int64_t scaleDenominator = 1;
    if ((int64_t)g_targetHeight * 640 <= (int64_t)g_targetWidth * 480) {
        scaleNumerator = g_targetHeight;
        scaleDenominator = 480;
    } else {
        scaleNumerator = g_targetWidth;
        scaleDenominator = 640;
    }

    const bool contentScalingEnabled = g_targetHeight >= 1080;
    const int factorSetting = s_currentKnobs.itemScaleFactor;
    const int64_t factorNumerator = factorSetting >= 10 ? factorSetting : factorSetting * 10LL;
    const int64_t factorDenominator = 10;

    UniversalScaleState state{};
    state.screenWidth = g_targetWidth;
    state.screenHeight = g_targetHeight;
    state.scaleNumerator = scaleNumerator;
    state.scaleDenominator = scaleDenominator;
    state.uiWidth = DivideRoundedNearest(640LL * scaleNumerator, scaleDenominator);
    state.uiHeight = DivideRoundedNearest(480LL * scaleNumerator, scaleDenominator);
    state.contentScalingEnabled = contentScalingEnabled;
    state.contentScaleNumerator = contentScalingEnabled ?
        scaleNumerator * factorNumerator : 1;
    state.contentScaleDenominator = contentScalingEnabled ?
        scaleDenominator * factorDenominator : 1;
    state.layoutGeneration = g_layoutGeneration;
    return state;
}

static int ScaleUiValue(int value, const UniversalScaleState& scale) {
    return DivideRoundedNearest((int64_t)value * scale.scaleNumerator,
                                scale.scaleDenominator);
}

static int ScaleUiValueFromBase(int value, int sourceBase, int uiBase,
                                const UniversalScaleState& scale) {
    if (sourceBase <= 0 || uiBase <= 0) return value;
    return DivideRoundedNearest(
        (int64_t)value * uiBase * scale.scaleNumerator,
        (int64_t)sourceBase * scale.scaleDenominator);
}

static int ScaleContainerValue(int value, const UniversalScaleState& scale) {
    if (!scale.contentScalingEnabled) return value;
    return DivideRoundedNearest(
        (int64_t)value * scale.contentScaleNumerator * 8,
        scale.contentScaleDenominator * 9);
}

static int ScaleTwoXValue(int value, const UniversalScaleState& scale) {
    if (!scale.contentScalingEnabled) return value;
    return DivideRoundedNearest(
        (int64_t)value * scale.contentScaleNumerator * 4,
        scale.contentScaleDenominator * 9);
}

static float TwoXScaleAdjustment(const UniversalScaleState& scale) {
    if (!scale.contentScalingEnabled) return 1.0f;
    return (float)(scale.contentScaleNumerator * 4.0) /
           (float)(scale.contentScaleDenominator * 9.0);
}

static void GetMenuCanvasSize(int /*baseWidth*/, int /*baseHeight*/,
                              int& targetWidth, int& targetHeight) {
    const UniversalScaleState scale = GetUniversalScaleState();
    targetWidth = scale.uiWidth;
    targetHeight = scale.uiHeight;

    // MenuScale is a macOS-port override for the one shared 640x480 menu canvas.
    // It must not derive a different process-wide canvas from each panel's source size.
    const float menuScale = ReadIniMenuScale();
    if (menuScale > 0.0f) {
        targetWidth = std::max(1, (int)std::lround(640.0f * menuScale));
        targetHeight = std::max(1, (int)std::lround(480.0f * menuScale));
    }
}

static int ScaleMenuUiValue(int value) {
    int targetWidth = 640;
    int targetHeight = 480;
    GetMenuCanvasSize(640, 480, targetWidth, targetHeight);
    return DivideRoundedNearest((int64_t)value * targetHeight, 480);
}

static int ScaleMenuContentValue(int value) {
    const float explicitScale = ReadIniMenuScale();
    if (explicitScale > 0.0f) {
        return (int)std::lround(value * explicitScale);
    }
    return ScaleTwoXValue(value, GetUniversalScaleState());
}

static Rect ScaleMapMarkerRect(const Rect& authored) {
    const int width = std::max(1, ScaleMenuUiValue(authored.width));
    const int height = std::max(1, ScaleMenuUiValue(authored.height));
    const int64_t centerX2 = (int64_t)authored.left * 2 + authored.width;
    const int64_t centerY2 = (int64_t)authored.top * 2 + authored.height;
    return {
        DivideRoundedNearest(centerX2 - width, 2),
        DivideRoundedNearest(centerY2 - height, 2),
        width,
        height,
    };
}

extern "C" void Hook_MapMarkerStackRect(char* framePointer) {
    BEGIN_HOOK();
    TryAdoptCommittedResolution();
    if (!s_committedResolutionObserved || !framePointer) return;
    Rect* marker = reinterpret_cast<Rect*>(framePointer - 0x1c0);
    if (!is_readable(marker) ||
        !is_readable(reinterpret_cast<char*>(marker) + sizeof(Rect) - 1)) return;
    if (marker->width <= 0 || marker->height <= 0 ||
        marker->width > 256 || marker->height > 256) return;
    *marker = ScaleMapMarkerRect(*marker);
}

extern "C" void Hook_ListBoxScaleScrollbarWidth(char* listBox) {
    BEGIN_HOOK();
    TryAdoptCommittedResolution();
    if (!s_committedResolutionObserved || !listBox || !is_readable(listBox + 0x16b)) return;
    int& width = *(int*)(listBox + 0x168);
    if (width != 0) width = std::max(1, ScaleMenuUiValue(16));
}

static int ScaleWindowsTwoXValue(int baseValue) {
    return ScaleTwoXValue(baseValue, GetUniversalScaleState());
}

int GetScaledItemRowHeight(int baseHeight = 56) {
    return ScaleWindowsTwoXValue(baseHeight);
}

struct ScaledItemGeometry {
    int itemHeight;
    int containerItemHeight;
    int skillHeight;
    int featHeight;
    int itemPadding;
    int containerPadding;
    int listHeight;
    int iconWidth;
    int iconHeight;
    int iconTopOffset;
    int textOffset;
    int textDeduct;
    int listLeftOffset;
    int listTopOffset;
    int listWidthOffset;
    int badgeOffset;
    int badgeTopOffset;
    int quantityHeight;
    int quantityWidthShort;
    int quantityWidthLong;
    int quantityTop;
};

static ScaledItemGeometry GetScaledItemGeometry() {
    RefreshRuntimeConfiguration();
    ScaledItemGeometry geometry{};
    geometry.itemHeight = s_currentKnobs.itemHeight > 0 ?
        s_currentKnobs.itemHeight : ScaleWindowsTwoXValue(0x38);
    geometry.containerItemHeight = s_currentKnobs.containerItemHeight > 0 ?
        s_currentKnobs.containerItemHeight : geometry.itemHeight;
    geometry.skillHeight = s_currentKnobs.skillHeight > 0 ?
        s_currentKnobs.skillHeight : ScaleWindowsTwoXValue(0x2a);
    geometry.featHeight = s_currentKnobs.featHeight > 0 ?
        s_currentKnobs.featHeight : ScaleWindowsTwoXValue(0x28);
    geometry.itemPadding = s_currentKnobs.itemPadding;
    geometry.containerPadding = s_currentKnobs.containerPadding;
    geometry.listHeight = s_currentKnobs.listHeight;
    geometry.iconWidth = s_currentKnobs.iconWidth > 0 ?
        s_currentKnobs.iconWidth : geometry.itemHeight;
    geometry.iconHeight = s_currentKnobs.iconHeight > 0 ?
        s_currentKnobs.iconHeight : geometry.itemHeight;
    geometry.iconTopOffset = s_currentKnobs.iconTopOffset;
    geometry.textOffset = s_currentKnobs.textOffset != 0 ?
        s_currentKnobs.textOffset : geometry.itemHeight;
    geometry.textDeduct = s_currentKnobs.textDeduct != 0 ?
        s_currentKnobs.textDeduct : geometry.itemHeight;
    geometry.listLeftOffset = s_currentKnobs.listLeftOffset;
    geometry.listTopOffset = s_currentKnobs.listTopOffset;
    geometry.listWidthOffset = s_currentKnobs.listWidthOffset;
    geometry.badgeOffset = s_currentKnobs.badgeOffset;
    geometry.badgeTopOffset = s_currentKnobs.badgeTopOffset;
    geometry.quantityHeight = std::max(1, ScaleWindowsTwoXValue(0x13));
    geometry.quantityWidthShort = std::max(1, ScaleWindowsTwoXValue(0x15));
    // The Windows patch intentionally uses the same 0x15 source width for both
    // quantity-width branches.
    geometry.quantityWidthLong = std::max(1, ScaleWindowsTwoXValue(0x15));
    geometry.quantityTop = ScaleWindowsTwoXValue(0x25) + geometry.badgeTopOffset;
    return geometry;
}

static int s_lastPatchedTargetHeight = -1;
static int s_lastPatchedTargetWidth = -1;
static int s_lastPatchedKnobVersion = -1;
static uint64_t s_lastPatchedGeneration = 0;

/*
  Apply generation-dependent immediate fields. The two CSWGuiListBox organizer
  corrections are static TOML hooks at 0x1004a8915 and 0x1004a8921; natural-row
  measurement in OrganizeUnequal remains untouched.
*/
void refreshPatchedListConstants() {
    TryAdoptCommittedResolution();
    if (!s_committedResolutionObserved) return;
    CheckAndReloadUiKnobs();
    if (!ValidateRuntimePatchSites()) return;
    if (s_lastPatchedTargetWidth == g_targetWidth &&
        s_lastPatchedTargetHeight == g_targetHeight &&
        s_lastPatchedKnobVersion == s_knobVersion &&
        s_lastPatchedGeneration == g_layoutGeneration) {
        return;
    }

    const ScaledItemGeometry geometry = GetScaledItemGeometry();
    const UniversalScaleState menuScale = GetUniversalScaleState();
    const float configuredMenuScale = ReadIniMenuScale();
    const auto scaleMenuInteger = [&](int authored) -> int {
        if (configuredMenuScale > 0.0f) {
            return std::max(1, (int)std::lround(authored * configuredMenuScale));
        }
        return std::max(1, ScaleUiValue(authored, menuScale));
    };

    std::vector<PendingRuntimeWrite> writes;
    writes.reserve(21);
    const auto addInt = [&](uintptr_t address, int value) {
        writes.push_back(MakeIntWrite(address, value));
    };

    // Item, store, workbench, and skill constructors start with immutable authored values.
    addInt(0x1002be874, geometry.itemHeight);
    addInt(0x1002bff71, geometry.containerItemHeight);
    addInt(0x10021c48f, geometry.containerItemHeight);
    addInt(0x10022f60f, geometry.skillHeight);

    // Movie and save/load rows are shared-menu geometry. The exact stock save immediate is 42.
    addInt(0x1002c4758, scaleMenuInteger(56));
    addInt(0x1002ff3cf, scaleMenuInteger(42));

    addInt(0x1002be4a4, geometry.quantityWidthLong);
    addInt(0x1002be4a9, geometry.quantityWidthShort);
    addInt(0x1002be4ce, geometry.quantityHeight);
    addInt(0x1002bfbac, geometry.quantityWidthLong);
    addInt(0x1002bfbb1, geometry.quantityWidthShort);
    addInt(0x1002bfbd6, geometry.quantityHeight);

    // FixMessageLabel's exact authored icon/text inset is 32 and belongs to the compact
    // two-X content family, not the full menu-canvas scale.
    addInt(0x1003065a2, std::max(1, ScaleTwoXValue(32, menuScale)));
    addInt(0x100306879, g_targetWidth);
    addInt(0x100306881, g_targetHeight);
    addInt(0x10030688d, g_targetWidth);
    addInt(0x1003068ff, g_targetHeight);
    // CSWGuiStatusSummary uses two <=439 fit loops. Preserve their inclusive
    // comparison while raising the authored 440-pixel ceiling to the framebuffer width.
    addInt(0x100307dbc, std::max(0, g_targetWidth - 1));
    addInt(0x100307e03, std::max(0, g_targetWidth - 1));

    // Dynamic area-map player arrow. The TOML rewrite widens center offsets to imm32.
    const int mapArrowSize = std::max(1, ScaleMenuUiValue(32));
    const int mapArrowOffset = -(mapArrowSize / 2);
    addInt(0x1002b54d7, mapArrowOffset);
    addInt(0x1002b54e3, mapArrowOffset);
    addInt(0x1002b550e, mapArrowSize);
    addInt(0x1002b5513, mapArrowSize);

    if (!ApplyRuntimeWriteBatch(writes)) return;
    s_lastPatchedTargetWidth = g_targetWidth;
    s_lastPatchedTargetHeight = g_targetHeight;
    s_lastPatchedKnobVersion = s_knobVersion;
    s_lastPatchedGeneration = g_layoutGeneration;
}

enum class ItemEntryKind { InGame, Store, Upgrade };

static void SetEmbeddedRect(char* object, const Rect& rect) {
    if (!object || !is_readable(object) || !is_readable(object + 0x17)) return;
    SetControlRect(object, rect);
}

static void SetBorderFillStretch(char* border) {
    if (!border || !is_readable(border + 0x34)) return;
    uint8_t& flags = *(uint8_t*)(border + 0x34);
    flags = (uint8_t)((flags & ~0x03u) | 0x02u);
}

static void LogItemHeightInvariant(const char* context, char* item, int expectedHeight) {
    if (!s_currentKnobs.itemHeightDiagnostics || !item || !is_readable(item + 0x17)) return;
    const Rect& extent = *(const Rect*)(item + 0x8);
    if (extent.height != expectedHeight) {
        fprintf(stderr,
                "[ScaledKotorWidescreen] %s row-height invariant failed: expected %d, got %d at %p\n",
                context, expectedHeight, extent.height, (void*)item);
    }
}

static void ApplyItemEntryGeometry(char* item, ItemEntryKind kind) {
    if (!item || !is_readable(item) || !is_readable(item + 0x410)) return;
    const ScaledItemGeometry geometry = GetScaledItemGeometry();

    Rect row = *(Rect*)(item + 0x8);
    if (row.width <= 0 || row.width > 16384 || row.height < 0 || row.height > 16384) return;

    const bool inGame = kind == ItemEntryKind::InGame;
    const bool upgrade = kind == ItemEntryKind::Upgrade;
    const int rowHeight = inGame ? geometry.itemHeight : geometry.containerItemHeight;
    const int iconWidth = inGame ? geometry.iconWidth : rowHeight;
    const int iconHeight = inGame ? geometry.iconHeight : rowHeight;
    const int iconTop = row.top + (inGame ? geometry.iconTopOffset : 0);
    const int textOffset = inGame ? geometry.textOffset : iconWidth;
    const int textDeduct = inGame ? geometry.textDeduct : iconWidth;

    row.height = rowHeight;
    memcpy(item + 0x8, &row, sizeof(row));

    Rect textRect = {
        row.left + textOffset,
        row.top,
        std::max(1, row.width - textDeduct),
        rowHeight
    };
    SetEmbeddedRect(item + 0xa8, textRect);
    SetEmbeddedRect(item + 0x130, textRect);
    SetControlRect(item + 0x1b8, textRect);
    SetBorderFillStretch(item + 0xa8);
    SetBorderFillStretch(item + 0x130);

    const ptrdiff_t iconBase = upgrade ? 0x258 : 0x248;
    const Rect iconRect = { row.left, iconTop, iconWidth, iconHeight };
    SetEmbeddedRect(item + iconBase, iconRect);
    SetEmbeddedRect(item + iconBase + 0x88, iconRect);
    SetEmbeddedRect(item + iconBase + 0x110, iconRect);

    const ptrdiff_t quantityOffset = upgrade ? 0x3f0 : 0x3e0;
    char* quantity = item + quantityOffset;
    if (is_readable(quantity) && is_readable(quantity + 0x17)) {
        Rect quantityRect = *(Rect*)(quantity + 0x8);
        // Windows patches both quantity-width branches from the same immutable
        // 0x15 source value, so no previous control width is reused here.
        quantityRect.width = geometry.quantityWidthShort;
        quantityRect.height = geometry.quantityHeight;
        quantityRect.left = row.left + iconWidth - quantityRect.width + geometry.badgeOffset;
        quantityRect.top = row.top + geometry.quantityTop;
        SetControlRect(quantity, quantityRect);
    }

    LogItemHeightInvariant(
        inGame ? "inventory/equipment" : (upgrade ? "upgrade/workbench" : "store/container"),
        item, rowHeight);
}

using LabelSetExtentFn = void (*)(void*, const Rect*);
static constexpr uintptr_t kLabelSetExtent = 0x1004a3d4c;

extern "C" void Hook_InGameItemFinalExtent(char* textLabel, const Rect* originalRect) {
    BEGIN_HOOK();
    if (!textLabel || !originalRect) return;
    ((LabelSetExtentFn)kLabelSetExtent)(textLabel, originalRect);
    ApplyItemEntryGeometry(textLabel - 0x1b8, ItemEntryKind::InGame);
}

extern "C" void Hook_StoreItemFinalExtent(char* textLabel, const Rect* originalRect) {
    BEGIN_HOOK();
    if (!textLabel || !originalRect) return;
    ((LabelSetExtentFn)kLabelSetExtent)(textLabel, originalRect);
    ApplyItemEntryGeometry(textLabel - 0x1b8, ItemEntryKind::Store);
}

extern "C" void Hook_UpgradeItemFinalExtent(char* textLabel, const Rect* originalRect) {
    BEGIN_HOOK();
    if (!textLabel || !originalRect) return;
    ((LabelSetExtentFn)kLabelSetExtent)(textLabel, originalRect);
    ApplyItemEntryGeometry(textLabel - 0x1b8, ItemEntryKind::Upgrade);
}

// CSWGuiSkillFlow is the composite row used by Feats and Powers. The first hook
// replaces only the local incoming-height copy, so all native child-layout math
// sees an immutable 40-pixel authored height transformed by the Windows formula.
extern "C" void Hook_SkillFlowLocalHeight(Rect* localExtent) {
    BEGIN_HOOK();
    if (!localExtent) return;
    localExtent->height = GetScaledItemGeometry().featHeight;
}

// The stock epilogue copies the caller's original height back into the stored
// row extent. Replace that final copy so the same immutable value survives every
// repopulation and organizer pass.
extern "C" void Hook_SkillFlowStoredExtent(const Rect* sourceExtent, char* skillFlow) {
    BEGIN_HOOK();
    if (!sourceExtent || !skillFlow) return;
    Rect stored = *sourceExtent;
    stored.height = GetScaledItemGeometry().featHeight;
    memcpy(skillFlow + 0x8, &stored, sizeof(stored));
    LogItemHeightInvariant("feat/power", skillFlow, stored.height);
}

struct ListBoxAuthoredState {
    void* vtable;
    uint8_t flags;
    uint8_t padding;
};

static std::unordered_map<char*, ListBoxAuthoredState> s_listBoxAuthoredStates;

static ListBoxAuthoredState* GetListBoxAuthoredState(char* listBox) {
    if (!listBox || !is_readable(listBox + 0x373)) return nullptr;
    void* vtable = *(void**)listBox;
    auto it = s_listBoxAuthoredStates.find(listBox);
    if (it == s_listBoxAuthoredStates.end() || it->second.vtable != vtable) {
        ListBoxAuthoredState state{
            vtable,
            *(uint8_t*)(listBox + 0x370),
            *(uint8_t*)(listBox + 0x373),
        };
        if (it == s_listBoxAuthoredStates.end()) {
            it = s_listBoxAuthoredStates.emplace(listBox, state).first;
        } else {
            it->second = state;
        }
    }
    return &it->second;
}

static void ConfigureListBoxPadding(char* listBox, int requestedPadding) {
    ListBoxAuthoredState* authored = GetListBoxAuthoredState(listBox);
    if (!authored) return;

    uint8_t& flags = *(uint8_t*)(listBox + 0x370);
    uint8_t& padding = *(uint8_t*)(listBox + 0x373);
    if (requestedPadding >= 0) {
        padding = (uint8_t)ClampValue(requestedPadding, 0, 255);
        flags |= 0x08;
    } else {
        padding = authored->padding;
        flags = (uint8_t)((flags & ~0x08u) | (authored->flags & 0x08u));
    }
}

static void RestoreAndClearTrackedListBoxes() {
    for (const auto& pair : s_listBoxAuthoredStates) {
        char* listBox = pair.first;
        const ListBoxAuthoredState& authored = pair.second;
        if (!listBox || !is_readable(listBox + 0x373) || *(void**)listBox != authored.vtable) {
            continue;
        }
        uint8_t& flags = *(uint8_t*)(listBox + 0x370);
        *(uint8_t*)(listBox + 0x373) = authored.padding;
        flags = (uint8_t)((flags & ~0x08u) | (authored.flags & 0x08u));
    }
    s_listBoxAuthoredStates.clear();
}

static void LogListBoxHeightInvariant(const char* context, char* listBox, int expectedHeight) {
    if (!s_currentKnobs.itemHeightDiagnostics || !listBox || !is_readable(listBox + 0x36b)) return;
    const int storedHeight = *(int*)(listBox + 0x368);
    if (storedHeight != expectedHeight) {
        fprintf(stderr,
                "[ScaledKotorWidescreen] %s list-box height invariant failed: expected %d, got %d at %p\n",
                context, expectedHeight, storedHeight, (void*)listBox);
    }
}


static int s_lastPatchedCenteringW = -1;
static int s_lastPatchedCenteringH = -1;

/*
  patchMenuCenteringConstants:
  In vanilla KotOR, CSWGuiPanel, CSWGuiWindow, and CSWGuiInGameMap calculate centering
  offsets using hardcoded constants:
    x_offset = (g_uiWidth - 640) / 2
    y_offset = (g_uiHeight - 480) / 2
  When panels are dynamically scaled up to fill the display (e.g. 1309x982 at 1512x982),
  the engine's rendering (CSWGuiPanel::Draw / ScreenToClient), mouse input routines
  (CSWGuiPanel::HandleMouseInput / GetLocalMousePos), and full map viewport
  (CSWGuiInGameMap::Draw / HandleMouseInput) must use the target scaled menu
  dimensions (-targetWidth and -targetHeight) instead of -640 and -480.
  
  Displacements in the Aspyr 64-bit Mac binary:
    Width displacements (-targetW):
      0x10049dcca: CSWGuiPanel::HandleMouseInput (width disp 1)
      0x10049dcd4: CSWGuiPanel::HandleMouseInput (width disp 2)
      0x10049ddb2: CSWGuiPanel::ScreenToClient (width disp 1)
      0x10049ddbc: CSWGuiPanel::ScreenToClient (width disp 2)
      0x10049e855: CSWGuiPanel::GetLocalMousePos (width disp 1)
      0x10049e85f: CSWGuiPanel::GetLocalMousePos (width disp 2)
      0x1002b4b3b: CSWGuiInGameMap::Draw (map viewport X disp 1)
      0x1002b4b46: CSWGuiInGameMap::Draw (map viewport X disp 2)
      0x1002b5615: CSWGuiInGameMap::HandleMouseInput (map mouse X disp 1)
      0x1002b561f: CSWGuiInGameMap::HandleMouseInput (map mouse X disp 2)
    Height displacements (-targetH):
      0x10049dce8: CSWGuiPanel::HandleMouseInput (height disp 1)
      0x10049dcf2: CSWGuiPanel::HandleMouseInput (height disp 2)
      0x10049ddd9: CSWGuiPanel::ScreenToClient (height disp 1)
      0x10049dde3: CSWGuiPanel::ScreenToClient (height disp 2)
      0x10049e872: CSWGuiPanel::GetLocalMousePos (height disp 1)
      0x10049e87c: CSWGuiPanel::GetLocalMousePos (height disp 2)
      0x1002b4b59: CSWGuiInGameMap::Draw (map viewport Y disp 1)
      0x1002b4b63: CSWGuiInGameMap::Draw (map viewport Y disp 2)
      0x1002b5631: CSWGuiInGameMap::HandleMouseInput (map mouse Y disp 1)
      0x1002b563b: CSWGuiInGameMap::HandleMouseInput (map mouse Y disp 2)
*/
void patchMenuCenteringConstants(int targetWidth, int targetHeight) {
    TryAdoptCommittedResolution();
    if (!s_committedResolutionObserved) return;
    if (!ValidateRuntimePatchSites()) return;
    if (s_lastPatchedCenteringW == targetWidth &&
        s_lastPatchedCenteringH == targetHeight) return;

    std::vector<PendingRuntimeWrite> writes;
    writes.reserve(20);
    const auto addInt = [&](uintptr_t address, int value) {
        writes.push_back(MakeIntWrite(address, value));
    };

    addInt(0x10049dcca, -targetWidth);
    addInt(0x10049dcd4, -targetWidth);
    addInt(0x10049ddb2, -targetWidth);
    addInt(0x10049ddbc, -targetWidth);
    addInt(0x10049e855, -targetWidth);
    addInt(0x10049e85f, -targetWidth);
    addInt(0x1002b4b3b, -targetWidth);
    addInt(0x1002b4b46, -targetWidth);
    addInt(0x1002b5615, -targetWidth);
    addInt(0x1002b561f, -targetWidth);

    addInt(0x10049dce8, -targetHeight);
    addInt(0x10049dcf2, -targetHeight);
    addInt(0x10049ddd9, -targetHeight);
    addInt(0x10049dde3, -targetHeight);
    addInt(0x10049e872, -targetHeight);
    addInt(0x10049e87c, -targetHeight);
    addInt(0x1002b4b59, -targetHeight);
    addInt(0x1002b4b63, -targetHeight);
    addInt(0x1002b5631, -targetHeight);
    addInt(0x1002b563b, -targetHeight);

    if (!ApplyRuntimeWriteBatch(writes)) return;
    s_lastPatchedCenteringW = targetWidth;
    s_lastPatchedCenteringH = targetHeight;
}

struct ControlRectSnapshot {
    char* control = nullptr;
    void* vtable = nullptr;
    Rect rect{};
};

static ControlRectSnapshot CaptureControlRectSnapshot(char* control) {
    ControlRectSnapshot snapshot{};
    snapshot.control = control;
    if (control && is_readable(control + 0x17)) {
        snapshot.vtable = *(void**)control;
        snapshot.rect = *(Rect*)(control + 0x8);
    }
    return snapshot;
}

static void SyncControlRectSnapshots(std::vector<ControlRectSnapshot>& snapshots,
                                     char** controls, int controlCount) {
    if (!controls || !is_readable(controls) || controlCount < 0 || controlCount > 1024) {
        snapshots.clear();
        return;
    }
    snapshots.resize((size_t)controlCount);
    for (int i = 0; i < controlCount; ++i) {
        char* current = controls[i];
        ControlRectSnapshot& snapshot = snapshots[(size_t)i];
        void* currentVtable = nullptr;
        if (current && is_readable(current + 0x17)) currentVtable = *(void**)current;
        if (snapshot.control != current || snapshot.vtable != currentVtable) {
            snapshot = CaptureControlRectSnapshot(current);
        }
    }
}

struct PopupSnapshot {
    void* vtable;
    char** controlsIdentity;
    int controlCount;
    Rect root;
    std::vector<ControlRectSnapshot> children;
    uint64_t lastAppliedGeneration;
    Rect messageControl;
    bool messageControlValid;
    uint8_t messageIconFillFlags;
    bool messageIconFillFlagsValid;
};
static std::unordered_map<void*, PopupSnapshot> s_popupSnapshots;
static thread_local char* s_statusSummaryBuilding = nullptr;

static bool HasUsefulRect(const Rect& rect) {
    return rect.width > 0 && rect.height > 0 &&
           rect.width < 8192 && rect.height < 8192;
}

static bool IsIdentityTwoXScale(const UniversalScaleState& scale) {
    return !scale.contentScalingEnabled ||
        (int64_t)scale.contentScaleNumerator * 4 ==
        (int64_t)scale.contentScaleDenominator * 9;
}

// CSWGuiLabel embeds CSWGuiBorder at +0x88. CSWGuiBorder's draw/fill flags are
// the byte at +0x34, so a label's fill-mode bits live at +0xbc.
static uint8_t* GetLabelBorderFlags(char* label) {
    if (!label || !is_readable(label + 0xbc)) return nullptr;
    return reinterpret_cast<uint8_t*>(label + 0xbc);
}

static void ApplyMessageBoxIconFill(PopupSnapshot& snapshot, char* panel,
                                    const UniversalScaleState& scale) {
    char* icon = panel + 0x238;
    uint8_t* flags = GetLabelBorderFlags(icon);
    if (!flags) return;
    if (!snapshot.messageIconFillFlagsValid) {
        snapshot.messageIconFillFlags = *flags;
        snapshot.messageIconFillFlagsValid = true;
    }
    const uint8_t fillMode = IsIdentityTwoXScale(scale) ?
        (snapshot.messageIconFillFlags & 0x03u) : 0x02u;
    *flags = (uint8_t)((*flags & ~0x03u) | fillMode);
}

static void RestorePopupSnapshot(char* panel, const PopupSnapshot& snapshot) {
    if (!panel || !is_readable(panel) || *(void**)panel != snapshot.vtable) return;
    SetControlRect(panel, snapshot.root);
    char* border = *(char**)(panel + 0x70);
    if (border && is_readable(border)) {
        SetControlRect(border, { 0, 0, snapshot.root.width, snapshot.root.height });
    }
    char** controls = *(char***)(panel + 0x30);
    const int controlCount = *(int*)(panel + 0x38);
    if (controls && controls == snapshot.controlsIdentity && is_readable(controls) &&
        controlCount == snapshot.controlCount && controlCount > 0 && controlCount <= 1024) {
        const int count = std::min(controlCount, (int)snapshot.children.size());
        for (int i = 0; i < count; ++i) {
            const ControlRectSnapshot& child = snapshot.children[(size_t)i];
            if (controls[i] == child.control && child.control && is_readable(child.control) &&
                *(void**)child.control == child.vtable && HasUsefulRect(child.rect)) {
                SetControlRect(child.control, child.rect);
            }
        }
    }
    if (snapshot.messageIconFillFlagsValid) {
        uint8_t* flags = GetLabelBorderFlags(panel + 0x238);
        if (flags) *flags = snapshot.messageIconFillFlags;
    }
}

/*
  scalePopupPanel:
  Applies the Windows shared popup formulas from immutable panel snapshots.
  Container roots use contentScale*8/9; the remaining popup family uses the compact
  contentScale*4/9 transform. Message-box and status-summary late layout is repaired
  by focused hooks below rather than by consuming already-scaled rectangles.
*/
void scalePopupPanel(char* panel) {
    if (!panel || !is_readable(panel)) return;

    refreshPatchedListConstants();

    Rect* rect = reinterpret_cast<Rect*>(panel + 0x8);
    if (!HasUsefulRect(*rect)) return;

    void* vtable = *(void**)panel;
    const bool isContainer = vtable == (void*)0x1005ab758;
    const bool isMessageBox = vtable == (void*)0x1005ae880;
    const bool isStatusSummary = vtable == (void*)0x1005ae9a0;
    const bool isBark = vtable == (void*)0x1005ad420;
    const bool isPause = vtable == (void*)0x1005ad640;

    // OnPanelAdded invokes the common panel finalizer before it finishes creating
    // status rows. Suppress that mid-build callback; the epilogue hook captures the
    // complete native result and scales it once.
    if (isStatusSummary && s_statusSummaryBuilding == panel) return;

    const UniversalScaleState universalScale = GetUniversalScaleState();
    const float explicitMenuScale = ReadIniMenuScale();
    const auto scaleValue = [&](int value) -> int {
        if (isContainer) return ScaleContainerValue(value, universalScale);
        if (explicitMenuScale > 0.0f) {
            return (int)std::lround(value * explicitMenuScale);
        }
        return ScaleTwoXValue(value, universalScale);
    };

    const int popupControlCount = *(int*)(panel + 0x38);
    char** popupControls = *(char***)(panel + 0x30);
    auto it = s_popupSnapshots.find(panel);
    if (it != s_popupSnapshots.end() && it->second.vtable != vtable) {
        s_popupSnapshots.erase(it);
        it = s_popupSnapshots.end();
    }

    if (it == s_popupSnapshots.end()) {
        PopupSnapshot snap{};
        snap.vtable = vtable;
        snap.controlsIdentity = popupControls;
        snap.controlCount = popupControlCount;
        snap.root = *rect;
        snap.lastAppliedGeneration = 0;

        if (isMessageBox && is_readable(panel + 0x1c7)) {
            const Rect messageRect = *(Rect*)(panel + 0x1b8);
            if (HasUsefulRect(messageRect)) {
                snap.messageControl = messageRect;
                snap.messageControlValid = true;
            }
            uint8_t* flags = GetLabelBorderFlags(panel + 0x238);
            if (flags) {
                snap.messageIconFillFlags = *flags;
                snap.messageIconFillFlagsValid = true;
            }
        }

        if (popupControls && is_readable(popupControls) &&
            popupControlCount > 0 && popupControlCount <= 1024) {
            SyncControlRectSnapshots(snap.children, popupControls, popupControlCount);
        }
        if (it == s_popupSnapshots.end()) {
            it = s_popupSnapshots.emplace(panel, std::move(snap)).first;
        } else {
            it->second = std::move(snap);
        }
    }

    PopupSnapshot& snap = it->second;
    snap.controlsIdentity = popupControls;
    snap.controlCount = popupControlCount;
    SyncControlRectSnapshots(snap.children, popupControls, popupControlCount);
    const int targetW = std::max(1, scaleValue(snap.root.width));
    const int targetH = std::max(1, scaleValue(snap.root.height));
    const int scaledLeft = scaleValue(snap.root.left);
    const int scaledTop = scaleValue(snap.root.top);

    int targetLeft = (g_targetWidth - targetW) / 2;
    int targetTop = (g_targetHeight - targetH) / 2;
    if (isBark) {
        targetTop = scaledTop;
    } else if (isPause) {
        targetLeft = scaledLeft;
        targetTop = scaledTop;
    }

    if (snap.lastAppliedGeneration == g_layoutGeneration &&
        rect->left == targetLeft && rect->top == targetTop &&
        rect->width == targetW && rect->height == targetH) {
        return;
    }

    const Rect scaledRoot = { targetLeft, targetTop, targetW, targetH };
    SetControlRect(panel, scaledRoot);

    char* border = *(char**)(panel + 0x70);
    if (border && is_readable(border)) {
        SetControlRect(border, { 0, 0, targetW, targetH });
    }

    const int numControls = *(int*)(panel + 0x38);
    char** controls = *(char***)(panel + 0x30);
    if (controls && is_readable(controls) && numControls > 0) {
        const int count = std::min((int)snap.children.size(), numControls);
        for (int i = 0; i < count; ++i) {
            const ControlRectSnapshot& child = snap.children[(size_t)i];
            if (!child.control || controls[i] != child.control || !is_readable(child.control) ||
                *(void**)child.control != child.vtable) continue;
            const Rect& authored = child.rect;
            if (!HasUsefulRect(authored)) continue;

            const Rect scaled = {
                scaleValue(authored.left),
                scaleValue(authored.top),
                std::max(1, scaleValue(authored.width)),
                std::max(1, scaleValue(authored.height)),
            };

            void* ctrlVtable = child.vtable;
            if (ctrlVtable == (void*)0x1005b4318) {
                if (*(int*)(child.control + 0x168) != 0) {
                    *(int*)(child.control + 0x168) =
                        std::max(1, ScaleMenuUiValue(16));
                }
                if (isContainer) {
                    const ScaledItemGeometry popupGeom = GetScaledItemGeometry();
                    ConfigureListBoxPadding(child.control, popupGeom.containerPadding);
                }
            }
            SetControlRect(child.control, scaled);
        }
    }

    if (isMessageBox) {
        ApplyMessageBoxIconFill(snap, panel, universalScale);
        // FixMessageLabel copies these two base rectangles at function entry.
        *(Rect*)(panel + 0xbf0) = scaledRoot;
        if (snap.messageControlValid) {
            *(Rect*)(panel + 0xc00) = {
                scaleValue(snap.messageControl.left),
                scaleValue(snap.messageControl.top),
                std::max(1, scaleValue(snap.messageControl.width)),
                std::max(1, scaleValue(snap.messageControl.height)),
            };
        }
    }

    panel[0x5c] = (panel[0x5c] & ~0x60) | 0x1;
    snap.lastAppliedGeneration = g_layoutGeneration;
}

static void ScaleMessageBoxButtonWidth(char* box, ptrdiff_t offset) {
    char* button = box + offset;
    if (!is_readable(button) || !is_readable(button + 0x17)) return;
    Rect rect = *(Rect*)(button + 0x8);
    if (!HasUsefulRect(rect)) return;

    const int centerX = rect.left + rect.width / 2;
    int width = std::max(1, ScaleMenuContentValue(100));
    const Rect root = *(Rect*)(box + 0x8);
    const int margin = std::max(0, rect.left);
    const int maxWidth = root.width - margin * 2;
    if (maxWidth > 0) width = std::min(width, maxWidth);
    rect.width = width;
    rect.left = centerX - width / 2;
    SetControlRect(button, rect);
}

// CSWGuiMessageBox::FixMessageLabel has completed its text fitting and final button
// setters. Repair only the values that the native function overwrites late.
extern "C" void Hook_MessageBoxPostFit(char* box) {
    BEGIN_HOOK();
    TryAdoptCommittedResolution();
    if (!s_committedResolutionObserved || !box || !is_readable(box + 0xc0f)) return;
    RefreshRuntimeConfiguration();
    if (*(void**)box != (void*)0x1005ae880) return;

    const UniversalScaleState scale = GetUniversalScaleState();
    Rect root = *(Rect*)(box + 0x8);
    if (HasUsefulRect(root)) {
        root.left = (g_targetWidth - root.width) / 2;
        root.top = (g_targetHeight - root.height) / 2;
        SetControlRect(box, root);
        char* border = *(char**)(box + 0x70);
        if (border && is_readable(border)) {
            SetControlRect(border, { 0, 0, root.width, root.height });
        }
        *(Rect*)(box + 0xbf0) = root;
    }

    ScaleMessageBoxButtonWidth(box, 0x3d0);
    ScaleMessageBoxButtonWidth(box, 0x610);

    auto it = s_popupSnapshots.find(box);
    if (it != s_popupSnapshots.end()) {
        ApplyMessageBoxIconFill(it->second, box, scale);
        if (it->second.messageControlValid) {
            const auto scaleValue = [&](int value) {
                return ScaleTwoXValue(value, scale);
            };
            *(Rect*)(box + 0xc00) = {
                scaleValue(it->second.messageControl.left),
                scaleValue(it->second.messageControl.top),
                std::max(1, scaleValue(it->second.messageControl.width)),
                std::max(1, scaleValue(it->second.messageControl.height)),
            };
        }
        it->second.lastAppliedGeneration = g_layoutGeneration;
    }
}

// Restore the previous authored status-summary baseline before the native routine
// repopulates dynamic rows. Its panel-finalizer callback is suppressed until the epilogue.
extern "C" void Hook_StatusSummaryBeforeBuild(char* owner) {
    BEGIN_HOOK();
    s_statusSummaryBuilding = owner;
    if (!owner) return;
    auto it = s_popupSnapshots.find(owner);
    if (it != s_popupSnapshots.end()) {
        RestorePopupSnapshot(owner, it->second);
        s_popupSnapshots.erase(it);
    }
}

extern "C" void Hook_StatusSummaryPostBuild(char* framePointer) {
    BEGIN_HOOK();
    char* owner = nullptr;
    if (framePointer && is_readable(framePointer - 0x78)) {
        owner = *(char**)(framePointer - 0x78);
    }
    s_statusSummaryBuilding = nullptr;
    TryAdoptCommittedResolution();
    if (!s_committedResolutionObserved || !owner || !is_readable(owner) ||
        *(void**)owner != (void*)0x1005ae9a0) return;
    RefreshRuntimeConfiguration();
    s_popupSnapshots.erase(owner);
    scalePopupPanel(owner);
}

// Pazaak hand cards are CSWGuiPazaakCard composites embedded directly in the
// CSWGuiPazaakGame object. RefreshDisplay updates the native card state first; the
// focused call-site hook below applies the final scaled extents only after the last
// native SetEnabled call has completed. This avoids mutating the composites at Draw entry.
struct PazaakCardLayout {
    ptrdiff_t offset;
    Rect card;
    Rect label;
};

static const PazaakCardLayout kPazaakHandCards[] = {
    { 0x3a40 + 0x3f0 * 0, {  94, 425, 80, 80 }, { 109, 433, 50, 50 } },
    { 0x3a40 + 0x3f0 * 1, { 161, 425, 80, 80 }, { 176, 433, 50, 50 } },
    { 0x3a40 + 0x3f0 * 2, { 229, 425, 80, 80 }, { 244, 433, 50, 50 } },
    { 0x3a40 + 0x3f0 * 3, { 296, 425, 80, 80 }, { 311, 433, 50, 50 } },
    { 0x6d70 + 0x3f0 * 0, { 421, 425, 80, 80 }, { 436, 433, 50, 50 } },
    { 0x6d70 + 0x3f0 * 1, { 489, 425, 80, 80 }, { 505, 433, 50, 50 } },
    { 0x6d70 + 0x3f0 * 2, { 556, 425, 80, 80 }, { 571, 433, 50, 50 } },
    { 0x6d70 + 0x3f0 * 3, { 625, 425, 80, 80 }, { 640, 433, 50, 50 } },
};

static Rect ScalePazaakRect(const Rect& source) {
    int targetW = 640;
    int targetH = 480;
    GetMenuCanvasSize(640, 480, targetW, targetH);
    return {
        DivideRoundedNearest((int64_t)source.left * targetW, 800),
        DivideRoundedNearest((int64_t)source.top * targetH, 600),
        std::max(1, DivideRoundedNearest((int64_t)source.width * targetW, 800)),
        std::max(1, DivideRoundedNearest((int64_t)source.height * targetH, 600)),
    };
}

static void ApplyPazaakHandGeometry(char* game) {
    if (!game || !is_readable(game + 0x7d2f)) return;
    using ButtonSetExtentFn = void (*)(void*, const Rect*);
    using LabelSetExtentFn = void (*)(void*, const Rect*);
    const ButtonSetExtentFn setButtonExtent =
        reinterpret_cast<ButtonSetExtentFn>(0x1004a5adc);
    const LabelSetExtentFn setLabelExtent =
        reinterpret_cast<LabelSetExtentFn>(0x1004a56f0);

    for (const PazaakCardLayout& layout : kPazaakHandCards) {
        char* card = game + layout.offset;
        if (!is_readable(card + 0x3ef)) continue;
        const Rect cardRect = ScalePazaakRect(layout.card);
        const Rect labelRect = ScalePazaakRect(layout.label);
        setButtonExtent(card, &cardRect);
        setLabelExtent(card + 0x240, &labelRect);
        SetBorderFillStretch(card + 0xa8);
        SetBorderFillStretch(card + 0x130);
    }
}

// Replaces the final CSWGuiActivatedButton::SetEnabled call in RefreshDisplay.
// The incoming button is game+0x8bb0; call the native target exactly once, then
// recover the owning game object and apply the completed hand geometry.
extern "C" void Hook_PazaakRefreshDisplayComplete(char* finalButton, int enabled) {
    BEGIN_HOOK();
    using SetEnabledFn = void (*)(void*, int);
    reinterpret_cast<SetEnabledFn>(0x10021cff4)(finalButton, enabled);
    if (!finalButton) return;

    char* game = finalButton - 0x8bb0;
    TryAdoptCommittedResolution();
    if (!s_committedResolutionObserved || !is_readable(game)) return;
    RefreshRuntimeConfiguration();
    ApplyPazaakHandGeometry(game);
}

/*
  scaleMenuPanelTree:
  Dynamically scales in-game menu panels (CSWGuiInGameMenu container, Equip, Inventory,
  Character, Abilities/Skills, Journal, Map, Messages, Options), Save & Load Game screens
  (CSWGuiSaveLoad), and Main Menu screens.
  
  Port of WindowsScaledKotor "Scaled Menu" (scaled-menu-v1) patch to macOS 64-bit AMD64.
  
  Mechanics:
  1. Popups (containers, message boxes, pause dialogs) are dispatched directly to scalePopupPanel.
  2. Inspects the panel's root rectangle. Matches standard 4:3 panels (640x480, 800x600, etc.),
     top tab bars, and known menu vtables (including CSWGuiSaveLoad, CSWGuiPartySelection, etc.).
  3. Calculates the scaled dimensions (uiHeight = screenHeight, uiWidth = screenHeight * 4 / 3,
     or user-configured MenuScale in swkotor.ini).
  4. Dynamically patches the engine's 12 centering displacements via patchMenuCenteringConstants(targetW, targetH).
  5. Resizes the panel's root extent and border to { 0, 0, targetW, targetH }.
  6. Scales every child control (buttons, labels, text boxes, portraits, list containers, scrollbars)
     using nearest-integer proportional scaling.
  7. Centering flags at panel + 0x5c:
     - Top-level windows (CSWGuiInGameMenu, CSWGuiMainMenu, CSWGuiSaveLoad, etc.) receive flags
       0x20 | 0x40 (0x60). CSWGuiPanel::Draw centers them at (screenWidth - targetWidth)/2,
       and HandleMouseInput / GetLocalMousePos subtract (screenWidth - targetWidth)/2.
     - Embedded child tab screens (Inventory, Equip, Character) have 0x60 cleared and bit 0x1 set
       so ScreenToClient does not subtract centering a second time.
     Result: 100% pixel-perfect mouse click hit testing across all menus and dialogs!
*/
/*
  getScaledClassSlotRect:
  Computes pixel-perfect widescreen slot geometry for the 6 class character models
  and selection buttons on the Character Generation screen (CSWGuiClassSelection / classsel.gui).
  Port of WindowsScaledKotor "Scaled Class Menu".
*/
Rect getScaledClassSlotRect(int slotIndex, int targetW, int targetH) {
    static constexpr int kBaseWidth = 800;
    static constexpr int kBaseHeight = 600;
    static constexpr int kBaseBoxWidth = 94;
    static constexpr int kBaseBoxHeight = 240;
    static constexpr int kBaseLefts[6] = { 77, 190, 299, 406, 516, 625 };

    if (slotIndex < 0 || slotIndex >= 6 || targetW <= 0 || targetH <= 0) {
        return { 0, 0, 0, 0 };
    }

    const int boxW = DivideRoundedNearest(
        (int64_t)kBaseBoxWidth * targetW, kBaseWidth);
    const int boxH = DivideRoundedNearest(
        (int64_t)kBaseBoxHeight * targetH, kBaseHeight);
    const int left = DivideRoundedNearest(
        (int64_t)kBaseLefts[slotIndex] * targetW, kBaseWidth);

    return {
        left,
        (targetH - boxH) / 2,
        boxW,
        boxH,
    };
}

struct SmallPanelSnapshot {
    void* vtable;
    char** controlsIdentity;
    int controlCount;
    Rect root;
    struct ChildSnap {
        char* control = nullptr;
        void* vtable = nullptr;
        Rect rect{};
    };
    std::vector<ChildSnap> children;
};

static std::unordered_map<void*, SmallPanelSnapshot> s_smallPanelSnapshots;

/*
  scaleSmallChargenPanel:
  Dynamically scales the 4 small independent chargen / level-up root panels:
    - CSWGuiQuickOrCustomPanel (qorcpnl)
    - CSWGuiCustomPanel (custpnl)
    - CSWGuiQuickPanel (quickpnl)
    - CSWGuiLevelUpCharGen (leveluppnl)
  Port of WindowsScaledKotor "Scaled Panels" (scaled-panels-v1) patch.
  
  In vanilla KotOR (640x480), these panels are authored on the right side of the screen
  (e.g. left ~322, top ~87, width ~267-273, height ~275-281).
  We scale their root bounds and child controls proportionally using the 640x480 basis,
  and retain flag 0x60 so the engine's 4:3 menu centering displacement positions them
  pixel-perfectly on the right half of the character console!
*/
void scaleSmallChargenPanel(char* panel) {
    if (!panel || !is_readable(panel)) return;
    
    void* vtable = *(void**)panel;
    Rect* rect = (Rect*)(panel + 0x8);
    if (!rect || rect->width <= 0 || rect->height <= 0 || rect->width >= 8192 || rect->height >= 8192) {
        return;
    }
    
    // Target dimensions use the same shared 640x480 UI canvas as the Windows provider.
    int targetW = 640;
    int targetH = 480;
    GetMenuCanvasSize(640, 480, targetW, targetH);
    
    // Capture immutable authored geometry for each panel/control-array identity.
    auto it = s_smallPanelSnapshots.find(panel);
    char** smallControls = *(char***)(panel + 0x30);
    int smallControlCount = *(int*)(panel + 0x38);
    if (it != s_smallPanelSnapshots.end() && it->second.vtable != vtable) {
        s_smallPanelSnapshots.erase(it);
        it = s_smallPanelSnapshots.end();
    }
    if (it == s_smallPanelSnapshots.end()) {
        SmallPanelSnapshot snap;
        snap.vtable = vtable;
        snap.controlsIdentity = *(char***)(panel + 0x30);
        snap.controlCount = *(int*)(panel + 0x38);
        snap.root = *rect;
        
        int numControls = *(int*)(panel + 0x38);
        char** controls = *(char***)(panel + 0x30);
        if (controls && is_readable(controls) && numControls > 0 && numControls <= 1024) {
            snap.children.resize((size_t)numControls);
            for (int i = 0; i < numControls; ++i) {
                char* ctrl = controls[i];
                if (ctrl && is_readable(ctrl + 0x17)) {
                    snap.children[(size_t)i] = { ctrl, *(void**)ctrl, *(Rect*)(ctrl + 0x8) };
                }
            }
        }
        s_smallPanelSnapshots[panel] = snap;
        it = s_smallPanelSnapshots.find(panel);
    }
    
    SmallPanelSnapshot& snap = it->second;
    snap.controlsIdentity = smallControls;
    snap.controlCount = smallControlCount;
    if (smallControls && is_readable(smallControls) && smallControlCount >= 0 && smallControlCount <= 1024) {
        snap.children.resize((size_t)smallControlCount);
        for (int i = 0; i < smallControlCount; ++i) {
            char* current = smallControls[i];
            void* currentVtable = current && is_readable(current + 0x17) ? *(void**)current : nullptr;
            SmallPanelSnapshot::ChildSnap& child = snap.children[(size_t)i];
            if (child.control != current || child.vtable != currentVtable) {
                child = {};
                if (current && currentVtable) {
                    child.control = current;
                    child.vtable = currentVtable;
                    child.rect = *(Rect*)(current + 0x8);
                }
            }
        }
    }
    
    Rect scaledRoot = {
        DivideRoundedNearest((int64_t)snap.root.left * targetW, 640),
        DivideRoundedNearest((int64_t)snap.root.top * targetH, 480),
        DivideRoundedNearest((int64_t)snap.root.width * targetW, 640),
        DivideRoundedNearest((int64_t)snap.root.height * targetH, 480),
    };
    if (rect->left == scaledRoot.left && rect->top == scaledRoot.top &&
        rect->width == scaledRoot.width && rect->height == scaledRoot.height) {
        return;
    }

    // Scale root extent from the immutable authored snapshot.
    SetControlRect(panel, scaledRoot);
    
    // Scale glowing window border at panel + 0x70
    char* border = *(char**)(panel + 0x70);
    if (border && is_readable(border)) {
        Rect borderRect = { 0, 0, scaledRoot.width, scaledRoot.height };
        SetControlRect(border, borderRect);
    }
    
    // Scale all child controls from snapshot
    for (size_t index = 0; index < snap.children.size(); ++index) {
        const SmallPanelSnapshot::ChildSnap& childSnap = snap.children[index];
        if (!childSnap.control || !smallControls || index >= (size_t)smallControlCount ||
            smallControls[index] != childSnap.control || !is_readable(childSnap.control) ||
            *(void**)childSnap.control != childSnap.vtable) continue;
        Rect s = {
            DivideRoundedNearest((int64_t)childSnap.rect.left * targetW, 640),
            DivideRoundedNearest((int64_t)childSnap.rect.top * targetH, 480),
            DivideRoundedNearest((int64_t)childSnap.rect.width * targetW, 640),
            DivideRoundedNearest((int64_t)childSnap.rect.height * targetH, 480),
        };

        void* ctrlVtable = *(void**)childSnap.control;
        if (ctrlVtable == (void*)0x1005b4318 &&
            *(int*)(childSnap.control + 0x168) != 0) {
            *(int*)(childSnap.control + 0x168) = std::max(
                1, DivideRoundedNearest((int64_t)16 * targetW, 640));
        }
        SetControlRect(childSnap.control, s);
    }
    
    // Retain 0x60 centering flag so the 4:3 displacement is added to its screen draw position
    panel[0x5c] = (panel[0x5c] & ~0x09) | 0x60;
}

static void RestoreVariableHeightMessageList(char* listBox) {
    if (!listBox || !is_readable(listBox + 0x373)) return;
    *(uint8_t*)(listBox + 0x370) &= (uint8_t)~0x08u;
    *(int*)(listBox + 0x368) = 0;
}

static std::unordered_map<char*, uint64_t> s_messageReflowGenerations;
static thread_local bool s_reflowingMessages = false;

static void RestoreAllMessageLists(char* panel) {
    if (!panel || !is_readable(panel + 0x38)) return;
    char** controls = *(char***)(panel + 0x30);
    const int count = *(int*)(panel + 0x38);
    if (!controls || !is_readable(controls) || count <= 0 || count > 1024) return;
    for (int i = 0; i < count; ++i) {
        char* control = controls[i];
        if (control && is_readable(control) &&
            *(void**)control == (void*)0x1005b4318) {
            RestoreVariableHeightMessageList(control);
        }
    }
}

static void ReflowMessagesForGeneration(char* panel) {
    if (!panel || s_reflowingMessages) return;
    const auto it = s_messageReflowGenerations.find(panel);
    if (it != s_messageReflowGenerations.end() && it->second == g_layoutGeneration) return;

    // Mark before calling Show because the native repopulation path can re-enter panel layout.
    s_messageReflowGenerations[panel] = g_layoutGeneration;
    s_reflowingMessages = true;
    reinterpret_cast<void (*)(void*)>(0x100305ae2)(panel);
    s_reflowingMessages = false;
    RestoreAllMessageLists(panel);
}

struct MenuChildSnapshot {
    char* control = nullptr;
    void* vtable = nullptr;
    Rect vanillaRect{};
    int vanillaScrollW = 0;
};

struct MenuPanelSnapshot {
    void* vtable;
    char** controlsIdentity;
    int controlCount;
    int baseW;
    int baseH;
    int lastKnobVersion;
    Rect vanillaRoot;
    std::vector<MenuChildSnapshot> children;
};
static std::unordered_map<void*, MenuPanelSnapshot> s_menuPanelSnapshots;

void scaleMenuPanelTree(char* panel) {
    TryAdoptCommittedResolution();
    if (!s_committedResolutionObserved || !panel || !is_readable(panel)) return;
    if (panel == g_lastScaledHud) return; // HUD interface has its own custom anchoring
    
    refreshPatchedListConstants();
    
    void* vtable = *(void**)panel;
    if (vtable == (void*)0x1005d3210) return; // Tooltip panel (hidden coordinate canvas)
    if (isMainInterfacePanel(vtable)) return; // Gameplay HUD has its own layout path.
    
    // Resolution changes invalidate rendered state, but authored snapshots remain valid
    // for the lifetime of the same panel/control-array identity. Retaining them prevents
    // a live mode change from treating already-scaled rectangles as new source geometry.
    if (s_lastScaleGeneration != g_resolutionGeneration ||
        s_lastScaleTargetHeight != g_targetHeight) {
        RestoreAndClearTrackedListBoxes();
        s_scaledPanels.clear();
        s_lastScaleTargetHeight = g_targetHeight;
        s_lastScaleGeneration = g_resolutionGeneration;
    }
    
    // Handle Small Chargen Panels (qorcpnl, custpnl, quickpnl, leveluppnl)
    if (isSmallChargenPanel(vtable)) {
        scaleSmallChargenPanel(panel);
        return;
    }
    
    // Bark bubbles remain HUD-relative rather than generic centered popups.
    if (vtable == (void*)0x1005ad420) {
        PositionBarkBubble(panel);
        return;
    }

    // Handle Popups (containers, message boxes, pause dialogs, etc.)
    if (isPopupPanel(vtable)) {
        scalePopupPanel(panel);
        return;
    }
    
    Rect* rect = (Rect*)(panel + 0x8);
    if (!rect || rect->width <= 0 || rect->height <= 0 || rect->width >= 8192 || rect->height >= 8192) {
        return;
    }
    
    // CSWGuiFade panel: scale to fill entire screen
    if (vtable == (void*)0x1005a90e0) {
        if (s_scaledPanels.find(panel) != s_scaledPanels.end()) {
            if (rect->width == g_targetWidth && rect->height == g_targetHeight) {
                return;
            }
            s_scaledPanels.erase(panel);
        }
        Rect fullscreen = { 0, 0, g_targetWidth, g_targetHeight };
        SetControlRect(panel, fullscreen);
        s_scaledPanels.insert(panel);
        return;
    }
    
    int origW = rect->width;
    int origH = rect->height;
    
    const bool is43 = rect->left == 0 && rect->top == 0 &&
        origW >= 640 && origH >= 480 && origW * 3 == origH * 4;
    const bool isTopTab = rect->left == 0 && rect->top == 0 && origW >= 640 &&
        std::abs((long long)origH * 640 - (long long)origW * 86) <= 640;
    const bool fullscreenMainMenu = isMainMenuPanel(vtable) &&
        rect->left == 0 && rect->top == 0;
    if (!is43 && !isTopTab && !fullscreenMainMenu) return;
    
    int baseW = origW;
    int baseH = origH;
    if (isTopTab) {
        baseH = (origW * 3) / 4;
    }
    
    auto snapIt = s_menuPanelSnapshots.find(panel);
    char** currentControls = *(char***)(panel + 0x30);
    int currentControlCount = *(int*)(panel + 0x38);
    const bool staleSnapshot = snapIt != s_menuPanelSnapshots.end() &&
        snapIt->second.vtable != vtable;
    if (staleSnapshot) {
        s_menuPanelSnapshots.erase(snapIt);
        s_scaledPanels.erase(panel);
        snapIt = s_menuPanelSnapshots.end();
    }
    if (snapIt == s_menuPanelSnapshots.end()) {
        MenuPanelSnapshot snap;
        snap.vtable = vtable;
        snap.controlsIdentity = *(char***)(panel + 0x30);
        snap.controlCount = *(int*)(panel + 0x38);
        snap.baseW = baseW;
        snap.baseH = baseH;
        snap.lastKnobVersion = s_knobVersion;
        snap.vanillaRoot = *rect;
        
        int numControls = *(int*)(panel + 0x38);
        char** controls = *(char***)(panel + 0x30);
        if (controls && is_readable(controls) && numControls > 0 && numControls <= 1024) {
            snap.children.resize((size_t)numControls);
            for (int i = 0; i < numControls; ++i) {
                char* ctrl = controls[i];
                if (!ctrl || !is_readable(ctrl + 0x17)) continue;
                const void* ctrlVtable = *(void**)ctrl;
                int scrollW = 0;
                if (ctrlVtable == (void*)0x1005b4318) scrollW = *(int*)(ctrl + 0x168);
                snap.children[(size_t)i] = { ctrl, (void*)ctrlVtable, *(Rect*)(ctrl + 0x8), scrollW };
            }
        }
        s_menuPanelSnapshots[panel] = snap;
        snapIt = s_menuPanelSnapshots.find(panel);
    }

    snapIt->second.controlsIdentity = currentControls;
    snapIt->second.controlCount = currentControlCount;
    if (currentControls && is_readable(currentControls) &&
        currentControlCount >= 0 && currentControlCount <= 1024) {
        snapIt->second.children.resize((size_t)currentControlCount);
        for (int i = 0; i < currentControlCount; ++i) {
            char* current = currentControls[i];
            void* currentVtable = current && is_readable(current + 0x17) ? *(void**)current : nullptr;
            MenuChildSnapshot& child = snapIt->second.children[(size_t)i];
            if (child.control != current || child.vtable != currentVtable) {
                child = {};
                if (current && currentVtable) {
                    child.control = current;
                    child.vtable = currentVtable;
                    child.vanillaRect = *(Rect*)(current + 0x8);
                    if (currentVtable == (void*)0x1005b4318) {
                        child.vanillaScrollW = *(int*)(current + 0x168);
                    }
                }
            }
        }
    }

    baseW = snapIt->second.baseW;
    baseH = snapIt->second.baseH;
    int targetW = baseW;
    int targetH = baseH;
    if (fullscreenMainMenu) {
        targetW = g_targetWidth;
        targetH = g_targetHeight;
    } else {
        GetMenuCanvasSize(baseW, baseH, targetW, targetH);
    }

    const Rect& authoredRoot = snapIt->second.vanillaRoot;
    const Rect scaledRoot = {
        DivideRoundedNearest((int64_t)authoredRoot.left * targetW, baseW),
        DivideRoundedNearest((int64_t)authoredRoot.top * targetH, baseH),
        std::max(1, DivideRoundedNearest((int64_t)authoredRoot.width * targetW, baseW)),
        std::max(1, DivideRoundedNearest((int64_t)authoredRoot.height * targetH, baseH)),
    };

    if (s_scaledPanels.find(panel) != s_scaledPanels.end() &&
        snapIt->second.lastKnobVersion == s_knobVersion) {
        if (rect->left == scaledRoot.left && rect->top == scaledRoot.top &&
            rect->width == scaledRoot.width && rect->height == scaledRoot.height) {
            return;
        }
        s_scaledPanels.erase(panel);
    }
    snapIt->second.lastKnobVersion = s_knobVersion;

    float scale = (snapIt->second.baseH > 0) ?
        ((float)targetH / (float)snapIt->second.baseH) : 1.0f;

    // Centered 4:3 roots share one process-wide canvas. The fullscreen main menu
    // is client-relative and therefore does not depend on these displacements.
    if (!fullscreenMainMenu) {
        patchMenuCenteringConstants(targetW, targetH);
    }

    // 1. Scale the authored panel root. A top-tab root keeps its short authored
    // height instead of being expanded to the full 4:3 canvas.
    SetControlRect(panel, scaledRoot);
    
    // 2. Scale panel border (offset +0x70 in 64-bit Mac)
    char* border = *(char**)(panel + 0x70);
    if (border && is_readable(border)) {
        SetControlRect(border, scaledRoot);
    }
    
    // 3. Scale all child controls from snapshot
    for (size_t childIndex = 0; childIndex < snapIt->second.children.size(); ++childIndex) {
        const MenuChildSnapshot& childSnap = snapIt->second.children[childIndex];
        char* ctrl = childSnap.control;
        if (!ctrl || !currentControls || childIndex >= (size_t)currentControlCount ||
            currentControls[childIndex] != ctrl || !is_readable(ctrl) ||
            *(void**)ctrl != childSnap.vtable) continue;
        const Rect& r = childSnap.vanillaRect;
        if (r.width > 0 && r.height > 0 && r.width < 8192 && r.height < 8192) {
            Rect s;
            s.left   = (int)(((long long)r.left   * targetW + snapIt->second.baseW / 2) / snapIt->second.baseW);
            s.top    = (int)(((long long)r.top    * targetH + snapIt->second.baseH / 2) / snapIt->second.baseH);
            s.width  = (int)(((long long)r.width  * targetW + snapIt->second.baseW / 2) / snapIt->second.baseW);
            s.height = (int)(((long long)r.height * targetH + snapIt->second.baseH / 2) / snapIt->second.baseH);
            
            // Save & Load Game Screen (0x1005ae300): calibrate LB_GAMES (width 272)
            // so save selection boxes match the 6 background slots pixel-for-pixel
            if (vtable == (void*)0x1005ae300 && r.width == 272) {
                s.left -= (int)(0.5f * scale + 0.5f);   // 1px left at 982p (aligns left edge flush with slot at X=22)
                s.width += (int)(1.0f * scale + 0.5f);  // widens by 2px at 982p (aligns right edge flush with slot at X=794)
                s.top -= (int)(2.8f * scale + 0.5f);    // 6px up at 982p (seats top save entry flush with top background slot at Y=18 with 19px padding)
                s.height = (int)(327.5f * scale + 0.5f); // 670px client height at 982p (produces 108.5px stride and 140px box height)
            }
            // Inventory/equipment list box: only list-container offsets are scaled
            // here. Natural item height is measured by CSWGuiListBox from rows built
            // with the immutable Windows-parity constructor values.
            const ScaledItemGeometry geom = GetScaledItemGeometry();
            const bool isInventoryList =
                (vtable == (void*)0x1005a75c0 || vtable == (void*)0x1005ab508) &&
                (r.height == 294 || r.width == 269 || r.width == 270);
            if (isInventoryList) {
                s.left += geom.listLeftOffset;
                s.width += geom.listWidthOffset;
                s.top += geom.listTopOffset;
                if (geom.listHeight > 0) s.height = geom.listHeight;
            }

            void* ctrlVtable = *(void**)ctrl;
            const bool isMessagesList =
                vtable == (void*)0x1005ae790 && ctrlVtable == (void*)0x1005b4318;
            if (ctrlVtable == (void*)0x1005b4318) {
                if (childSnap.vanillaScrollW != 0) {
                    *(int*)(ctrl + 0x168) = std::max(1, ScaleMenuUiValue(16));
                }
                if (vtable == (void*)0x1005ae300) {
                    // Save/load is outside the Windows Scaled List Items module.
                    *(uint8_t*)(ctrl + 0x373) = (uint8_t)ClampValue(
                        (int)(9.3f * scale + 0.5f), 0, 255);
                }
                if (isInventoryList) {
                    ConfigureListBoxPadding(ctrl, geom.itemPadding);
                }
                if (isMessagesList) {
                    RestoreVariableHeightMessageList(ctrl);
                }
            }
            SetControlRect(ctrl, s);
            if (isMessagesList) {
                // CSWGuiListBox::SetExtent can restore fixed-height mode; clear it again.
                RestoreVariableHeightMessageList(ctrl);
            }

            // pazaakgame.gui authors one player-side card with centered rather than stretched
            // border artwork. Apply the same focused correction to normal and highlight states.
            if (vtable == (void*)0x1005a5b80 &&
                r.left == 129 && r.top == 340 && r.width == 64 && r.height == 64) {
                SetBorderFillStretch(ctrl + 0xa8);
                SetBorderFillStretch(ctrl + 0x130);
            }

            if (ctrlVtable == (void*)0x1005b4318 && isInventoryList) {
                const int childCount = *(int*)(ctrl + 0x350);
                char** items = *(char***)(ctrl + 0x348);
                if (items && is_readable(items) && childCount > 0 && childCount <= 1024) {
                    for (int j = 0; j < childCount; ++j) {
                        char* item = items[j];
                        if (item && is_readable(item)) {
                            ApplyItemEntryGeometry(item, ItemEntryKind::InGame);
                        }
                    }
                }
                LogListBoxHeightInvariant("inventory/equipment", ctrl, geom.itemHeight);
            }
        }
    }
    
    // CSWGuiInGameMap (0x1005ab010): scale internal subcontrols (mapView, mapHider, mapTexture)
    // so the map viewport and grid fit the scaled blue frame
    if (vtable == (void*)0x1005ab010) {
        int mapLeft = (int)(((long long)95  * targetW + baseW / 2) / baseW);
        int mapTop  = (int)(((long long)118 * targetH + baseH / 2) / baseH);
        int mapW    = (int)(((long long)440 * targetW + baseW / 2) / baseW);
        int mapH    = (int)(((long long)256 * targetH + baseH / 2) / baseH);
        
        Rect viewRect = { mapLeft, mapTop, mapW, mapH };
        SetControlRect(panel + 0x80, viewRect);
        
        // mapHider operates relative to the active mapView viewport; origin must be (0, 0)
        Rect hiderRect = { 0, 0, mapW, mapH };
        SetControlRect(panel + 0x1220, hiderRect);
        
        int texW = (int)(((long long)512 * targetW + baseW / 2) / baseW);
        int texH = (int)(((long long)256 * targetH + baseH / 2) / baseH);
        Rect texRect = { 0, 0, texW, texH };
        SetControlRect(panel + 0x1528, texRect);
    }
    
    // CSWGuiClassSelection mirrors the Windows module: initialize the six embedded
    // slot rectangles once in target coordinates, then replace the three per-frame
    // setter arguments with the same final target rectangle.
    if (vtable == (void*)0x1005af890) {
        static const ptrdiff_t classControlOffsets[6] = {
            0x90, 0x138, 0x1c0, 0x248, 0x2d0, 0x358,
        };
        for (int slot = 0; slot < 6; ++slot) {
            const ptrdiff_t slotOffset = (ptrdiff_t)slot * 0x320;
            const Rect slotRect = getScaledClassSlotRect(slot, targetW, targetH);
            for (size_t i = 0; i < sizeof(classControlOffsets) / sizeof(classControlOffsets[0]); ++i) {
                char* control = panel + slotOffset + classControlOffsets[i];
                if (is_readable(control)) SetControlRect(control, slotRect);
            }
        }
    }

    // Embedded tabs and the fullscreen main menu use client-relative coordinates.
    // Independent 4:3 roots retain the engine's centered-panel transform.
    if (fullscreenMainMenu || isEmbeddedInGameMenuPanel(vtable)) {
        panel[0x5c] = (panel[0x5c] & ~0x68) | 0x01;
    } else if (isTopLevelMenu(vtable) || is43 || isTopTab) {
        panel[0x5c] = (panel[0x5c] & ~0x09) | 0x60;
    }
    
    s_scaledPanels.insert(panel);

    if (vtable == (void*)0x1005ae790) {
        ReflowMessagesForGeneration(panel);
    }
    
    // If this is the master in-game menu container, also scale any embedded child tab screens
    if (vtable == (void*)0x1005ae6a0) {
        int childOffsets[8] = { 0x1180, 0x13c0, 0x1600, 0x1840, 0x1a80, 0x1cc0, 0x1f00, 0x2140 };
        for (int k = 0; k < 8; k++) {
            char* child = panel + childOffsets[k];
            if (child && is_readable(child)) {
                scaleMenuPanelTree(child);
            }
        }
    }
}

void refreshMenuPanelTrees(char* mgr) {
    if (!mgr || !is_readable(mgr)) return;

    int defaultCanvasW = 640;
    int defaultCanvasH = 480;
    GetMenuCanvasSize(640, 480, defaultCanvasW, defaultCanvasH);
    patchMenuCenteringConstants(defaultCanvasW, defaultCanvasH);

    char** panels = *(char***)(mgr + 0xd8);
    int panelCount = *(int*)(mgr + 0xe0);
    if (!panels || !is_readable(panels) || panelCount <= 0 || panelCount > 256) return;
    for (int i = 0; i < panelCount; i++) {
        char* panel = panels[i];
        if (panel && is_readable(panel)) {
            scaleMenuPanelTree(panel);
        }
    }

    // A fullscreen Main Menu or a custom-base panel must not leave the process-wide
    // input-centering operands in a panel-specific state.
    patchMenuCenteringConstants(defaultCanvasW, defaultCanvasH);
}

// Resolution commits can occur while no gameplay HUD is active. Defer GUI mutation
// until the next manager/HUD draw, then refresh each manager exactly once per layout generation.
static std::unordered_map<char*, uint64_t> s_managerRefreshGenerations;

static void ResetManagerPanelFonts(char* manager) {
    if (!manager || !is_readable(manager + 0xe0)) return;
    char** panels = *(char***)(manager + 0xd8);
    const int panelCount = *(int*)(manager + 0xe0);
    if (!panels || !is_readable(panels) || panelCount <= 0 || panelCount > 256) return;

    using ResetFontFn = void (*)(void*);
    constexpr uintptr_t kPanelResetFont = 0x10049e8a4;
    for (int i = 0; i < panelCount; ++i) {
        char* panel = panels[i];
        if (panel && is_readable(panel)) {
            ((ResetFontFn)kPanelResetFont)(panel);
        }
    }
}

static void RefreshManagerLayoutForGeneration(char* manager) {
    if (!manager || !is_readable(manager)) return;
    updateEngineGlobals(manager);

    const auto it = s_managerRefreshGenerations.find(manager);
    if (it != s_managerRefreshGenerations.end() &&
        it->second == g_layoutGeneration) {
        return;
    }

    refreshMenuPanelTrees(manager);
    ScaledFont::RefreshAllLoadedFonts();
    ResetManagerPanelFonts(manager);
    s_managerRefreshGenerations[manager] = g_layoutGeneration;
}

/*
  Class-selection geometry.

  The upstream Windows module overwrites the base and wrapper stack rectangles with
  one final scaled slot rectangle immediately before each SetExtent pair. Do the same
  here: the live slot controls are initialized to the final rectangle, and each focused
  call-site hook sends that same rectangle to the button/model setter exactly once.
*/
static Rect GetClassSelectionTargetRect(int slot) {
    int targetW = 640;
    int targetH = 480;
    GetMenuCanvasSize(640, 480, targetW, targetH);
    return getScaledClassSlotRect(slot, targetW, targetH);
}

extern "C" void Hook_ClassSelectionSetButtonExtent(
    char* button, const Rect* originalRect, char* panel, uintptr_t slotOffset) {
    BEGIN_HOOK();
    using ButtonSetExtentFn = void (*)(void*, const Rect*);
    constexpr uintptr_t kButtonSetExtent = 0x1004a5adc;
    if (!button || !originalRect) return;

    const int slot = (int)(slotOffset / 0x320);
    if (!panel || slotOffset >= 0x12c0 || slot < 0 || slot >= 6) {
        ((ButtonSetExtentFn)kButtonSetExtent)(button, originalRect);
        return;
    }

    RefreshRuntimeConfiguration();
    const Rect target = GetClassSelectionTargetRect(slot);
    ((ButtonSetExtentFn)kButtonSetExtent)(button, &target);
}

extern "C" void Hook_ClassSelectionSetModelExtent(
    char* model, const Rect* originalRect, char* panel, uintptr_t slotOffset) {
    BEGIN_HOOK();
    using ModelSetExtentFn = void (*)(void*, const Rect*);
    constexpr uintptr_t kModelSetExtent = 0x1004aaca6;
    if (!model || !originalRect) return;

    const int slot = (int)(slotOffset / 0x320);
    if (!panel || slotOffset >= 0x12c0 || slot < 0 || slot >= 6) {
        ((ModelSetExtentFn)kModelSetExtent)(model, originalRect);
        return;
    }

    RefreshRuntimeConfiguration();
    const Rect target = GetClassSelectionTargetRect(slot);
    ((ModelSetExtentFn)kModelSetExtent)(model, &target);
}

struct HudControlSnapshot {
    char* control = nullptr;
    void* vtable = nullptr;
    Rect authored{};
};

struct HudSnapshot {
    char* hud = nullptr;
    void* vtable = nullptr;
    char** controlsIdentity = nullptr;
    int controlCount = 0;
    std::vector<HudControlSnapshot> controls;
    uint64_t appliedGeneration = 0;
    int appliedKnobVersion = -1;
};

static HudSnapshot s_hudSnapshot;

static void ClearHudSnapshot() {
    s_hudSnapshot = HudSnapshot{};
}

static bool EnsureHudSnapshot(char* hud) {
    if (!hud || !is_readable(hud + 0x38)) return false;
    void* vtable = *(void**)hud;
    char** controls = *(char***)(hud + 0x30);
    const int count = *(int*)(hud + 0x38);
    if (!controls || !is_readable(controls) || count <= 0 || count > 1024) return false;

    if (s_hudSnapshot.hud != hud || s_hudSnapshot.vtable != vtable) {
        ClearHudSnapshot();
        s_hudSnapshot.hud = hud;
        s_hudSnapshot.vtable = vtable;
    }

    std::vector<HudControlSnapshot> updated((size_t)count);
    for (int i = 0; i < count; ++i) {
        char* current = controls[i];
        void* currentVtable = current && is_readable(current + 0x17) ? *(void**)current : nullptr;
        if (i < s_hudSnapshot.controlCount &&
            (size_t)i < s_hudSnapshot.controls.size() &&
            s_hudSnapshot.controls[(size_t)i].control == current &&
            s_hudSnapshot.controls[(size_t)i].vtable == currentVtable) {
            updated[(size_t)i] = s_hudSnapshot.controls[(size_t)i];
        } else if (current && currentVtable) {
            updated[(size_t)i].control = current;
            updated[(size_t)i].vtable = currentVtable;
            updated[(size_t)i].authored = *(Rect*)(current + 0x8);
        }
    }

    s_hudSnapshot.controlsIdentity = controls;
    s_hudSnapshot.controlCount = count;
    s_hudSnapshot.controls.swap(updated);
    return true;
}

static const Rect* GetHudAuthoredRect(int index) {
    if (index < 0 || index >= s_hudSnapshot.controlCount) return nullptr;
    return &s_hudSnapshot.controls[(size_t)index].authored;
}

static char* GetHudControl(int index) {
    if (index < 0 || index >= s_hudSnapshot.controlCount) return nullptr;
    return s_hudSnapshot.controls[(size_t)index].control;
}

static void PositionBarkBubble(char* window) {
    if (!window || !is_readable(window + 0x21f)) return;

    int minimapBottom = std::max(1, (int)std::lround(144.0f * ReadIniHudScale()));
    char* mapBorder = GetHudControl(15);
    if (mapBorder && is_readable(mapBorder + 0x17)) {
        const Rect rect = *(Rect*)(mapBorder + 0x8);
        if (rect.height > 0 && rect.height < 4096) {
            minimapBottom = rect.top + rect.height;
        }
    }

    const int margin = std::max(1, (int)std::lround(8.0f * ReadIniHudScale()));
    const int desiredTop = minimapBottom + margin;
    *(int*)(window + 0x21c) = desiredTop;

    Rect root = *(Rect*)(window + 0x8);
    if (root.top != desiredTop) {
        root.top = desiredTop;
        SetControlRect(window, root);
    }
}

static void ApplyHudLayout(char* hud) {
    if (!EnsureHudSnapshot(hud)) return;
    const int numControls = s_hudSnapshot.controlCount;
    const float hudScale = ReadIniHudScale();
    const float combatScale = ReadIniCombatScale();

    SetControlRect(hud, { 0, 0, g_targetWidth, g_targetHeight });

    const int minimapBorder = std::max(1, (int)std::lround(144.0f * hudScale));
    const int minimapRadar = std::max(1, (int)std::lround(130.0f * hudScale));
    const int minimapInset = (minimapBorder - minimapRadar) / 2;
    const int arrowSize = std::max(1, (int)std::lround(27.0f * hudScale));
    const int arrowOffset = (minimapRadar - arrowSize) / 2;

    *(Rect*)(hud + 0x7b08) = { minimapInset, minimapInset, minimapRadar, minimapRadar };
    *(Rect*)(hud + 0x7b28) = { minimapInset, minimapInset, minimapRadar, minimapRadar };
    SetControlRect(hud + 0x7970,
                   { arrowOffset, arrowOffset, arrowSize, arrowSize });

    // Top-right menu controls. The authored control identities are stable even after a mode change.
    static constexpr int menuIndices[8] = { 30, 29, 27, 28, 23, 24, 25, 26 };
    const int buttonWidth = 42;
    const int buttonHeight = 42;
    const int buttonSpacing = 43;
    const int topMargin = 2;
    const int lastRight = g_targetWidth - 4;
    const int firstLeft = lastRight - buttonWidth - 7 * buttonSpacing;
    for (int i = 0; i < 8; ++i) {
        char* control = GetHudControl(menuIndices[i]);
        if (control) SetControlRect(control,
            { firstLeft + i * buttonSpacing, topMargin, buttonWidth, buttonHeight });
    }
    char* bannerControl = GetHudControl(19);
    if (bannerControl) {
        SetControlRect(bannerControl,
            { firstLeft + 1, topMargin + 2, lastRight - 1 - (firstLeft + 1), buttonHeight - 4 });
    }
    for (int index : { 0, 5 }) {
        char* control = GetHudControl(index);
        const Rect* authored = GetHudAuthoredRect(index);
        if (control && authored) {
            Rect rect = *authored;
            rect.left = minimapBorder - 1;
            rect.width = std::max(1, firstLeft + 2 - rect.left);
            SetControlRect(control, rect);
        }
    }

    // Bottom-right action description cluster. Always derive from immutable authored geometry.
    constexpr int actionOriginX = 1095;
    constexpr int actionOriginY = 872;
    const int actionWidth = std::max(1, (int)std::lround(180.0f * hudScale));
    const int actionHeight = std::max(1, (int)std::lround(85.0f * hudScale));
    const int actionLeft = g_targetWidth - 6 - actionWidth;
    const int actionTop = g_targetHeight - 8 - actionHeight;
    for (int i = 0; i < numControls; ++i) {
        char* control = GetHudControl(i);
        const Rect* authored = GetHudAuthoredRect(i);
        if (!control || !authored) continue;
        if (authored->left >= 1090 && authored->left <= 1280 &&
            authored->top >= 870 && authored->top <= 960 &&
            authored->width > 0 && authored->height > 0) {
            SetControlRect(control, {
                actionLeft + (int)std::lround((authored->left - actionOriginX) * hudScale),
                actionTop + (int)std::lround((authored->top - actionOriginY) * hudScale),
                std::max(1, (int)std::lround(authored->width * hudScale)),
                std::max(1, (int)std::lround(authored->height * hudScale)),
            });
        }
    }

    // Bottom-left portrait/status cluster, also from the immutable snapshot.
    constexpr int portraitOriginX = 4;
    constexpr int portraitOriginY = 822;
    const int portraitHeight = std::max(1, (int)std::lround(134.0f * hudScale));
    const int portraitTop = g_targetHeight - 8 - portraitHeight;
    for (int i = 0; i < numControls; ++i) {
        char* control = GetHudControl(i);
        const Rect* authored = GetHudAuthoredRect(i);
        if (!control || !authored) continue;
        if (authored->left <= 320 && authored->top >= 750 &&
            authored->width > 0 && authored->height > 0) {
            SetControlRect(control, {
                portraitOriginX + (int)std::lround((authored->left - portraitOriginX) * hudScale),
                portraitTop + (int)std::lround((authored->top - portraitOriginY) * hudScale),
                std::max(1, (int)std::lround(authored->width * hudScale)),
                std::max(1, (int)std::lround(authored->height * hudScale)),
            });
        }
    }

    char* c = GetHudControl(15);
    if (c) SetControlRect(c, { 0, 0, minimapBorder, minimapBorder });
    c = GetHudControl(16);
    if (c) SetControlRect(c, { minimapInset, minimapInset, minimapRadar, minimapRadar });
    c = GetHudControl(17);
    if (c) SetControlRect(c, { minimapInset, minimapInset, minimapRadar, minimapRadar });
    c = GetHudControl(80);
    if (c) SetControlRect(c,
        { minimapInset + arrowOffset, minimapInset + arrowOffset, arrowSize, arrowSize });

    // Center combat queue.
    const int pillWidth = std::max(1, (int)std::lround(131.0f * combatScale));
    const int pillHeight = std::max(1, (int)std::lround(90.0f * combatScale));
    const int pillLeft = (g_targetWidth - pillWidth) / 2;
    const int queueTop = g_targetHeight - 8 - pillHeight;
    const Rect pill = { pillLeft, queueTop, pillWidth, pillHeight };
    c = GetHudControl(1);
    if (c) SetControlRect(c, pill);
    c = GetHudControl(2);
    if (c) SetControlRect(c, pill);
    c = GetHudControl(3);
    if (c) SetControlRect(c,
        { pillLeft, queueTop, pillWidth, std::max(1, (int)std::lround(77.0f * combatScale)) });
    c = GetHudControl(4);
    if (c) SetControlRect(c, {
        pillLeft + (int)std::lround(3.0f * combatScale),
        queueTop + (int)std::lround(56.0f * combatScale),
        std::max(1, (int)std::lround(125.0f * combatScale)),
        std::max(1, (int)std::lround(35.0f * combatScale)),
    });

    const int activeX = pillLeft + (int)std::lround(102.0f * combatScale);
    const int activeY = queueTop + (int)std::lround(28.0f * combatScale);
    const int activeSize = std::max(1, (int)std::lround(32.0f * combatScale));
    c = GetHudControl(90);
    if (c) SetControlRect(c,
        { activeX - activeSize / 2, activeY - activeSize / 2, activeSize, activeSize });
    c = GetHudControl(95);
    if (c) {
        const int w = std::max(1, (int)std::lround(51.0f * combatScale));
        const int h = std::max(1, (int)std::lround(53.0f * combatScale));
        SetControlRect(c, { activeX - w / 2, activeY - h / 2, w, h });
    }

    const int queuedSize = std::max(1, (int)std::lround(20.0f * combatScale));
    const int queuedY = queueTop + (int)std::lround(41.0f * combatScale);
    const int queuedX[3] = {
        pillLeft + (int)std::lround(pillWidth * 0.52f),
        pillLeft + (int)std::lround(pillWidth * 0.35f),
        pillLeft + (int)std::lround(pillWidth * 0.18f),
    };
    for (int i = 0; i < 3; ++i) {
        c = GetHudControl(91 + i);
        if (c) SetControlRect(c,
            { queuedX[i] - queuedSize / 2, queuedY - queuedSize / 2, queuedSize, queuedSize });
    }
    c = GetHudControl(94);
    if (c) SetControlRect(c, {
        pillLeft + (int)std::lround(18.0f * combatScale),
        queuedY - (int)std::lround(15.0f * combatScale),
        std::max(1, (int)std::lround(75.0f * combatScale)),
        std::max(1, (int)std::lround(30.0f * combatScale)),
    });

    // Rebuild existing GUI strings after font metric changes.
    ((void(*)(void*))0x100239500)(hud); // CSWGuiMainInterface::ResetFont

    s_hudSnapshot.appliedGeneration = g_layoutGeneration;
    s_hudSnapshot.appliedKnobVersion = s_knobVersion;
    g_lastScaledHud = hud;
}

/* Hook_MainInterfaceDraw: pre-hook; KPM resumes the original Draw after return. */
extern "C" void Hook_MainInterfaceDraw(char* hud) {
    BEGIN_HOOK();
    TryAdoptCommittedResolution();
    if (!s_committedResolutionObserved) return;
    RefreshRuntimeConfiguration();
    refreshPatchedListConstants();
    if (!hud || !is_readable(hud)) return;

    char* manager = *(char**)(hud + 0x20);
    if (manager && is_readable(manager)) {
        // +0xa8/+0xa9 are engine-owned backdrop draw-order flags; never clear them here.
        RefreshManagerLayoutForGeneration(manager);
    } else {
        updateEngineGlobals(nullptr);
    }

    *(Rect*)(hud + 0x8) = { 0, 0, g_targetWidth, g_targetHeight };
    if (!EnsureHudSnapshot(hud)) return;
    if (s_hudSnapshot.appliedGeneration != g_layoutGeneration ||
        s_hudSnapshot.appliedKnobVersion != s_knobVersion ||
        g_lastScaledHud != hud) {
        ApplyHudLayout(hud);
    }

    // Dynamic action-description text can be relaid out by the engine each frame.
    char* actionDesc = hud + 0xce40;
    char* actionDescBg = hud + 0xcfd8;
    if (is_readable(actionDesc + 0x17) && is_readable(actionDescBg + 0x17)) {
        Rect rect = *(Rect*)(actionDesc + 0x8);
        if (rect.width > 0 && rect.height > 0 && rect.left + rect.width > g_targetWidth - 4) {
            rect.left = g_targetWidth - 4 - rect.width;
            SetControlRect(actionDesc, rect);
            SetControlRect(actionDescBg, rect);
        }
    }

    CorrectFloatingTargetClamp(hud);
    if (s_lastTargetActionMenu && s_lastTargetGeneration != g_layoutGeneration) {
        ApplyFloatingTargetGeometry(s_lastTargetActionMenu);
    }
    ScaledFont::RefreshAllLoadedFonts();
}


/*
  Hook_PanelPostLoad:
  Runs at the epilogue of the helper tail-called by CSWGuiPanel::StopLoadFromLayout,
  after linked controls have been resolved. This replaces the broad per-frame
  CSWGuiPanel::Draw detour.
*/
extern "C" void Hook_PanelPostLoad(char* panel) {
    BEGIN_HOOK();
    TryAdoptCommittedResolution();
    if (!s_committedResolutionObserved || !panel || !is_readable(panel)) return;
    RefreshRuntimeConfiguration();
    refreshPatchedListConstants();

    char* manager = *(char**)(panel + 0x20);
    updateEngineGlobals(manager);
    void* vtable = *(void**)panel;

    if (vtable == (void*)0x1005d3210) { // CSWGuiToolTipPanel
        Rect full = { 0, 0, g_targetWidth, g_targetHeight };
        SetControlRect(panel, full);
        if (is_readable(panel + 0x100)) *(int*)(panel + 0x100) = 500;
        return;
    }

    // The Windows StopLoadFromLayout hook is geometry-qualified rather than
    // vtable-allowlisted. scaleMenuPanelTree performs the HUD/tooltip/popup guards.
    scaleMenuPanelTree(panel);
}


static void ForgetListBoxStatesInPanel(char* panel) {
    if (!panel || !is_readable(panel + 0x38)) return;
    char** controls = *(char***)(panel + 0x30);
    const int controlCount = *(int*)(panel + 0x38);
    if (controls && is_readable(controls) && controlCount > 0 && controlCount <= 1024) {
        for (int i = 0; i < controlCount; ++i) {
            if (controls[i]) s_listBoxAuthoredStates.erase(controls[i]);
        }
    }
    s_listBoxAuthoredStates.erase(panel);
}

// CSWGuiPanel::~CSWGuiPanel entry hook. Cache erasure occurs while the panel and its
// control array are still valid; KPM then replays the original destructor prologue.
extern "C" void Hook_PanelDestructor(char* panel) {
    BEGIN_HOOK();
    if (!panel) return;

    ForgetListBoxStatesInPanel(panel);
    s_scaledPanels.erase(panel);
    s_popupSnapshots.erase(panel);
    if (s_statusSummaryBuilding == panel) s_statusSummaryBuilding = nullptr;
    s_smallPanelSnapshots.erase(panel);
    s_menuPanelSnapshots.erase(panel);
    s_messageReflowGenerations.erase(panel);

    if (s_hudSnapshot.hud == panel || g_lastScaledHud == panel) {
        ClearHudSnapshot();
        g_lastScaledHud = nullptr;
        ForgetTargetClampState(panel);
    }

    // The target-action owner is embedded in the gameplay HUD. Reacquire it through the
    // focused bind/draw hooks after any panel destruction rather than retaining stale memory.
    s_lastTargetActionMenu = nullptr;
    s_lastTargetGeneration = 0;
    s_managerRefreshGenerations.clear();
}

// Dialogue letterbox bars use height/6, matching the Windows patch. These hooks run at
// the SetTop/SetBottom epilogues after the stock width-derived calculation has completed.
static int GetLetterboxBarHeight() {
    return std::max(1, (g_targetHeight + 3) / 6);
}

extern "C" void Hook_LetterboxSetTop(char* letterbox) {
    BEGIN_HOOK();
    TryAdoptCommittedResolution();
    if (!s_committedResolutionObserved || !letterbox || !is_readable(letterbox + 0x9f)) return;
    RefreshRuntimeConfiguration();

    const int barHeight = GetLetterboxBarHeight();
    *(Rect*)(letterbox + 0x8) = { 0, 0, g_targetWidth, 0 };
    *(Rect*)(letterbox + 0x7c) = { 0, 0, g_targetWidth, barHeight };
    *(int*)(letterbox + 0x98) = 0;
    *(int*)(letterbox + 0x9c) = 1;
}

extern "C" void Hook_LetterboxSetBottom(char* letterbox) {
    BEGIN_HOOK();
    TryAdoptCommittedResolution();
    if (!s_committedResolutionObserved || !letterbox || !is_readable(letterbox + 0x9f)) return;
    RefreshRuntimeConfiguration();

    const int barHeight = GetLetterboxBarHeight();
    *(Rect*)(letterbox + 0x8) = { 0, g_targetHeight, g_targetWidth, barHeight };
    *(Rect*)(letterbox + 0x7c) =
        { 0, g_targetHeight - barHeight, g_targetWidth, barHeight };
    *(int*)(letterbox + 0x90) = g_targetHeight;
    *(int*)(letterbox + 0x9c) = 2;
}

// ==============================================================================
// 3. SCALED FLOATING TARGETS (HEALTH BARS & ACTION ICONS)
// ==============================================================================
/*
  Background & Technical Rationale:
  In Star Wars: KotOR, targeting an NPC or interactive object renders a floating
  HUD reticle in 3D world space. This reticle contains:
    - Target Nameplate (LBL_NAME / LBL_NAMEBG): original base { 0, 0, 200, 26 }
    - Target Health Bar (PB_HEALTH / LBL_HEALTHBG): original base { 0, 27, 200, 6 }
    - 3 Combat Action Slots, each with:
        * Container Button (BTN_TARGET0..2): { 43, 35, 35, 59 }, { 83, ... }, { 122, ... }
        * Action Icon (LBL_TARGET0..2): { 45, 49, 32, 32 }, { 85, ... }, { 124, ... }
        * Cycle Up Arrow (BTN_TARGETUP0..2): { 43, 36, 35, 12 }, { 83, ... }, { 122, ... }
        * Cycle Down Arrow (BTN_TARGETDOWN0..2): { 44, 80, 35, 12 }, { 84, ... }, { 123, ... }

  Vanilla Problem:
  The target-action menu repeatedly restores its authored 800x600 control extents during
  initialization and drawing. Focused lifecycle hooks reapply scaled extents after binding
  controls and immediately before the target name is drawn; no global GUI setter is intercepted.

  Windows parity:
  The source rectangles are authored for an 800x600 layout. The Windows module converts
  each axis through the shared 640x480 uniform UI canvas with scaleUiValueFromBase.
  This is still a uniform transform, but unlike a height-only approximation it also handles
  width-constrained modes correctly. Square action icons remain square at every aspect ratio.
*/

static Rect ScaleTargetRect(const Rect& source,
                            const UniversalScaleState& scale) {
    return {
        ScaleUiValueFromBase(source.left, 800, 640, scale),
        ScaleUiValueFromBase(source.top, 600, 480, scale),
        std::max(1, ScaleUiValueFromBase(source.width, 800, 640, scale)),
        std::max(1, ScaleUiValueFromBase(source.height, 600, 480, scale)),
    };
}

static void ApplyFloatingTargetGeometry(char* owner) {
    if (!owner || !is_readable(owner) || !is_readable(owner + 0x2080)) return;
    RefreshRuntimeConfiguration();
    const UniversalScaleState scale = GetUniversalScaleState();

    static const Rect actionRects[4] = {
        { 43, 35, 35, 59 }, // container button
        { 45, 49, 32, 32 }, // action icon
        { 43, 36, 35, 12 }, // cycle up
        { 44, 80, 35, 12 }  // cycle down
    };
    static const ptrdiff_t actionOffsets[4] = { 0x000, 0x240, 0x480, 0x6c0 };

    for (int slot = 0; slot < 3; ++slot) {
        char* entry = owner + 0x60 + slot * 0x910;
        const int baseShift = slot == 0 ? 0 : (slot == 1 ? 40 : 79);
        for (int control = 0; control < 4; ++control) {
            Rect base = actionRects[control];
            base.left += baseShift;
            SetControlRect(entry + actionOffsets[control], ScaleTargetRect(base, scale));
        }
    }

    const Rect nameRect = ScaleTargetRect({ 0, 0, 200, 26 }, scale);
    const Rect healthRect = ScaleTargetRect({ 0, 27, 200, 6 }, scale);
    SetControlRect(owner + 0x1bb8, nameRect); // LBL_NAME
    SetControlRect(owner + 0x1d50, nameRect); // LBL_NAMEBG
    SetControlRect(owner + 0x1ee8, healthRect); // LBL_HEALTHBG
    SetControlRect(owner + 0x2080, healthRect); // PB_HEALTH

    s_lastTargetActionMenu = owner;
    s_lastTargetGeneration = g_layoutGeneration;
}

struct TargetClampState {
    int authoredHeight = 0;
    int appliedHeight = 0;
    uint64_t generation = 0;
};

static std::unordered_map<char*, TargetClampState> s_targetClampStates;
static constexpr Rect kAuthoredPauseRect = { 10, 744, 56, 56 };

static void ForgetTargetClampState(char* hud) {
    if (hud) s_targetClampStates.erase(hud);
}

static void CorrectFloatingTargetClamp(char* hud) {
    if (!hud || !is_readable(hud + 0x1cb7)) return;
    RefreshRuntimeConfiguration();

    int& currentHeight = *(int*)(hud + 0x1cb4);
    TargetClampState& state = s_targetClampStates[hud];
    if (state.authoredHeight <= 0 || currentHeight != state.appliedHeight) {
        // The stock path has rebuilt the clamp. Capture that unmodified height.
        state.authoredHeight = currentHeight;
    }
    if (state.authoredHeight <= 0) return;

    if (state.generation == g_layoutGeneration && currentHeight == state.appliedHeight) return;

    const UniversalScaleState scale = GetUniversalScaleState();
    const int scaledPauseTop = ScaleUiValueFromBase(
        kAuthoredPauseRect.top, 960, 480, scale);
    const int desiredHeight = std::max(1,
        state.authoredHeight + (scaledPauseTop - kAuthoredPauseRect.top));
    currentHeight = desiredHeight;
    state.appliedHeight = desiredHeight;
    state.generation = g_layoutGeneration;
}

extern "C" void Hook_TargetControlsBound(char* owner) {
    BEGIN_HOOK();
    TryAdoptCommittedResolution();
    if (!s_committedResolutionObserved) return;
    ApplyFloatingTargetGeometry(owner);
}

extern "C" void Hook_TargetNameDraw(char* owner) {
    BEGIN_HOOK();
    TryAdoptCommittedResolution();
    if (!s_committedResolutionObserved) return;
    ApplyFloatingTargetGeometry(owner);
}

extern "C" void Hook_TargetClampReady(char* hud) {
    BEGIN_HOOK();
    TryAdoptCommittedResolution();
    if (!s_committedResolutionObserved) return;
    CorrectFloatingTargetClamp(hud);
}

static bool ResRefEquals(const char* actual, const char* expected) {
    if (!actual || !expected) return false;
    const size_t length = strlen(expected);
    if (length > 16) return false;
    if (memcmp(actual, expected, length) != 0) return false;
    return length == 16 || actual[length] == '\0';
}

static bool IsWideMenuBackground(const char* name) {
    static const char* const names[] = {
        "800x600back", "800x600comp0", "800x600pazaak", "800x600load",
        "800x600map", "800x600store", "800x600comp1",
    };
    for (const char* expected : names) {
        if (ResRefEquals(name, expected)) return true;
    }
    return false;
}

extern "C" void Hook_MenuBackgroundLabelDraw(char* label) {
    BEGIN_HOOK();
    TryAdoptCommittedResolution();
    if (!s_committedResolutionObserved || !ReadIniHDMenuTextures() ||
        !label || !is_readable(label + 0xd4)) return;

    char* border = label + 0x88;
    const char* resourceName = border + 0x3d;
    if (!IsWideMenuBackground(resourceName)) return;

    Rect rect = *(Rect*)(border + 0x8);
    int menuWidth = 640;
    int menuHeight = 480;
    GetMenuCanvasSize(640, 480, menuWidth, menuHeight);
    rect.left = (g_targetWidth - menuWidth * 2) / 2;
    rect.width = menuWidth * 2;
    memcpy(border + 0x8, &rect, sizeof(rect));
}

extern "C" void Hook_MenuBackdropDraw(char* manager) {
    BEGIN_HOOK();
    TryAdoptCommittedResolution();
    if (!s_committedResolutionObserved || !manager || !is_readable(manager)) return;
    RefreshRuntimeConfiguration();
    RefreshManagerLayoutForGeneration(manager);
    ScaledFont::RefreshAllLoadedFonts();
    char* label = *(char**)(manager + 0xc0);
    if (!label || !is_readable(label)) return;

    int targetW = 640;
    int targetH = 480;
    GetMenuCanvasSize(640, 480, targetW, targetH);
    SetControlRect(label, {
        (g_targetWidth - targetW) / 2,
        (g_targetHeight - targetH) / 2,
        targetW,
        targetH,
    });
}

/*
 ==============================================================================================
  5. C++ DETOUR: SCALED FONTS (CAurFontInfo METADATA SCALING)
 ==============================================================================================
  Port of WindowsScaledKotor "Scaled Font" (font-scale-2x-v1) patch to macOS 64-bit AMD64.
  
  Engine Architecture & Mechanics:
  ----------------------------------------------------------------------------------------------
  In Star Wars: Knights of the Old Republic, fonts are loaded as bitmap textures (fnt_*.tga / .tpc)
  accompanied by metadata TXI files (.txi). During engine startup and resource streaming,
  CResTexture::ParseTXI (0x1001f866a) parses the font attributes:
      - "fontheight"       -> FontHeight (float)       (+0x04)
      - "baselineheight"   -> BaselineHeight (float)   (+0x08)
      - "texturewidth"     -> TextureWidth (float)     (+0x0C)
      - "spacingR"         -> SpacingR (float)         (+0x10)
      - "spacingB"         -> SpacingB (float)         (+0x14)
      - "numchars"         -> Character count (int)    (+0x00)
      - "upperleftcoords"  -> UV coordinates table     (+0x18...)
      - "lowerrightcoords" -> UV coordinates table     (+0x28...)
  
  These attributes are stored inside a dedicated CAurFontInfo structure.
  On the 64-bit Aspyr macOS port, a pointer to this CAurFontInfo is attached to the live
  CResTexture instance at offset +0x48 (which was +0x38 in 32-bit Windows). Non-font textures
  have this field set to nullptr (0x0).
  
  Detour Strategy:
  ----------------------------------------------------------------------------------------------
  We place our detour at 0x1001f8883, the exact instruction where CResTexture::ParseTXI has
  finished loading and validating the texture metadata. At this point:
      - Register R14 holds the live CResTexture pointer.
      - Offset *(void**)(R14 + 0x48) holds the CAurFontInfo pointer (or nullptr).
  
  When scaleLoadedTextureMetadata(void* texture) is called:
      1. We verify that texture is valid and readable via Mach VM query.
      2. We inspect *(void**)(texture + 0x48). If nullptr, the texture is not a font; return.
      3. We verify that the font info contains sane font metrics (fontHeight > 0, textureWidth > 0).
      4. We check our font cache to guarantee each distinct font is scaled exactly once.
      5. We multiply all 5 metric floats by the target font scaling multiplier:
             unrounded = metric * scale;
             metric = floor(unrounded * 100.0f + 0.00001f) / 100.0f;
         This preserves precise subpixel baseline alignment and prevents text baseline drift.
  
  Font Scale Calculation:
  ----------------------------------------------------------------------------------------------
  1. If swkotor.ini contains "FontScale" in [Graphics Options], that explicit multiplier is used.
     Example in swkotor.ini:
         [Graphics Options]
         FontScale=1.25
  2. If FontScale is omitted or 0 (auto-scale mode), use the Windows shared compact scale:
         scale = contentScale * 4 / 9
     with content scaling disabled below a 1080-pixel committed framebuffer height.
 ==============================================================================================
*/

namespace ScaledFont {

constexpr uint32_t FontInfoOffset = 0x48;
constexpr uint32_t FontMetricOffsets[] = { 0x04, 0x08, 0x0c, 0x10, 0x14 };
constexpr float FontMetricPixelsPerUnit = 100.0f;
constexpr float FontMetricFloorEpsilon = 0.00001f;

struct FontScaleRecord {
    void* texture = nullptr;
    void* fontInfo = nullptr;
    float original[5]{};
    float applied[5]{};
    uint64_t generation = 0;
};

static std::vector<FontScaleRecord> s_fonts;
static uint64_t s_lastRefreshGeneration = 0;
static std::mutex s_fontMutex;

static float ReadIniFontScale() {
    RefreshRuntimeConfiguration();
    return s_iniFontScale;
}

static float GetEffectiveFontScale() {
    const float iniScale = ReadIniFontScale();
    if (iniScale > 0.0f) return iniScale;
    return TwoXScaleAdjustment(GetUniversalScaleState());
}

static bool HasSaneFontMetrics(char* fontInfo) {
    if (!fontInfo || !is_readable(fontInfo + 0x17)) return false;
    const float fh = *(float*)(fontInfo + 0x04);
    const float bh = *(float*)(fontInfo + 0x08);
    const float tw = *(float*)(fontInfo + 0x0c);
    return fh > 0.0f && fh < 512.0f && bh > 0.0f && bh < 512.0f &&
           tw > 0.0f && tw < 8192.0f;
}

static float FloorFontMetricToPixel(float value) {
    return std::floor(value * FontMetricPixelsPerUnit + FontMetricFloorEpsilon) /
           FontMetricPixelsPerUnit;
}

static bool NearlyEqual(float a, float b) {
    return std::fabs(a - b) < 0.0005f;
}

static FontScaleRecord* FindRecord(void* texture, void* /*fontInfo*/) {
    for (auto& record : s_fonts) {
        if (record.texture == texture) return &record;
    }
    return nullptr;
}

static void ApplyFontUnlocked(void* texture, void* fontInfoPtr) {
    if (!texture || !fontInfoPtr || !is_readable(texture) || !is_readable(fontInfoPtr)) return;
    char* fontInfo = (char*)fontInfoPtr;
    if (!HasSaneFontMetrics(fontInfo)) return;

    FontScaleRecord* record = FindRecord(texture, fontInfoPtr);
    if (!record) {
        FontScaleRecord fresh;
        fresh.texture = texture;
        fresh.fontInfo = fontInfoPtr;
        for (size_t i = 0; i < 5; ++i) {
            fresh.original[i] = *(float*)(fontInfo + FontMetricOffsets[i]);
            fresh.applied[i] = fresh.original[i];
        }
        s_fonts.push_back(fresh);
        record = &s_fonts.back();
    } else {
        const bool identityChanged = record->texture != texture || record->fontInfo != fontInfoPtr;
        bool resourceWasReloaded = identityChanged;
        if (!resourceWasReloaded) {
            for (size_t i = 0; i < 5; ++i) {
                const float current = *(float*)(fontInfo + FontMetricOffsets[i]);
                if (!NearlyEqual(current, record->applied[i])) {
                    resourceWasReloaded = true;
                    break;
                }
            }
        }
        if (resourceWasReloaded) {
            record->texture = texture;
            record->fontInfo = fontInfoPtr;
            for (size_t i = 0; i < 5; ++i) {
                record->original[i] = *(float*)(fontInfo + FontMetricOffsets[i]);
                record->applied[i] = record->original[i];
            }
            record->generation = 0;
        }
    }

    if (record->generation == g_layoutGeneration) return;
    const float scale = GetEffectiveFontScale();
    for (size_t i = 0; i < 5; ++i) {
        const float value = FloorFontMetricToPixel(record->original[i] * scale);
        *(float*)(fontInfo + FontMetricOffsets[i]) = value;
        record->applied[i] = value;
    }
    record->generation = g_layoutGeneration;
}


static void ApplyFont(void* texture, void* fontInfoPtr) {
    std::lock_guard<std::mutex> lock(s_fontMutex);
    ApplyFontUnlocked(texture, fontInfoPtr);
}

void RefreshAllLoadedFonts() {
    std::lock_guard<std::mutex> lock(s_fontMutex);
    if (s_lastRefreshGeneration == g_layoutGeneration) return;

    for (size_t i = 0; i < s_fonts.size();) {
        FontScaleRecord& record = s_fonts[i];
        if (!record.texture || !is_readable(record.texture) ||
            !is_readable((char*)record.texture + FontInfoOffset + sizeof(void*) - 1) ||
            *(void**)((char*)record.texture + FontInfoOffset) != record.fontInfo ||
            !record.fontInfo || !is_readable(record.fontInfo)) {
            s_fonts.erase(s_fonts.begin() + (ptrdiff_t)i);
            continue;
        }
        ApplyFontUnlocked(record.texture, record.fontInfo);
        ++i;
    }
    s_lastRefreshGeneration = g_layoutGeneration;
}

} // namespace ScaledFont

/* CResTexture::ParseTXI completion hook. */
extern "C" void scaleLoadedTextureMetadata(void* texture) {
    BEGIN_HOOK();
    if (!texture || !is_readable(texture) || !is_readable((char*)texture + 0x4f)) return;
    RefreshRuntimeConfiguration();
    void* fontInfo = *(void**)((char*)texture + ScaledFont::FontInfoOffset);
    ScaledFont::ApplyFont(texture, fontInfo);
}

