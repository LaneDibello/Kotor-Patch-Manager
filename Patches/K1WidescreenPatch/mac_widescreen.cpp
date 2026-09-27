#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <mach/mach.h>
#include <mach/vm_map.h>
#include <cmath>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <algorithm>

/*
 ==============================================================================================
  STAR WARS: KNIGHTS OF THE OLD REPUBLIC 1 (MAC STEAM / ASYPR 64-BIT PORT)
  PURE C++ WIDESCREEN RESOLUTION, HUD & MENU CENTERING PATCH
 ==============================================================================================
  RESOLUTION CONFIGURATION:
  The resolution is AUTOMATICALLY detected from swkotor.ini (under [Graphics Options]).
  To change your resolution, simply set:
       Width=1440   (or 1512, 1728, 1920, 2560, etc.)
       Height=900   (or 982, 1117, 1080, 1440, etc.)
  in ~/Library/Application Support/Knights of the Old Republic/swkotor.ini.
  No manual hex-editing or code recompilation is required!
 ==============================================================================================
*/
int g_targetWidth  = 1512; // Target display width (defaults to 14" MacBook Pro Liquid Retina default)
int g_targetHeight = 982;  // Target display height

struct NativeCGPoint { double x, y; };
struct NativeCGSize { double width, height; };
struct NativeCGRect { NativeCGPoint origin; NativeCGSize size; };

typedef uint32_t (*CGMainDisplayIDFn)();
typedef NativeCGRect (*CGDisplayBoundsFn)(uint32_t);

static bool s_resolutionInitialized = false;

static void InitTargetResolution() {
    if (s_resolutionInitialized) return;
    s_resolutionInitialized = true;

    // 1. Hardware Display Auto-Detection via macOS CoreGraphics (loaded dynamically via dlopen)
    // Obtains the active monitor's native point resolution (e.g. 1512x982 on 14" MacBook Pro,
    // 1728x1117 on 16" MacBook Pro, 1920x1200 on 1200p monitors, 1920x1080 on 1080p, 2560x1440 on 1440p).
    // Cocoa window coordinates and mouse events match these exact display bounds.
    void* cgLib = dlopen("/System/Library/Frameworks/CoreGraphics.framework/CoreGraphics", RTLD_LAZY | RTLD_LOCAL);
    if (!cgLib) {
        cgLib = dlopen("CoreGraphics.framework/CoreGraphics", RTLD_LAZY | RTLD_LOCAL);
    }
    if (cgLib) {
        auto pfnMainID = (CGMainDisplayIDFn)dlsym(cgLib, "CGMainDisplayID");
        auto pfnBounds = (CGDisplayBoundsFn)dlsym(cgLib, "CGDisplayBounds");
        if (pfnMainID && pfnBounds) {
            uint32_t display = pfnMainID();
            NativeCGRect bounds = pfnBounds(display);
            int dispW = (int)bounds.size.width;
            int dispH = (int)bounds.size.height;
            if (dispW >= 640 && dispH >= 480) {
                g_targetWidth = dispW;
                g_targetHeight = dispH;
            }
        }
        dlclose(cgLib);
    }

    // 2. Check swkotor.ini for optional user override
    // - ForceWidth / ForceHeight: explicit user override
    // - Width / Height: used if CoreGraphics auto-detection returned 0
    char path[1024];
    FILE* f = nullptr;
    const char* home = getenv("HOME");
    if (home) {
        snprintf(path, sizeof(path), "%s/Library/Application Support/Knights of the Old Republic/swkotor.ini", home);
        f = fopen(path, "r");
    }
    if (!f) {
        f = fopen("swkotor.ini", "r");
    }
    if (f) {
        char line[256];
        bool inGraphics = false;
        int forceW = 0, forceH = 0;
        int iniW = 0, iniH = 0;
        while (fgets(line, sizeof(line), f)) {
            char* p = line;
            while (*p == ' ' || *p == '\t') p++;
            if (*p == '[') {
                inGraphics = (strncasecmp(p, "[Graphics Options]", 18) == 0);
                continue;
            }
            if (inGraphics) {
                if (strncasecmp(p, "ForceWidth", 10) == 0) {
                    char* eq = strchr(p, '=');
                    if (eq) {
                        char* val = eq + 1;
                        while (*val == ' ' || *val == '\t') val++;
                        forceW = atoi(val);
                    }
                } else if (strncasecmp(p, "ForceHeight", 11) == 0) {
                    char* eq = strchr(p, '=');
                    if (eq) {
                        char* val = eq + 1;
                        while (*val == ' ' || *val == '\t') val++;
                        forceH = atoi(val);
                    }
                } else if (strncasecmp(p, "Width", 5) == 0) {
                    char* eq = strchr(p, '=');
                    if (eq) {
                        char* val = eq + 1;
                        while (*val == ' ' || *val == '\t') val++;
                        iniW = atoi(val);
                    }
                } else if (strncasecmp(p, "Height", 6) == 0) {
                    char* eq = strchr(p, '=');
                    if (eq) {
                        char* val = eq + 1;
                        while (*val == ' ' || *val == '\t') val++;
                        iniH = atoi(val);
                    }
                }
            }
        }
        fclose(f);

        if (forceW >= 640 && forceH >= 480) {
            g_targetWidth = forceW;
            g_targetHeight = forceH;
        } else if (iniW > 800 && iniH > 600) {
            g_targetWidth = iniW;
            g_targetHeight = iniH;
        } else if ((g_targetWidth <= 0 || g_targetHeight <= 0) && iniW >= 640 && iniH >= 480) {
            g_targetWidth = iniW;
            g_targetHeight = iniH;
        }
    }
}

// Native template dimensions used internally by the Mac 4:3 GUI layout (mipc212x9)
int g_refWidth     = 1280;
int g_refHeight    = 960;

struct Rect {
    int left, top, width, height;
};

// Safe memory readability check using macOS Mach virtual memory regions
bool is_readable(const void* ptr) {
    if (!ptr) return false;
    uintptr_t u = (uintptr_t)ptr;
    if (u < 0x10000 || u > 0x7fffffffffffULL) return false;
    
    // Fast path: check against the most recently validated memory region
    static thread_local vm_address_t s_lastRegionStart = 0;
    static thread_local vm_size_t s_lastRegionSize = 0;
    if ((vm_address_t)ptr >= s_lastRegionStart && (vm_address_t)ptr < s_lastRegionStart + s_lastRegionSize) {
        return true;
    }

    vm_map_t map = mach_task_self();
    vm_address_t address = (vm_address_t)ptr;
    vm_size_t vmsize = 0;
    mach_port_t object = 0;
    vm_region_basic_info_data_64_t info;
    mach_msg_type_number_t info_count = VM_REGION_BASIC_INFO_COUNT_64;
    
    if (vm_region_64(map, &address, &vmsize, VM_REGION_BASIC_INFO_64, (vm_region_info_t)&info, &info_count, &object) == KERN_SUCCESS) {
        if ((vm_address_t)ptr >= address && (vm_address_t)ptr < address + vmsize && (info.protection & VM_PROT_READ)) {
            s_lastRegionStart = address;
            s_lastRegionSize = vmsize;
            return true;
        }
    }
    return false;
}

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
    *(Rect*)(ctrl + 0x8) = r;
}

// Check if a panel vtable belongs to an in-game or main menu screen
bool isMenuPanel(void* vtable) {
    return vtable == (void*)0x1005ae6a0 || // CSWGuiInGameMenu (container frame & top tabs)
           vtable == (void*)0x1005ab508 || // CSWGuiInGameEquip
           vtable == (void*)0x1005a75c0 || // CSWGuiInGameInventory
           vtable == (void*)0x1005ad790 || // CSWGuiInGameCharacter
           vtable == (void*)0x1005a5f80 || // CSWGuiInGameAbilities
           vtable == (void*)0x1005aed10 || // CSWGuiInGameJournal
           vtable == (void*)0x1005ab010 || // CSWGuiInGameMap
           vtable == (void*)0x1005ae790 || // CSWGuiInGameMessages
           vtable == (void*)0x1005aba30 || // CSWGuiInGameOptions
           vtable == (void*)0x1005abd60 || // CSWGuiLoadScreen (Loading screen)
           vtable == (void*)0x1005aefa0 || // CSWGuiMainMenu
           vtable == (void*)0x1005ae300 || // CSWGuiSaveLoad (Load & Save Game screen)
           vtable == (void*)0x1005ada20 || // CSWGuiPartySelection
           vtable == (void*)0x1005ab248 || // CSWGuiInGameGalaxyMap
           vtable == (void*)0x1005a5180 || // CSWGuiUpgrade
           vtable == (void*)0x1005ad040 || // CSWGuiStore
           vtable == (void*)0x1005abc50 || // CSWGuiTitleMovies
           vtable == (void*)0x1005af890 || // CSWGuiClassSelection (Character Generation - Class selection)
           vtable == (void*)0x1005ad530 || // CSWGuiMainCharGen (Character Generation - Main panel)
           vtable == (void*)0x1005afea0 || // CSWGuiPortraitCharGen
           vtable == (void*)0x1005aac10 || // CSWGuiNameChargen
           vtable == (void*)0x1005b0950 || // CSWGuiAbilitiesCharGen
           vtable == (void*)0x1005a7820 || // CSWGuiSkillsCharGen
           vtable == (void*)0x1005adc40 || // CSWGuiFeatsCharGen
           vtable == (void*)0x1005a5a90 || // CSWGuiPazaakStart (pazaaksetup.gui - Choose SideDeck)
           vtable == (void*)0x1005a5b80 || // CSWGuiPazaakGame (pazaakgame.gui - Pazaak game board)
           vtable == (void*)0x1005a59a0 || // CSWGuiWagerPopup (pazaakwager.gui - Pazaak wager dialog)
           vtable == (void*)0x1005abfe0 || // CSWGuiOptionsMain (optionsmain.gui - Main Menu Options)
           vtable == (void*)0x1005ac490 || // CSWGuiOptionsSound (optsound.gui - Sound Options)
           vtable == (void*)0x1005ac1c0 || // CSWGuiOptionsGraphics (optgraphics.gui - Graphics Options)
           vtable == (void*)0x1005ac2b0 || // CSWGuiOptionsGraphicsAdvanced (optgraphicsadv.gui - Advanced Graphics Options)
           vtable == (void*)0x1005ac3a0 || // CSWGuiOptionsResolution (optresolution.gui - Resolution Options)
           vtable == (void*)0x1005ac580 || // CSWGuiOptionsMouse (optmouse.gui - Mouse Settings)
           vtable == (void*)0x1005ac0d0 || // CSWGuiOptionsFeedback (optfeedback.gui - Feedback Settings)
           vtable == (void*)0x1005a76d0 || // CSWGuiInGameGameplay (optgameplay.gui - Gameplay Settings)
           vtable == (void*)0x1005a72d0 || // CSWGuiInGameOptKeyMappings (optkeymap.gui - Key Mapping)
           vtable == (void*)0x1005a5d30 || // CSWGuiInGameAutoPause (optautopause.gui - Auto-Pause Options)
           vtable == (void*)0x1005aad20 || // CSWGuiInGameCredits (credits.gui - Game Credits)
           vtable == (void*)0x1005a4fa0 || // CSWGuiUpgradeSelection (upgradesel.gui - Workbench Slot Selection)
           vtable == (void*)0x1005a5090 || // CSWGuiUpgradeItemSelect (upgradeitems.gui - Workbench Item Selection)
           vtable == (void*)0x1005abb40 || // CSWGuiPowersLevelUp (pwrlvlup.gui - Force Powers Level-Up)
           vtable == (void*)0x1005a9b40 || // CSWGuiLevelUpCharGen (MAINCG for Level Up)
           vtable == (void*)0x1005a6db0 || // CSWGuiDialogComputer (computer.gui - Computer Terminals)
           vtable == (void*)0x1005a6ed8;   // CSWGuiDialogComputerCamera (Security Cameras)
}

// Check if a panel is an independent top-level root window
bool isTopLevelMenu(void* vtable) {
    return isMenuPanel(vtable);
}

// Check if a panel is one of the small chargen/level-up root panels (qorcpnl, custpnl, quickpnl, leveluppnl)
bool isSmallChargenPanel(void* vtable) {
    return vtable == (void*)0x1005a9a30 || // CSWGuiQuickOrCustomPanel (qorcpnl)
           vtable == (void*)0x1005a6960 || // CSWGuiCustomPanel (custpnl)
           vtable == (void*)0x1005adb30 || // CSWGuiQuickPanel (quickpnl)
           vtable == (void*)0x1005a4c00;   // CSWGuiLevelUpPanel (leveluppnl - Level Up Choices)
}

// Check if a panel is a popup dialog (container, message box, etc.)
// CSWGuiInGamePause (0x1005ad640) is excluded so the engine's native PositionPause (0x100239496)
// positions it unobtrusively under the top-right category bar without giant centering or stretching.
bool isPopupPanel(void* vtable) {
    return vtable == (void*)0x1005ab758 || // CSWGuiContainer (Footlockers, corpses, placeable containers)
           vtable == (void*)0x1005a5cb8 || // CSWGuiMessageBox (OK / Cancel confirm dialogs)
           vtable == (void*)0x1005ae880 || // CSWGuiMessageBox master vtable
           vtable == (void*)0x1005ae9a0 || // CSWGuiStatusSummary
           vtable == (void*)0x1005a67c0 || // CSWGuiInGameAreaTransition
           vtable == (void*)0x1005abea0 || // CSWGuiInGameSoloModeQuery
           vtable == (void*)0x1005a8c60 || // CSWGuiTutorialBox
           vtable == (void*)0x1005a9e18 || // CSWGuiSkillInfoBox (Feats / Skills granted popup)
           vtable == (void*)0x1005ae3f0 || // CSWGuiSaveNamePanel
           vtable == (void*)0x1005aeaa8 || // CSWGuiControllerLossBox
           vtable == (void*)0x1005aee60;   // CSWGuiExamine
}

static char* g_lastGuiManager = nullptr;
static char* g_lastScaledHud = nullptr;

/*
  updateEngineGlobals:
  Maintains engine global UI dimensions, clipping limits, and GUI manager canvas sizes.
  Overriding 0x1005d3b8c (g_uiWidth) ensures that button tooltips, labels, and mouse hit testing
  can span the full widescreen resolution without being discarded at 1280px.
*/
void updateEngineGlobals(char* mgr) {
    InitTargetResolution();
    // Global UI width and height in Aspyr's engine
    *(int*)0x1005d3b8c = g_targetWidth;
    *(int*)0x1005d3b90 = g_targetHeight;
    
    // OpenGL viewport bounds
    *(short*)0x1005f4b44 = (short)g_targetWidth;
    *(short*)0x1005f4b46 = (short)g_targetHeight;
    
    // CSWGuiManager canvas extents
    if (mgr && is_readable(mgr)) {
        g_lastGuiManager = mgr;
        *(short*)(mgr + 0xa4) = (short)g_targetWidth;
        *(short*)(mgr + 0xa6) = (short)g_targetHeight;
    }

    // Floating Tooltip Window bounds (CSWGuiToolTipPanel via global tooltip manager)
    // Ensures tooltip window scissor rect spans the entire widescreen resolution
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
static float s_cachedIniMenuScale = -1.0f;
static int s_cachedIniHDMenuTextures = -1;

static float ReadIniMenuScale() {
    if (s_cachedIniMenuScale >= 0.0f) {
        return s_cachedIniMenuScale;
    }
    s_cachedIniMenuScale = 0.0f;
    const char* home = getenv("HOME");
    if (!home) return 0.0f;
    
    char path[1024];
    snprintf(path, sizeof(path), "%s/Library/Application Support/Knights of the Old Republic/swkotor.ini", home);
    FILE* f = fopen(path, "r");
    if (!f) return 0.0f;
    
    char line[256];
    bool inGraphics = false;
    while (fgets(line, sizeof(line), f)) {
        char* p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '[') {
            inGraphics = (strncmp(p, "[Graphics Options]", 18) == 0);
            continue;
        }
        if (inGraphics && strncasecmp(p, "MenuScale", 9) == 0) {
            char* eq = strchr(p, '=');
            if (eq) {
                float val = (float)atof(eq + 1);
                if (val > 0.0f && val <= 10.0f) {
                    s_cachedIniMenuScale = val;
                }
            }
        }
    }
    fclose(f);
    return s_cachedIniMenuScale;
}

static float s_cachedIniCombatScale = -1.0f;

static float ReadIniCombatScale() {
    if (s_cachedIniCombatScale >= 0.0f) {
        return s_cachedIniCombatScale;
    }
    InitTargetResolution();
    float autoCombat = (g_targetHeight > 0) ? (1.60f * (float)g_targetHeight / 982.0f) : 1.60f;
    s_cachedIniCombatScale = autoCombat; // Default scaled-up combat queue (+33% over vanilla 1.20x)
    const char* home = getenv("HOME");
    if (!home) return s_cachedIniCombatScale;
    
    char path[1024];
    snprintf(path, sizeof(path), "%s/Library/Application Support/Knights of the Old Republic/swkotor.ini", home);
    FILE* f = fopen(path, "r");
    if (!f) return s_cachedIniCombatScale;
    
    char line[256];
    bool inGraphics = false;
    while (fgets(line, sizeof(line), f)) {
        char* p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '[') {
            inGraphics = (strncmp(p, "[Graphics Options]", 18) == 0);
            continue;
        }
        if (inGraphics && strncasecmp(p, "CombatScale", 11) == 0) {
            char* eq = strchr(p, '=');
            if (eq) {
                float val = (float)atof(eq + 1);
                if (val >= 0.5f && val <= 5.0f) {
                    s_cachedIniCombatScale = val;
                }
            }
        }
    }
    fclose(f);
    return s_cachedIniCombatScale;
}

static float s_cachedIniHudScale = -1.0f;

static float ReadIniHudScale() {
    if (s_cachedIniHudScale >= 0.0f) {
        return s_cachedIniHudScale;
    }
    InitTargetResolution();
    float autoScale = (g_targetHeight > 0) ? (1.50f * (float)g_targetHeight / 982.0f) : 1.50f;
    s_cachedIniHudScale = autoScale; // Default scaled-up HUD elements (+25% over vanilla 1.20x)
    const char* home = getenv("HOME");
    if (!home) return s_cachedIniHudScale;
    
    char path[1024];
    snprintf(path, sizeof(path), "%s/Library/Application Support/Knights of the Old Republic/swkotor.ini", home);
    FILE* f = fopen(path, "r");
    if (!f) return s_cachedIniHudScale;
    
    char line[256];
    bool inGraphics = false;
    while (fgets(line, sizeof(line), f)) {
        char* p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '[') {
            inGraphics = (strncmp(p, "[Graphics Options]", 18) == 0);
            continue;
        }
        if (inGraphics && (strncasecmp(p, "HudScale", 8) == 0 || strncasecmp(p, "PortraitScale", 13) == 0)) {
            char* eq = strchr(p, '=');
            if (eq) {
                float val = (float)atof(eq + 1);
                if (val >= 0.5f && val <= 5.0f) {
                    s_cachedIniHudScale = val;
                }
            }
        }
    }
    fclose(f);
    return s_cachedIniHudScale;
}

static bool ReadIniHDMenuTextures() {
    if (s_cachedIniHDMenuTextures >= 0) {
        return s_cachedIniHDMenuTextures != 0;
    }
    s_cachedIniHDMenuTextures = 0;
    const char* home = getenv("HOME");
    if (!home) return false;
    
    char path[1024];
    snprintf(path, sizeof(path), "%s/Library/Application Support/Knights of the Old Republic/swkotor.ini", home);
    FILE* f = fopen(path, "r");
    if (!f) return false;
    
    char line[256];
    bool inGraphics = false;
    while (fgets(line, sizeof(line), f)) {
        char* p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '[') {
            inGraphics = (strncmp(p, "[Graphics Options]", 18) == 0);
            continue;
        }
        if (inGraphics && strncasecmp(p, "HDMenuTextures", 14) == 0) {
            char* eq = strchr(p, '=');
            if (eq) {
                int val = atoi(eq + 1);
                s_cachedIniHDMenuTextures = val;
            }
        }
    }
    fclose(f);
    return s_cachedIniHDMenuTextures != 0;
}

/*
  writeMemInt:
  Safely updates an integer in executable (__TEXT) memory using Darwin Mach virtual memory protection.
  Creates a private copy-on-write page to allow dynamic runtime configuration.
*/
static void writeMemInt(uintptr_t addr, int val) {
    vm_address_t page = addr & ~0xFFF;
    kern_return_t kr = vm_protect(mach_task_self(), page, 0x2000, FALSE, 
                                  VM_PROT_READ | VM_PROT_WRITE | VM_PROT_COPY);
    if (kr == KERN_SUCCESS) {
        *(int*)addr = val;
        vm_protect(mach_task_self(), page, 0x2000, FALSE, 
                   VM_PROT_READ | VM_PROT_EXECUTE);
    }
}

static void writeMemByte(uintptr_t addr, uint8_t val) {
    vm_address_t page = addr & ~0xFFF;
    kern_return_t kr = vm_protect(mach_task_self(), page, 0x2000, FALSE, 
                                  VM_PROT_READ | VM_PROT_WRITE | VM_PROT_COPY);
    if (kr == KERN_SUCCESS) {
        *(uint8_t*)addr = val;
        vm_protect(mach_task_self(), page, 0x2000, FALSE, 
                   VM_PROT_READ | VM_PROT_EXECUTE);
    }
}

static void writeMemFloat(uintptr_t addr, float val) {
    vm_address_t page = addr & ~0xFFF;
    kern_return_t kr = vm_protect(mach_task_self(), page, 0x2000, FALSE, 
                                  VM_PROT_READ | VM_PROT_WRITE | VM_PROT_COPY);
    if (kr == KERN_SUCCESS) {
        *(float*)addr = val;
        vm_protect(mach_task_self(), page, 0x2000, FALSE, 
                   VM_PROT_READ | VM_PROT_EXECUTE);
    }
}

static void writeMemBytes(uintptr_t addr, const uint8_t* src, size_t len) {
    vm_address_t page = addr & ~0xFFF;
    kern_return_t kr = vm_protect(mach_task_self(), page, 0x2000, FALSE, 
                                  VM_PROT_READ | VM_PROT_WRITE | VM_PROT_COPY);
    if (kr == KERN_SUCCESS) {
        memcpy((void*)addr, src, len);
        vm_protect(mach_task_self(), page, 0x2000, FALSE, 
                   VM_PROT_READ | VM_PROT_EXECUTE);
    }
}

/*
  GetScaledItemRowHeight:
  Calculates the scaled row height for list items across menus.
  In vanilla KotOR, standard item rows are 56px (0x38) high, skill/save rows are 42px (0x2A) high,
  designed for a 480px vertical canvas.
  - Save & Load screen: 89px row height with 19px padding (108.5px stride, 6 visible slots).
  - In-Game Inventory & Equipment: 96px row height (5 visible slots, 197 image px stride matching background slots).
  - Container / Store Popups: 96px row height with dynamic root/child dialog scaling.
  - Skill rows: 86px row height (42 * scale).
*/
int GetScaledItemRowHeight(int baseHeight = 56) {
    float scale = (g_targetHeight > 0) ? ((float)g_targetHeight / 480.0f) : 1.0f;
    float menuScale = ReadIniMenuScale();
    if (menuScale > 0.0f) {
        scale = menuScale;
    }
    return (int)(baseHeight * scale + 0.5f);
}

static int s_lastPatchedTargetHeight = -1;

/*
  Calibrated Item List & Container Geometry:
  Hardcoded baseline calibrated at 982p display resolution (14" MacBook Pro native Retina scale)
  to achieve pixel-perfect alignment between the 5 item list slots and the purple etched background slots.
  
  Universal Resolution Scaling:
  Automatically scaled for any display resolution (1080p, 1440p, 4K, 720p) via GetScaledItemGeometry().
  No manual swkotor.ini entries are required — the patch works out of the box.
  
  Optional INI Overrides:
  If swkotor.ini contains a [UI Tuning] section, those values act as live baseline overrides for
  modding or experimentation, updating dynamically when switching tabs.
*/
struct UiTuningKnobs {
    int itemHeight = 108;
    int itemPadding = 6;
    int listHeight = 0; // 0 = automatic synchronization: (itemHeight + itemPadding) * 5 + 26
    int iconWidth = 117;
    int iconHeight = 117;
    int iconTopOffset = -5;
    int textOffset = 118;
    int textDeduct = 118;
    int listLeftOffset = -4;
    int listTopOffset = 4;
    int listWidthOffset = 6;
    int badgeOffset = 113;
    int badgeTopOffset = -8;
    int containerItemHeight = 110;
    int containerPadding = 6;
    int skillHeight = 42;
    int workbenchItemHeight = 56;
    int areaTransitionTextOffset = 0;
};

static UiTuningKnobs s_currentKnobs;
static time_t s_lastIniModTime = 0;
static int s_knobVersion = 0;

static bool CheckAndReloadUiKnobs() {
    const char* home = getenv("HOME");
    if (!home) return false;
    char path[1024];
    snprintf(path, sizeof(path), "%s/Library/Application Support/Knights of the Old Republic/swkotor.ini", home);
    
    struct stat st;
    if (stat(path, &st) != 0) return false;
    if (s_lastIniModTime != 0 && st.st_mtime == s_lastIniModTime) return false;
    
    s_lastIniModTime = st.st_mtime;
    
    FILE* f = fopen(path, "r");
    if (!f) return false;
    
    UiTuningKnobs newKnobs = s_currentKnobs;
    char line[256];
    bool inTuning = false;
    while (fgets(line, sizeof(line), f)) {
        char* p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '[') {
            inTuning = (strncasecmp(p, "[UI Tuning]", 11) == 0);
            continue;
        }
        if (inTuning) {
            char* eq = strchr(p, '=');
            if (!eq) continue;
            *eq = '\0';
            char* valStr = eq + 1;
            while (*valStr == ' ' || *valStr == '\t') valStr++;
            int val = atoi(valStr);
            
            char* endKey = eq - 1;
            while (endKey > p && (*endKey == ' ' || *endKey == '\t')) {
                *endKey = '\0';
                endKey--;
            }
            
            if (strcasecmp(p, "ItemHeight") == 0) newKnobs.itemHeight = val;
            else if (strcasecmp(p, "ItemPadding") == 0) newKnobs.itemPadding = val;
            else if (strcasecmp(p, "ListHeight") == 0) newKnobs.listHeight = val;
            else if (strcasecmp(p, "IconWidth") == 0) newKnobs.iconWidth = val;
            else if (strcasecmp(p, "IconHeight") == 0) newKnobs.iconHeight = val;
            else if (strcasecmp(p, "IconTopOffset") == 0) newKnobs.iconTopOffset = val;
            else if (strcasecmp(p, "TextOffset") == 0) newKnobs.textOffset = val;
            else if (strcasecmp(p, "TextDeduct") == 0) newKnobs.textDeduct = val;
            else if (strcasecmp(p, "ListLeftOffset") == 0) newKnobs.listLeftOffset = val;
            else if (strcasecmp(p, "ListTopOffset") == 0) newKnobs.listTopOffset = val;
            else if (strcasecmp(p, "ListWidthOffset") == 0) newKnobs.listWidthOffset = val;
            else if (strcasecmp(p, "BadgeOffset") == 0) newKnobs.badgeOffset = val;
            else if (strcasecmp(p, "BadgeTopOffset") == 0 || strcasecmp(p, "BadgeYOffset") == 0) newKnobs.badgeTopOffset = val;
            else if (strcasecmp(p, "ContainerItemHeight") == 0) newKnobs.containerItemHeight = val;
            else if (strcasecmp(p, "ContainerPadding") == 0) newKnobs.containerPadding = val;
            else if (strcasecmp(p, "SkillHeight") == 0) newKnobs.skillHeight = val;
            else if (strcasecmp(p, "WorkbenchItemHeight") == 0) newKnobs.workbenchItemHeight = val;
            else if (strcasecmp(p, "AreaTransitionTextOffset") == 0) newKnobs.areaTransitionTextOffset = val;
        }
    }
    fclose(f);
    
    if (memcmp(&newKnobs, &s_currentKnobs, sizeof(UiTuningKnobs)) != 0 || s_knobVersion == 0) {
        s_currentKnobs = newKnobs;
        s_knobVersion++;
        s_scaledPanels.clear();
        return true;
    }
    return false;
}

struct ScaledItemGeometry {
    int itemHeight;
    int itemPadding;
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
    int containerItemHeight;
    int containerPadding;
};

/*
  GetScaledItemGeometry:
  Calculates resolution-scaled item and listbox dimensions.
  Calibrated baseline reference: targetHeight = 982 (14" MacBook Pro native Retina display).
  At 982p, resScale == 1.0f (exact calibrated values).
  On any other display resolution (e.g. 1080p, 1440p, 4K, 720p), scales all coordinates
  proportionally to preserve pixel-perfect alignment with the scaled background slots.
*/
static ScaledItemGeometry GetScaledItemGeometry(int currentTargetH) {
    float scale = (currentTargetH > 0) ? ((float)currentTargetH / 480.0f) : 1.0f;
    float resScale = (currentTargetH > 0) ? ((float)currentTargetH / 982.0f) : 1.0f;
    ScaledItemGeometry g;
    g.itemHeight = (int)(s_currentKnobs.itemHeight * resScale + 0.5f);
    g.itemPadding = (int)(s_currentKnobs.itemPadding * resScale + 0.5f);
    int stride = g.itemHeight + g.itemPadding;
    int autoListHeight = stride * 5 + (int)(26 * resScale + 0.5f);
    if (s_currentKnobs.listHeight > 0 && s_currentKnobs.listHeight != 505) {
        g.listHeight = (int)(s_currentKnobs.listHeight * resScale + 0.5f);
    } else {
        g.listHeight = autoListHeight;
    }
    g.iconWidth = (int)(s_currentKnobs.iconWidth * resScale + 0.5f);
    g.iconHeight = (int)(s_currentKnobs.iconHeight * resScale + 0.5f);
    g.iconTopOffset = (int)(s_currentKnobs.iconTopOffset * resScale + (s_currentKnobs.iconTopOffset < 0 ? -0.5f : 0.5f));
    g.textOffset = (int)(s_currentKnobs.textOffset * resScale + 0.5f);
    g.textDeduct = (int)(s_currentKnobs.textDeduct * resScale + 0.5f);
    g.listLeftOffset = (int)(s_currentKnobs.listLeftOffset * resScale + (s_currentKnobs.listLeftOffset < 0 ? -0.5f : 0.5f));
    g.listTopOffset = (int)(s_currentKnobs.listTopOffset * resScale + (s_currentKnobs.listTopOffset < 0 ? -0.5f : 0.5f));
    g.listWidthOffset = (int)(s_currentKnobs.listWidthOffset * resScale + (s_currentKnobs.listWidthOffset < 0 ? -0.5f : 0.5f));
    g.badgeOffset = (int)(s_currentKnobs.badgeOffset * resScale + 0.5f);
    g.badgeTopOffset = (int)(s_currentKnobs.badgeTopOffset * resScale + (s_currentKnobs.badgeTopOffset < 0 ? -0.5f : 0.5f));
    g.containerItemHeight = (int)(56.0f * scale + 0.5f);
    g.containerPadding = (int)(s_currentKnobs.containerPadding * resScale + 0.5f);
    return g;
}

/*
  Area Map Dynamic Coordinate Scaling:
  In vanilla KotOR, WorldToMapCoords (0x1004400d2) and GetPlayerMapCoords (0x100440300)
  map 3D world coordinates into a 440x256 map space.
  When the map viewport is scaled to fill vertical screen height
  (e.g. 2.50x on 1200p, 2.045x on 982p), the player direction arrow, party member markers,
  and map note bullseyes must have their coordinates dynamically scaled by the exact same
  ratio so they align with the illuminated room geometry.
  
  Minimap calls to GetPlayerMapCoords originate from 0x10023790c (outside CSWGuiMapHider::Draw),
  so they remain untouched.
*/
typedef int (*WorldToMapCoordsFn)(void* pMapInfo, double xy, float z, int* outX, int* outY);
typedef int (*GetPlayerMapCoordsFn)(char* pMapInfo, int partyIdx, int* outX, int* outY);

static const WorldToMapCoordsFn Orig_WorldToMapCoords = (WorldToMapCoordsFn)0x1004400d2;
static const GetPlayerMapCoordsFn Orig_GetPlayerMapCoords = (GetPlayerMapCoordsFn)0x100440300;

extern "C" int MapHider_WorldToMapCoords(void* pMapInfo, double xy, float z, int* outX, int* outY) {
    int res = Orig_WorldToMapCoords(pMapInfo, xy, z, outX, outY);
    if (res && outX && outY) {
        float scale = (g_targetHeight > 0) ? ((float)g_targetHeight / 480.0f) : 1.0f;
        float menuScale = ReadIniMenuScale();
        if (menuScale > 0.0f) scale = menuScale;
        *outX = (int)(*outX * scale + 0.5f);
        *outY = (int)(*outY * scale + 0.5f);
    }
    return res;
}

extern "C" int MapHider_GetPlayerMapCoords(char* pMapInfo, int partyIdx, int* outX, int* outY) {
    int res = Orig_GetPlayerMapCoords(pMapInfo, partyIdx, outX, outY);
    if (res && outX && outY) {
        float scale = (g_targetHeight > 0) ? ((float)g_targetHeight / 480.0f) : 1.0f;
        float menuScale = ReadIniMenuScale();
        if (menuScale > 0.0f) scale = menuScale;
        *outX = (int)(*outX * scale + 0.5f);
        *outY = (int)(*outY * scale + 0.5f);
    }
    return res;
}

/*
  refreshPatchedListConstants:
  Dynamically writes the calculated scaled row heights into the game's item, skill, and
  save entry layout routines in memory. Also dynamically updates quantity badge constants
  to position and size item stack count numbers properly on scaled icons.
*/
void refreshPatchedListConstants() {
    InitTargetResolution();
    float scale = (g_targetHeight > 0) ? ((float)g_targetHeight / 480.0f) : 1.0f;
    float menuScale = ReadIniMenuScale();
    if (menuScale > 0.0f) {
        scale = menuScale;
    }

    CheckAndReloadUiKnobs();

    ScaledItemGeometry geom = GetScaledItemGeometry(g_targetHeight);

    int itemHeight = geom.itemHeight;
    int containerItemHeight = geom.containerItemHeight;
    int iconWidth = geom.iconWidth;
    int iconHeight = geom.iconHeight;
    int iconTopOffset = geom.iconTopOffset;
    int textOffset = geom.textOffset;
    int textDeduct = geom.textDeduct;
    int skillHeight = s_currentKnobs.skillHeight;
    int saveHeight = (int)(43.5f * scale + 0.5f);
    int qBadgeX = geom.badgeOffset;
    
    int qHeight = (int)(18 * scale + 0.5f);
    int qTop = itemHeight - qHeight + geom.badgeTopOffset;
    int qWidthShort = (int)(20 * scale + 0.5f);
    int qWidthLong = (int)(38 * scale + 0.5f);
    
    int workbenchItemHeight = s_currentKnobs.workbenchItemHeight;
    writeMemInt(0x1002be874, itemHeight);            // CSWGuiInGameItemEntry::Layout
    writeMemInt(0x1002bff71, containerItemHeight);   // CSWGuiContainerItemEntry::Layout
    writeMemInt(0x10021c48f, workbenchItemHeight);   // CSWUpgradeItemEntry::Layout (matches 56px vanilla row height)
    writeMemInt(0x1002c4758, containerItemHeight);   // Movies List Layout (immediate starts at 0x1002c4758)
    writeMemInt(0x10022f60f, skillHeight);           // CSWGuiInGameSkillEntry
    writeMemInt(0x1002ff3cf, saveHeight);            // CSWGuiSaveLoad::PopulateGameList
    
    // Scale In-Game Item Entry icon geometry independently (CSWGuiInGameItemEntry::SetExtent at 0x1002be42b)
    // Controls icon texture (0x250), neon arch border (0x2d8), and highlight (0x360)
    uint8_t itemIconHook[98] = {
        0x44, 0x8b, 0x7b, 0x08,
        0x44, 0x8b, 0x63, 0x0c,
        0x41, 0x8d, 0x54, 0x24, (uint8_t)(iconTopOffset & 0xff),
        0xb8, 0x00, 0x00, 0x00, 0x00,
        0xb9, 0x00, 0x00, 0x00, 0x00,
        0x44, 0x89, 0xbb, 0x50, 0x02, 0x00, 0x00,
        0x89, 0x93, 0x54, 0x02, 0x00, 0x00,
        0x89, 0x83, 0x58, 0x02, 0x00, 0x00,
        0x89, 0x8b, 0x5c, 0x02, 0x00, 0x00,
        0x44, 0x89, 0xbb, 0xd8, 0x02, 0x00, 0x00,
        0x89, 0x93, 0xdc, 0x02, 0x00, 0x00,
        0x89, 0x83, 0xe0, 0x02, 0x00, 0x00,
        0x89, 0x8b, 0xe4, 0x02, 0x00, 0x00,
        0x44, 0x89, 0xbb, 0x60, 0x03, 0x00, 0x00,
        0x89, 0x93, 0x64, 0x03, 0x00, 0x00,
        0x89, 0x83, 0x68, 0x03, 0x00, 0x00,
        0x89, 0x8b, 0x6c, 0x03, 0x00, 0x00
    };
    *(int*)&itemIconHook[14] = iconWidth;
    *(int*)&itemIconHook[19] = iconHeight;
    writeMemBytes(0x1002be42b, itemIconHook, 98);
    writeMemInt(0x1002be4b8, qBadgeX);
    
    // Scale In-Game Item Entry text & highlight block (0x1002be4da):
    // Directly sets text box & highlight height to itemHeight using 32-bit arithmetic (no sign-extension bugs)
    uint8_t itemTextHook[64] = {
        0x41, 0x81, 0xc7, 0x00, 0x00, 0x00, 0x00, // addl $textOffset, %r15d (6 bytes)
        0x8b, 0x43, 0x10,                         // movl 0x10(%rbx), %eax (3 bytes)
        0x2d, 0x00, 0x00, 0x00, 0x00,             // subl $textDeduct, %eax (5 bytes)
        0x41, 0xbd, 0x00, 0x00, 0x00, 0x00,       // movl $itemHeight, %r13d (6 bytes)
        0x44, 0x89, 0xbb, 0xb0, 0x00, 0x00, 0x00, // movl %r15d, 0xb0(%rbx) (7 bytes)
        0x44, 0x89, 0xa3, 0xb4, 0x00, 0x00, 0x00, // movl %r12d, 0xb4(%rbx) (7 bytes)
        0x89, 0x83, 0xb8, 0x00, 0x00, 0x00,       // movl %eax, 0xb8(%rbx) (6 bytes)
        0x44, 0x89, 0xab, 0xbc, 0x00, 0x00, 0x00, // movl %r13d, 0xbc(%rbx) (7 bytes)
        0x0f, 0x10, 0x83, 0xb0, 0x00, 0x00, 0x00, // movups 0xb0(%rbx), %xmm0 (7 bytes)
        0x0f, 0x11, 0x83, 0x38, 0x01, 0x00, 0x00, // movups %xmm0, 0x138(%rbx) (7 bytes)
        0x90, 0x90                                // 2 nops (2 bytes)
    };
    *(int*)&itemTextHook[3] = textOffset;
    *(int*)&itemTextHook[11] = textDeduct;
    *(int*)&itemTextHook[17] = itemHeight;
    writeMemBytes(0x1002be4da, itemTextHook, 64);
    
    // Prevent CSWGuiListBox::RecalculateItemHeight (0x1004a9554 & 0x1004a959c)
    // from overwriting m_itemHeight (0x368) with the prototype 70px height
    uint8_t nops6[6] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
    writeMemBytes(0x1004a9554, nops6, 6);
    writeMemBytes(0x1004a959c, nops6, 6);
    
    // Scale item quantity badge constants (CSWGuiInGameItemEntry)
    writeMemInt(0x1002be4a4, qWidthLong);
    writeMemInt(0x1002be4a9, qWidthShort);
    writeMemByte(0x1002be4c7, (uint8_t)(qTop > 255 ? 255 : (qTop < 0 ? 0 : qTop)));
    writeMemInt(0x1002be4ce, qHeight);
    
    // Scale CSWGuiStoreItemEntry::SetExtent (0x1002bfb06) for Store & Container lists:
    // Sets %r13d to containerItemHeight (icon width/height, arch border, and text button height)
    writeMemInt(0x1002bfb4b, containerItemHeight);
    
    // Text button start X = rowX + %r13d (0x1002bfbe2)
    uint8_t storeBtnXHook[4] = { 0x45, 0x01, 0xef, 0x90 }; // addl %r13d, %r15d; nop
    writeMemBytes(0x1002bfbe2, storeBtnXHook, 4);

    // Text button width = rowW - %r13d (0x1002bfbe9)
    uint8_t storeBtnWHook[3] = { 0x44, 0x29, 0xe8 }; // subl %r13d, %eax
    writeMemBytes(0x1002bfbe9, storeBtnWHook, 3);

    // Badge X = rowX + %r13d - badgeW (0x1002bfbbf)
    uint8_t storeBadgeXHook[5] = { 0x44, 0x89, 0xe8, 0x90, 0x90 }; // movl %r13d, %eax; nop; nop
    writeMemBytes(0x1002bfbbf, storeBadgeXHook, 5);

    // Badge Y = rowY + (containerItemHeight - qHeight) (0x1002bfbcf)
    int qTopStore = containerItemHeight - qHeight;
    writeMemByte(0x1002bfbcf, (uint8_t)(qTopStore > 255 ? 255 : (qTopStore < 0 ? 0 : qTopStore)));
    writeMemInt(0x1002bfbd6, qHeight);
    writeMemInt(0x1002bfbac, qWidthLong);
    writeMemInt(0x1002bfbb1, qWidthShort);
    
    // FixMessageLabel / CSWGuiMessageBox fit ceilings and icon inset
    writeMemInt(0x1003065a2, (int)(34 * scale + 0.5f));
    writeMemInt(0x100306879, g_targetWidth);
    writeMemInt(0x100306881, g_targetHeight);
    writeMemInt(0x10030688d, g_targetWidth);
    writeMemInt(0x1003068ff, g_targetHeight);

    // CSWGuiMapHider::Draw: scale fog tile step X from this->width (0x10(%r12)) instead of hardcoded 440.0f
    uint8_t mapHiderWidthHook[8] = { 0xf3, 0x41, 0x0f, 0x2a, 0x4c, 0x24, 0x10, 0x90 };
    writeMemBytes(0x1002b4ce9, mapHiderWidthHook, 8);

    // CSWGuiMapHider::Draw: scale fog tile step Y from this->height (0x14(%r12)) instead of hardcoded 256.0f
    uint8_t mapHiderHeightHook[8] = { 0xf3, 0x41, 0x0f, 0x2a, 0x54, 0x24, 0x14, 0x90 };
    writeMemBytes(0x1002b4cfc, mapHiderHeightHook, 8);

    // Restore 0x1004a1f3d to vanilla (9 bytes: movl 0x10(%r15), %r11d; leal (%r13,%r13), %eax)
    uint8_t vanillaBorderF3D[9] = { 0x45, 0x8b, 0x5f, 0x10, 0x43, 0x8d, 0x44, 0x2d, 0x00 };
    writeMemBytes(0x1004a1f3d, vanillaBorderF3D, 9);

    // Stretch Fill Stub at 0x1000f4f90 (37 bytes)
    // Eliminates the "four mini box borders per item icon" bug across Inventory, Equipment, and Store lists.
    // In vanilla KotOR, item icon arch borders ("lbl_hex_3") are assigned as pure fill textures
    // with NO corner textures (0x70(%r15) == NULL) and fillStyle == 0 (tile mode).
    // When icons are scaled above their 56x56 texture size (e.g. 105px+ or 140px on widescreen),
    // the tile mode function (0x1004a23ca) divides width and height by 56, resulting in a 2x2 grid
    // of 4 mini hexagon boxes!
    // This stub intercepts CSWGuiBorder::Draw fill style dispatch at 0x1004a2350:
    // If the border has no corners (0x70(%r15) == NULL) and is an icon/slot frame (width <= 400 && height <= 400),
    // or if fillStyle == 2, it branches directly to 0x1004a2376 (DrawStretched), rendering the arch texture as a single
    // seamlessly scaled border with ZERO tiling and ZERO mini-boxes!
    // Regular window frames with corner textures (0x70(%r15) != NULL) branch to 0x1004a2355 (vanilla).
    uint8_t stretchFillStub[37] = {
        0x80, 0xf9, 0x02,                               // cmpb $0x2, %cl
        0x74, 0x16,                                     // je take_stretch (offset 0x1b)
        0x49, 0x83, 0x7f, 0x70, 0x00,                   // cmpq $0x0, 0x70(%r15)
        0x75, 0x14,                                     // jne vanilla_tile (offset 0x20)
        0x3d, 0x90, 0x01, 0x00, 0x00,                   // cmpl $400, %eax
        0x77, 0x0d,                                     // ja vanilla_tile (offset 0x20)
        0x81, 0xfb, 0x90, 0x01, 0x00, 0x00,             // cmpl $400, %ebx
        0x77, 0x05,                                     // ja vanilla_tile (offset 0x20)
        // take_stretch (offset 0x1b -> 0x1000f4fab):
        0xe9, 0xc6, 0xd3, 0x3a, 0x00,                   // jmp 0x1004a2376
        // vanilla_tile (offset 0x20 -> 0x1000f4fb0):
        0xe9, 0xa0, 0xd3, 0x3a, 0x00                    // jmp 0x1004a2355
    };
    writeMemBytes(0x1000f4f90, stretchFillStub, 37);

    // Hook at 0x1004a2350 in CSWGuiBorder::Draw: jmp 0x1000f4f90 (5 bytes)
    // Replaces: cmpb $0x2, %cl (3 bytes: 80 f9 02); je 0x1004a2376 (2 bytes: 74 21)
    uint8_t stretchFillHook[5] = {
        0xe9, 0x3b, 0x2c, 0xc5, 0xff                    // jmp 0x1000f4f90
    };
    writeMemBytes(0x1004a2350, stretchFillHook, 5);

    // CSWGuiInGameItemEntry::Init: pass fillStyle = 2 (STRETCH) to border constructors
    // Replaces `xorl %eax, %eax; pushq %rax` (31 c0 50) with `pushq $2; nop` (6a 02 90)
    uint8_t itemInitBorder[3] = { 0x6a, 0x02, 0x90 };
    writeMemBytes(0x1002be754, itemInitBorder, 3); // Unselected border
    writeMemBytes(0x1002be80c, itemInitBorder, 3); // Selected border

    // CSWGuiStoreItemEntry::Init: pass fillStyle = 2 (STRETCH) to border constructors
    writeMemBytes(0x1002bfe55, itemInitBorder, 3); // Unselected border
    writeMemBytes(0x1002bff0e, itemInitBorder, 3); // Selected border

    // CSWGuiMapHider::Draw dynamic coordinate scaling hooks:
    // Bridge 1 at 0x1000f4f68: jump to MapHider_WorldToMapCoords (12 bytes)
    uint8_t bridgeWorld[12] = { 0x48, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xe0 };
    *(void**)&bridgeWorld[2] = (void*)&MapHider_WorldToMapCoords;
    writeMemBytes(0x1000f4f68, bridgeWorld, 12);

    // Bridge 2 at 0x1000f4f78: jump to MapHider_GetPlayerMapCoords (12 bytes)
    uint8_t bridgePlayer[12] = { 0x48, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xe0 };
    *(void**)&bridgePlayer[2] = (void*)&MapHider_GetPlayerMapCoords;
    writeMemBytes(0x1000f4f78, bridgePlayer, 12);

    // Patch callsites in CSWGuiMapHider::Draw:
    // 0x1002b4fca: callq 0x1000f4f68 (Map notes / quest markers)
    uint8_t callWorld[5] = { 0xe8, 0x99, 0xff, 0xe3, 0xff };
    writeMemBytes(0x1002b4fca, callWorld, 5);

    // 0x1002b541b: callq 0x1000f4f78 (Party members)
    uint8_t callParty[5] = { 0xe8, 0x58, 0xfb, 0xe3, 0xff };
    writeMemBytes(0x1002b541b, callParty, 5);

    // 0x1002b54c2: callq 0x1000f4f78 (Player arrow)
    uint8_t callPlayer[5] = { 0xe8, 0xb1, 0xfa, 0xe3, 0xff };
    writeMemBytes(0x1002b54c2, callPlayer, 5);

    // Restore vanilla marker centering arithmetic in CSWGuiMapHider::Draw
    uint8_t vanillaNoteUnsel[] = {
        0x83, 0xc0, 0xf9, 0x89, 0x85, 0x40, 0xfe, 0xff, 0xff,
        0x8b, 0x85, 0x2c, 0xfe, 0xff, 0xff, 0x83, 0xc0, 0xf9,
        0x89, 0x85, 0x44, 0xfe, 0xff, 0xff, 0xb8, 0x0e, 0x00,
        0x00, 0x00, 0x89, 0x85, 0x48, 0xfe, 0xff, 0xff, 0x89,
        0x85, 0x4c, 0xfe, 0xff, 0xff
    };
    writeMemBytes(0x1002b4ff2, vanillaNoteUnsel, sizeof(vanillaNoteUnsel));

    uint8_t vanillaNoteSel[] = {
        0x83, 0xc0, 0xf6, 0x89, 0x85, 0x40, 0xfe, 0xff, 0xff,
        0x8b, 0x85, 0x2c, 0xfe, 0xff, 0xff, 0x83, 0xc0, 0xf6,
        0x89, 0x85, 0x44, 0xfe, 0xff, 0xff, 0xb8, 0x14, 0x00,
        0x00, 0x00, 0x89, 0x85, 0x48, 0xfe, 0xff, 0xff, 0x89,
        0x85, 0x4c, 0xfe, 0xff, 0xff
    };
    writeMemBytes(0x1002b52b2, vanillaNoteSel, sizeof(vanillaNoteSel));

    uint8_t vanillaParty[] = {
        0x8b, 0x85, 0x58, 0xfe, 0xff, 0xff, 0x83, 0xc0, 0xf8,
        0x89, 0x85, 0x40, 0xfe, 0xff, 0xff, 0x8b, 0x85, 0x5c,
        0xfe, 0xff, 0xff, 0x83, 0xc0, 0xf8, 0x89, 0x85, 0x44,
        0xfe, 0xff, 0xff, 0xb8, 0x10, 0x00, 0x00, 0x00, 0x89,
        0x85, 0x48, 0xfe, 0xff, 0xff, 0x89, 0x85, 0x4c, 0xfe,
        0xff, 0xff
    };
    writeMemBytes(0x1002b5430, vanillaParty, sizeof(vanillaParty));

    uint8_t vanillaArrow[] = {
        0x8b, 0xbd, 0x58, 0xfe, 0xff, 0xff, 0x83, 0xc7, 0xf0,
        0x89, 0xbd, 0x40, 0xfe, 0xff, 0xff, 0x8b, 0xb5, 0x5c,
        0xfe, 0xff, 0xff, 0x83, 0xc6, 0xf0, 0x89, 0xb5, 0x44,
        0xfe, 0xff, 0xff, 0xb8, 0x20, 0x00, 0x00, 0x00, 0x89,
        0x85, 0x48, 0xfe, 0xff, 0xff, 0x89, 0x85, 0x4c, 0xfe,
        0xff, 0xff
    };
    writeMemBytes(0x1002b54cf, vanillaArrow, sizeof(vanillaArrow));

    s_lastPatchedTargetHeight = g_targetHeight;
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
  the engine's rendering (CSWGuiWindow::Draw / ScreenToClient), mouse input routines
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
    if (s_lastPatchedCenteringW == targetWidth && s_lastPatchedCenteringH == targetHeight) return;
    
    // Width displacements (-targetWidth)
    writeMemInt(0x10049dcca, -targetWidth);
    writeMemInt(0x10049dcd4, -targetWidth);
    writeMemInt(0x10049ddb2, -targetWidth);
    writeMemInt(0x10049ddbc, -targetWidth);
    writeMemInt(0x10049e855, -targetWidth);
    writeMemInt(0x10049e85f, -targetWidth);
    writeMemInt(0x1002b4b3b, -targetWidth);
    writeMemInt(0x1002b4b46, -targetWidth);
    writeMemInt(0x1002b5615, -targetWidth);
    writeMemInt(0x1002b561f, -targetWidth);
    
    // Height displacements (-targetHeight)
    writeMemInt(0x10049dce8, -targetHeight);
    writeMemInt(0x10049dcf2, -targetHeight);
    writeMemInt(0x10049ddd9, -targetHeight);
    writeMemInt(0x10049dde3, -targetHeight);
    writeMemInt(0x10049e872, -targetHeight);
    writeMemInt(0x10049e87c, -targetHeight);
    writeMemInt(0x1002b4b59, -targetHeight);
    writeMemInt(0x1002b4b63, -targetHeight);
    writeMemInt(0x1002b5631, -targetHeight);
    writeMemInt(0x1002b563b, -targetHeight);
    
    // Bypass vanilla CSWGuiClassSelection::Update loop (0x100337897 -> jmp 0x100337b38)
    // Ensures the vanilla mutating delta animation math is disabled, preventing rapid flashing
    uint8_t bypassClassUpdateLoop[5] = { 0xe9, 0x9c, 0x02, 0x00, 0x00 };
    writeMemBytes(0x100337897, bypassClassUpdateLoop, 5);
    
    s_lastPatchedCenteringW = targetWidth;
    s_lastPatchedCenteringH = targetHeight;
}

struct PopupSnapshot {
    Rect root;
    std::vector<Rect> children;
    int targetHeight;
    int lastKnobVersion;
};
static std::unordered_map<void*, PopupSnapshot> s_popupSnapshots;

/*
  scalePopupPanel:
  Dynamically scales popup dialog windows (CSWGuiContainer, CSWGuiMessageBox,
  CSWGuiStatusSummary, CSWGuiInGamePause, CSWGuiTutorialBox, etc.;
  CSWGuiBarkBubble is excluded to preserve native 3D camera-projected world coordinates).
  
  Port of WindowsScaledKotor "Scaled Popups" and "Scaled Container Popup" patches.
  
  Mechanics:
  1. Popups have small authored extents (e.g. 240x260 for containers).
  2. Scales the root extent and all child controls proportionally by:
       scale = (float)g_targetHeight / 480.0f  (or user-configured MenuScale)
  3. Centers the scaled popup dialog cleanly on screen:
       targetLeft = (g_targetWidth - targetW) / 2
       targetTop  = (g_targetHeight - targetH) / 2
  4. Resizes the glowing window border at panel + 0x70 to { 0, 0, targetW, targetH }.
  5. Configures panel flags at panel + 0x5c:
       panel[0x5c] = (panel[0x5c] & ~0x60) | 0x1;
     Bit 0x1 tells CSWGuiPanel::HandleMouseInput and GetLocalMousePos to use client-relative
     coordinates via ScreenToClient (subtracting targetLeft and targetTop).
     Clearing bits 0x20 and 0x40 (0x60) prevents the engine from subtracting the large
     4:3 full-screen menu centering offset.
     Result: 0-pixel click error on all popup buttons, items, and scrollbars!
*/
void scalePopupPanel(char* panel) {
    if (!panel || !is_readable(panel)) return;
    
    refreshPatchedListConstants();
    
    Rect* rect = (Rect*)(panel + 0x8);
    if (!rect || rect->width <= 0 || rect->height <= 0 || rect->width >= 8192 || rect->height >= 8192) {
        return;
    }
    
    float scale = (g_targetHeight > 0) ? ((float)g_targetHeight / 480.0f) : 1.0f;
    float menuScale = ReadIniMenuScale();
    if (menuScale > 0.0f) {
        scale = menuScale;
    }
    
    void* vtable = *(void**)panel;
    bool isMsgBox = (vtable == (void*)0x1005a8c60 || vtable == (void*)0x1005ae880 ||
                     vtable == (void*)0x1005a5cb8 || vtable == (void*)0x1005ae9a0 ||
                     vtable == (void*)0x1005abea0 || vtable == (void*)0x1005aeaa8);
    
    if (isMsgBox) {
        int targetLeft = (g_targetWidth - rect->width) / 2;
        int targetTop  = (g_targetHeight - rect->height) / 2;
        if (rect->left == targetLeft && rect->top == targetTop) {
            return;
        }
        // Centering the message box window on screen without altering child controls.
        // The engine's FixMessageLabel already sizes the window, insets text by 70px below
        // the icon, centers text, and places OK/Cancel buttons.
        Rect centered = { targetLeft, targetTop, rect->width, rect->height };
        SetControlRect(panel, centered);
        char* border = *(char**)(panel + 0x70);
        if (border && is_readable(border)) {
            Rect borderRect = { 0, 0, rect->width, rect->height };
            SetControlRect(border, borderRect);
        }
        panel[0x5c] = (panel[0x5c] & ~0x60) | 0x1;
        return;
    }
    
    // Check if popup root is already scaled to target dimensions.
    // If so, return early to prevent resetting CSWGuiListBox scroll offsets!
    auto it = s_popupSnapshots.find(panel);
    if (it != s_popupSnapshots.end() && it->second.targetHeight == g_targetHeight && it->second.lastKnobVersion == s_knobVersion) {
        int curTargetW = (int)(it->second.root.width * scale + 0.5f);
        int curTargetH = (int)(it->second.root.height * scale + 0.5f);
        if (rect->width == curTargetW && rect->height == curTargetH) {
            return;
        }
    }
    
    // Capture or refresh unscaled popup geometry when popup opens with vanilla dimensions
    if (rect->width <= (g_targetWidth * 3) / 4) {
        PopupSnapshot snap;
        snap.root = *rect;
        snap.targetHeight = g_targetHeight;
        snap.lastKnobVersion = s_knobVersion;
        int numControls = *(int*)(panel + 0x38);
        char** controls = *(char***)(panel + 0x30);
        if (controls && is_readable(controls) && numControls > 0 && numControls <= 1024) {
            for (int i = 0; i < numControls; i++) {
                if (controls[i] && is_readable(controls[i])) {
                    snap.children.push_back(*(Rect*)(controls[i] + 0x8));
                } else {
                    snap.children.push_back({0, 0, 0, 0});
                }
            }
        }
        s_popupSnapshots[panel] = snap;
        it = s_popupSnapshots.find(panel);
    }
    
    if (it == s_popupSnapshots.end()) {
        return;
    }
    it->second.lastKnobVersion = s_knobVersion;
    
    const PopupSnapshot& snap = it->second;
    int targetW = (int)(snap.root.width * scale + 0.5f);
    int targetH = (int)(snap.root.height * scale + 0.5f);
    int targetLeft = (g_targetWidth - targetW) / 2;
    int targetTop = (g_targetHeight - targetH) / 2;
    
    // Scale root extent
    Rect scaledRoot = { targetLeft, targetTop, targetW, targetH };
    SetControlRect(panel, scaledRoot);
    
    // Scale border frame
    char* border = *(char**)(panel + 0x70);
    if (border && is_readable(border)) {
        Rect borderRect = { 0, 0, targetW, targetH };
        SetControlRect(border, borderRect);
    }
    
    // Scale all child controls
    int numControls = *(int*)(panel + 0x38);
    char** controls = *(char***)(panel + 0x30);
    if (controls && is_readable(controls) && numControls > 0) {
        int count = std::min((int)snap.children.size(), numControls);
        for (int i = 0; i < count; i++) {
            if (!controls[i] || !is_readable(controls[i])) continue;
            const Rect& r = snap.children[i];
            if (r.width > 0 && r.height > 0) {
                Rect s;
                s.left   = (int)(r.left * scale + 0.5f);
                s.top    = (int)(r.top * scale + 0.5f);
                s.width  = (int)(r.width * scale + 0.5f);
                s.height = (int)(r.height * scale + 0.5f);
                
                void* ctrlVtable = *(void**)controls[i];
                if (ctrlVtable == (void*)0x1005b4318) {
                    if (*(int*)(controls[i] + 0x168) != 0) {
                        *(int*)(controls[i] + 0x168) = (int)(16 * scale + 0.5f);
                    }
                    if (vtable == (void*)0x1005ab758) {
                        // CSWGuiContainer: set scaled cell spacing between container items
                        *(uint8_t*)(controls[i] + 0x373) = (uint8_t)s_currentKnobs.containerPadding;
                    }
                }
                if (vtable == (void*)0x1005a67c0 && (controls[i] == (panel + 0x3b0) || (r.top == 9 && r.height == 20))) {
                    // CSWGuiInGameAreaTransition (areatransition.gui): LBL_DESCRIPTION
                    // In vanilla 480p, the background pill (LBL_TEXTBG) is 32px tall and text is authored at top: 9.
                    // Because bitmap fonts (fnt_d16x16, ~16px) do not scale with resolution, scaling top linearly
                    // leaves the text shifted significantly towards the top of the vertically expanded pill.
                    // Center the text vertically inside the scaled background pill:
                    int bgHeight = 32;
                    for (size_t ci = 0; ci < snap.children.size(); ci++) {
                        if (snap.children[ci].top == 0 && snap.children[ci].height > 0 && snap.children[ci].width >= 350) {
                            bgHeight = snap.children[ci].height;
                            break;
                        }
                    }
                    int scaledBgHeight = (int)(bgHeight * scale + 0.5f);
                    s.top = (scaledBgHeight - 16) / 2 + s_currentKnobs.areaTransitionTextOffset;
                    if (s.top + s.height > scaledBgHeight) {
                        s.height = scaledBgHeight - s.top;
                    }
                }
                SetControlRect(controls[i], s);
            }
        }
    }
    
    // Set bit 0x1 (client-relative mouse handling) and clear 0x60 (centering flags)
    panel[0x5c] = (panel[0x5c] & ~0x60) | 0x1;
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
       0x20 | 0x40 (0x60). CSWGuiWindow::Draw centers them at (screenWidth - targetWidth)/2,
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
    float scale = (float)targetH / 480.0f;
    int boxW = (int)(75.2f * scale + 0.5f);
    int boxH = (int)(193.0f * scale + 0.5f);
    int boxTop = (int)(128.0f * scale + 0.5f);
    
    // Base 640x480 slot left coordinates (matches 800x600 BaseBoxLefts { 77, 190, 299, 406, 516, 625 })
    const float baseLefts[6] = { 61.6f, 152.0f, 239.2f, 324.8f, 412.8f, 500.0f };
    
    int totalSpan = (int)((500.0f - 61.6f) * scale + 0.5f) + boxW;
    int firstLeft = (int)(baseLefts[0] * scale + 0.5f);
    int rightEdge = firstLeft + totalSpan;
    int centerAdjustment = (targetW - rightEdge - firstLeft) / 2;
    
    int left = (int)(baseLefts[slotIndex] * scale + 0.5f) + centerAdjustment;
    
    Rect r;
    r.left = left;
    r.top = boxTop;
    r.width = boxW;
    r.height = boxH;
    return r;
}

struct SmallPanelSnapshot {
    void* vtable;
    Rect root;
    struct ChildSnap {
        char* control;
        Rect rect;
    };
    std::vector<ChildSnap> children;
    int targetHeight;
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
    
    // Clear snapshot cache if screen resolution changed
    if (s_lastScaleTargetHeight != g_targetHeight) {
        s_smallPanelSnapshots.clear();
    }
    
    // Target dimensions for uniform 4:3 menu canvas
    int targetH = g_targetHeight;
    int targetW = (g_targetHeight * 4) / 3;
    float menuScale = ReadIniMenuScale();
    if (menuScale > 0.0f) {
        targetH = (int)(480 * menuScale);
        targetW = (int)(640 * menuScale);
    }
    
    // Capture unscaled snapshot if new instance or vanilla dimensions (width <= 350)
    auto it = s_smallPanelSnapshots.find(panel);
    if (it == s_smallPanelSnapshots.end() || rect->width <= 350) {
        SmallPanelSnapshot snap;
        snap.vtable = vtable;
        snap.root = *rect;
        snap.targetHeight = g_targetHeight;
        
        int numControls = *(int*)(panel + 0x38);
        char** controls = *(char***)(panel + 0x30);
        if (controls && is_readable(controls) && numControls > 0 && numControls <= 1024) {
            for (int i = 0; i < numControls; i++) {
                char* ctrl = controls[i];
                if (ctrl && is_readable(ctrl)) {
                    Rect cr = *(Rect*)(ctrl + 0x8);
                    snap.children.push_back({ ctrl, cr });
                }
            }
        }
        s_smallPanelSnapshots[panel] = snap;
        it = s_smallPanelSnapshots.find(panel);
    }
    
    const SmallPanelSnapshot& snap = it->second;
    
    // Check if already scaled
    int curTargetW = (int)(((long long)snap.root.width * targetW + 320) / 640);
    int curTargetH = (int)(((long long)snap.root.height * targetH + 240) / 480);
    if (rect->width == curTargetW && rect->height == curTargetH) {
        return;
    }
    
    // Scale root extent
    Rect scaledRoot;
    scaledRoot.left   = (int)(((long long)snap.root.left   * targetW + 320) / 640);
    scaledRoot.top    = (int)(((long long)snap.root.top    * targetH + 240) / 480);
    scaledRoot.width  = (int)(((long long)snap.root.width  * targetW + 320) / 640);
    scaledRoot.height = (int)(((long long)snap.root.height * targetH + 240) / 480);
    SetControlRect(panel, scaledRoot);
    
    // Scale glowing window border at panel + 0x70
    char* border = *(char**)(panel + 0x70);
    if (border && is_readable(border)) {
        Rect borderRect = { 0, 0, scaledRoot.width, scaledRoot.height };
        SetControlRect(border, borderRect);
    }
    
    // Scale all child controls from snapshot
    for (const auto& childSnap : snap.children) {
        if (!childSnap.control || !is_readable(childSnap.control)) continue;
        Rect s;
        s.left   = (int)(((long long)childSnap.rect.left   * targetW + 320) / 640);
        s.top    = (int)(((long long)childSnap.rect.top    * targetH + 240) / 480);
        s.width  = (int)(((long long)childSnap.rect.width  * targetW + 320) / 640);
        s.height = (int)(((long long)childSnap.rect.height * targetH + 240) / 480);
        
        void* ctrlVtable = *(void**)childSnap.control;
        if (ctrlVtable == (void*)0x1005b4318) { // Listbox scrollbar
            if (*(int*)(childSnap.control + 0x168) != 0) {
                float scrollScale = (g_targetHeight > 0) ? ((float)g_targetHeight / 480.0f) : 1.0f;
                if (menuScale > 0.0f) scrollScale = menuScale;
                *(int*)(childSnap.control + 0x168) = (int)(16 * scrollScale + 0.5f);
            }
        }
        SetControlRect(childSnap.control, s);
    }
    
    // Retain 0x60 centering flag so the 4:3 displacement is added to its screen draw position
    panel[0x5c] = (panel[0x5c] & ~0x09) | 0x60;
}

struct MenuChildSnapshot {
    char* control;
    Rect vanillaRect;
    int vanillaScrollW;
};

struct MenuPanelSnapshot {
    void* vtable;
    int baseW;
    int baseH;
    int lastKnobVersion;
    Rect vanillaRoot;
    std::vector<MenuChildSnapshot> children;
};
static std::unordered_map<void*, MenuPanelSnapshot> s_menuPanelSnapshots;

/*
  positionBarkBubble:
  Aligns the NPC in-game bark conversation dialog box (CSWGuiBarkBubble at 0x1005ad420)
  cleanly underneath the minimap radar without overlapping the radar frame.
  
  Mechanics:
  1. In vanilla KotOR (640x480), barkbubble.gui authors a base top offset of 140px,
     seating the dialogue banner directly below the 144px minimap border.
  2. In high-resolution widescreen displays, the minimap scales dynamically via hudScale.
     We query the actual bottom boundary of LBL_MAPBORDER (Control 15 in the HUD)
     or compute minimapBorder = (int)(144 * hudScale + 0.5f).
  3. We position the banner at:
       desiredTop = minimapBorder + margin (where margin = (int)(8 * hudScale + 0.5f))
  4. Updating *(int*)(window + 0x21c) = desiredTop ensures that CSWGuiBarkBubble::Draw
     (0x1002ddfa2) natively loads desiredTop into its local frame layout on every render tick.
  5. If the active rect does not match desiredTop, we invoke CSWGuiBarkBubble::SetExtent
     (vtable[2] at 0x1002de2b8) to immediately synchronize the window and child label bounds.
*/
void positionBarkBubble(char* window) {
    if (!window || !is_readable(window)) return;
    
    float hudScale = ReadIniHudScale();
    int minimapBorder = (int)(144 * hudScale + 0.5f);
    
    // Dynamically query actual minimap border height from active HUD if available
    if (g_lastScaledHud && is_readable(g_lastScaledHud)) {
        int numControls = *(int*)(g_lastScaledHud + 0x180);
        char** controls = *(char***)(g_lastScaledHud + 0x188);
        if (controls && is_readable(controls) && 15 < numControls && controls[15] && is_readable(controls[15])) {
            Rect* mapRect = (Rect*)(controls[15] + 0x8);
            if (mapRect && mapRect->height > 0 && mapRect->height < 2000) {
                minimapBorder = mapRect->top + mapRect->height;
            }
        }
    }
    
    int margin = (int)(8 * hudScale + 0.5f);
    int desiredTop = minimapBorder + margin;
    
    // Update native baseline top offset so CSWGuiBarkBubble::Draw uses it
    *(int*)(window + 0x21c) = desiredTop;
    
    // Synchronize current window extent and child label (LBL_BARKTEXT)
    Rect* r = (Rect*)(window + 0x8);
    if (r && r->top != desiredTop) {
        Rect newRect = *r;
        newRect.top = desiredTop;
        typedef void (*SetExtentFn)(void*, const Rect*);
        void** vtbl = *(void***)window;
        if (vtbl && is_readable(vtbl) && is_readable(vtbl + 2)) {
            SetExtentFn setExtent = (SetExtentFn)vtbl[2];
            if (setExtent) {
                setExtent(window, &newRect);
            }
        }
    }
}

void scaleMenuPanelTree(char* panel) {
    if (!panel || !is_readable(panel)) return;
    if (panel == g_lastScaledHud) return; // HUD interface has its own custom anchoring
    
    refreshPatchedListConstants();
    
    void* vtable = *(void**)panel;
    if (vtable == (void*)0x1005d3210) return; // Tooltip panel (hidden coordinate canvas)
    if (vtable == (void*)0x1005ad420) {
        positionBarkBubble(panel);
        return;
    }
    
    // Clear cache if screen resolution changed
    if (s_lastScaleTargetHeight != g_targetHeight) {
        s_scaledPanels.clear();
        s_menuPanelSnapshots.clear();
        s_popupSnapshots.clear();
        s_smallPanelSnapshots.clear();
        s_lastScaleTargetHeight = g_targetHeight;
    }
    
    // Handle Small Chargen Panels (qorcpnl, custpnl, quickpnl, leveluppnl)
    if (isSmallChargenPanel(vtable)) {
        scaleSmallChargenPanel(panel);
        return;
    }
    
    // Handle Popups (Containers, message boxes, pause dialogs, etc.)
    if (isPopupPanel(vtable)) {
        scalePopupPanel(panel);
        return;
    }
    
    // CSWGuiMainInterface (HUD): managed exclusively by Hook_MainInterfaceDraw
    if (vtable == (void*)0x1005a6220) {
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
    
    // Match 4:3 panel roots (640x480, 800x600, etc.), top tab bar, or known menu vtables
    bool is43 = (origW >= 640 && origH >= 480 && origW * 3 == origH * 4);
    bool isTopTab = (rect->left == 0 && rect->top == 0 && origW >= 640 && 
                     std::abs((long long)origH * 640 - (long long)origW * 86) <= 640);
    bool isKnownMenu = isMenuPanel(vtable);
    
    if (!is43 && !isTopTab && !isKnownMenu) {
        return;
    }
    
    int baseW = origW;
    int baseH = origH;
    if (isTopTab) {
        baseH = (origW * 3) / 4;
    }
    
    // Compute target dimensions: uniform 4:3 scaling to fill vertical screen
    int targetH = g_targetHeight;
    int targetW = (g_targetHeight * 4) / 3;
    
    float menuScale = ReadIniMenuScale();
    if (menuScale > 0.0f) {
        targetH = (int)(baseH * menuScale);
        targetW = (int)(baseW * menuScale);
    }
    
    auto snapIt = s_menuPanelSnapshots.find(panel);
    if (snapIt == s_menuPanelSnapshots.end() || rect->width <= 350 || (rect->width == baseW && rect->height == baseH && rect->width != targetW)) {
        MenuPanelSnapshot snap;
        snap.vtable = vtable;
        snap.baseW = baseW;
        snap.baseH = baseH;
        snap.lastKnobVersion = s_knobVersion;
        snap.vanillaRoot = *rect;
        
        int numControls = *(int*)(panel + 0x38);
        char** controls = *(char***)(panel + 0x30);
        if (controls && is_readable(controls) && numControls > 0 && numControls <= 1024) {
            for (int i = 0; i < numControls; i++) {
                char* ctrl = controls[i];
                if (!ctrl || !is_readable(ctrl)) continue;
                Rect cr = *(Rect*)(ctrl + 0x8);
                int scrollW = 0;
                void* ctrlVtable = *(void**)ctrl;
                if (ctrlVtable == (void*)0x1005b4318) {
                    scrollW = *(int*)(ctrl + 0x168);
                }
                snap.children.push_back({ ctrl, cr, scrollW });
            }
        }
        s_menuPanelSnapshots[panel] = snap;
        snapIt = s_menuPanelSnapshots.find(panel);
    }
    
    if (s_scaledPanels.find(panel) != s_scaledPanels.end() && snapIt->second.lastKnobVersion == s_knobVersion) {
        if (rect->width == targetW && rect->height == targetH) {
            return;
        }
        s_scaledPanels.erase(panel);
    }
    snapIt->second.lastKnobVersion = s_knobVersion;
    
    float scale = (snapIt->second.baseH > 0) ? ((float)targetH / (float)snapIt->second.baseH) : 1.0f;
    
    // Dynamically patch centering displacements in executable memory so
    // CSWGuiWindow::Draw and CSWGuiPanel::HandleMouseInput / GetLocalMousePos
    // use exact targetWidth and targetHeight
    patchMenuCenteringConstants(targetW, targetH);
    
    // 1. Scale panel root extent
    Rect scaledRoot = { 0, 0, targetW, targetH };
    SetControlRect(panel, scaledRoot);
    
    // Store Screen (0x1005ad040): synchronize prototype item heights and zero padding
    if (vtable == (void*)0x1005ad040) {
        ScaledItemGeometry geom = GetScaledItemGeometry(targetH);
        *(int*)(panel + 0x1d60) = geom.containerItemHeight;
        *(int*)(panel + 0x2100) = geom.containerItemHeight;
        *(uint8_t*)(panel + 0x1d93) = 0;
        *(uint8_t*)(panel + 0x2133) = 0;
    }
    
    // 2. Scale panel border (offset +0x70 in 64-bit Mac)
    char* border = *(char**)(panel + 0x70);
    if (border && is_readable(border)) {
        SetControlRect(border, scaledRoot);
    }
    
    // 3. Scale all child controls from snapshot
    for (const auto& childSnap : snapIt->second.children) {
        char* ctrl = childSnap.control;
        if (!ctrl || !is_readable(ctrl)) continue;
        const Rect& r = childSnap.vanillaRect;
        if (r.width > 0 && r.height > 0 && r.width < 8192 && r.height < 8192) {
            Rect s;
            s.left   = (int)(((long long)r.left   * targetW + snapIt->second.baseW / 2) / snapIt->second.baseW);
            s.top    = (int)(((long long)r.top    * targetH + snapIt->second.baseH / 2) / snapIt->second.baseH);
            s.width  = (int)(((long long)r.width  * targetW + snapIt->second.baseW / 2) / snapIt->second.baseW);
            s.height = (int)(((long long)r.height * targetH + snapIt->second.baseH / 2) / snapIt->second.baseH);
            
            // Save & Load Game Screen (0x1005ae300): calibrate LB_GAMES (width 272, height 323)
            // so save selection boxes match the 6 background slots pixel-for-pixel.
            // Do NOT match LBL_PLANETNAME or LBL_AREANAME (which also have width 272, but height 20)
            // so the save file name and area name stay inside the curved banner boxes above the picture!
            if (vtable == (void*)0x1005ae300 && r.width == 272 && r.height == 323) {
                s.left -= (int)(0.5f * scale + 0.5f);   // 1px left at 982p (aligns left edge flush with slot at X=22)
                s.width += (int)(1.0f * scale + 0.5f);  // widens by 2px at 982p (aligns right edge flush with slot at X=794)
                s.top -= (int)(2.8f * scale + 0.5f);    // 6px up at 982p (seats top save entry flush with top background slot at Y=18 with 19px padding)
                s.height = (int)(327.5f * scale + 0.5f); // 670px client height at 982p (produces 108.5px stride and 140px box height)
            }
            // In-Game Inventory (0x1005a75c0) and Equipment (0x1005ab508): calibrate LB_ITEMS
            ScaledItemGeometry geom = GetScaledItemGeometry(targetH);
            if ((vtable == (void*)0x1005a75c0 || vtable == (void*)0x1005ab508) &&
                (r.height == 294 || r.width == 269 || r.width == 270)) {
                s.left += geom.listLeftOffset;
                s.width += geom.listWidthOffset;
                s.top += geom.listTopOffset;
                s.height = geom.listHeight;
            }
            void* ctrlVtable = *(void**)ctrl;
            if (ctrlVtable == (void*)0x1005b4318) {
                if (childSnap.vanillaScrollW != 0) {
                    float scrollScale = (g_targetHeight > 0) ? ((float)g_targetHeight / 480.0f) : 1.0f;
                    if (menuScale > 0.0f) scrollScale = menuScale;
                    *(int*)(ctrl + 0x168) = (int)(16 * scrollScale + 0.5f);
                }
                if (vtable == (void*)0x1005ae300) {
                    // CSWGuiSaveLoad: scale padding between save entries so gap matches background slots
                    *(uint8_t*)(ctrl + 0x373) = (uint8_t)(9.3f * scale + 0.5f); // 19px padding at 982p (89px height + 19px padding = 108.5px stride)
                }
                if ((vtable == (void*)0x1005a75c0 || vtable == (void*)0x1005ab508) &&
                    (r.height == 294 || r.width == 269 || r.width == 270)) {
                    // Inventory and Equipment: scale cell spacing and base item row height
                    *(uint8_t*)(ctrl + 0x373) = (uint8_t)geom.itemPadding;
                    *(uint8_t*)(ctrl + 0x370) |= 0x8; // Set m_hasCustomPadding flag so KotOR uses itemPadding!
                    *(int*)(ctrl + 0x368) = geom.itemHeight;
                }
                if (vtable == (void*)0x1005ad040 && (r.width == 263 && r.height == 307)) {
                    // Store Screen: match listbox row height and padding to container item height
                    *(uint8_t*)(ctrl + 0x373) = 0;
                    *(uint8_t*)(ctrl + 0x370) |= 0x8;
                    *(int*)(ctrl + 0x368) = geom.containerItemHeight;
                }
                if (vtable == (void*)0x1005a5090 && r.width == 270) {
                    // CSWGuiUpgradeItemSelect (upgradeitems.gui): enforce workbench row height and 1px padding
                    *(uint8_t*)(ctrl + 0x373) = 1;
                    *(uint8_t*)(ctrl + 0x370) |= 0x8;
                    *(int*)(ctrl + 0x368) = s_currentKnobs.workbenchItemHeight;
                }
                bool isTextList = (vtable == (void*)0x1005ae790 || // CSWGuiInGameMessages
                                   vtable == (void*)0x1005a6db0 || // CSWGuiDialogComputer
                                   vtable == (void*)0x1005a6ed8 || // CSWGuiDialogComputerCamera
                                   vtable == (void*)0x1005a6a70 || // CSWGuiDialog
                                   vtable == (void*)0x1005a6c88 || // CSWGuiDialogCinematic
                                   vtable == (void*)0x1005a6b98);  // CSWGuiDialogLetterbox
                if (isTextList) {
                    *(uint8_t*)(ctrl + 0x370) &= ~0x8;
                    *(int*)(ctrl + 0x368) = 0;
                }
            }
            SetControlRect(ctrl, s);
            bool isTextList = (vtable == (void*)0x1005ae790 ||
                               vtable == (void*)0x1005a6db0 ||
                               vtable == (void*)0x1005a6ed8 ||
                               vtable == (void*)0x1005a6a70 ||
                               vtable == (void*)0x1005a6c88 ||
                               vtable == (void*)0x1005a6b98);
            if (isTextList && ctrlVtable == (void*)0x1005b4318) {
                // SetExtent sets bit 0x8 in 0x370; clear it so list items maintain natural line height
                *(uint8_t*)(ctrl + 0x370) &= ~0x8;
                *(int*)(ctrl + 0x368) = 0;
            }

            // Pazaak Game Screen (0x1005a5b80): fix BioWare authored typo in pazaakgame.gui
            // where Control 27 (Card 2 / BTN_PLRSIDE1 at {129, 340, 64, 64}) has BORDER FILLSTYLE = 1
            // (DrawCentered) instead of 2 (DrawStretched).
            // In CSWGuiButton, m_border is at +0xa8 and m_hilightBorder is at +0x130, with fillStyle at border + 0x34.
            // Enforce fillStyle = 2 (DrawStretched) on Control 27 only, so Card 2 fills its scaled slot identically to Cards 1, 3, 4!
            if (vtable == (void*)0x1005a5b80 && r.left == 129 && r.top == 340 && r.width == 64 && r.height == 64) {
                *(uint8_t*)(ctrl + 0xa8 + 0x34) = (*(uint8_t*)(ctrl + 0xa8 + 0x34) & ~0x03) | 0x02;
                *(uint8_t*)(ctrl + 0x130 + 0x34) = (*(uint8_t*)(ctrl + 0x130 + 0x34) & ~0x03) | 0x02;
            }

            if (ctrlVtable == (void*)0x1005b4318 && 
                (vtable == (void*)0x1005a75c0 || vtable == (void*)0x1005ab508) && 
                (r.height == 294 || r.width == 269 || r.width == 270)) {
                int childCount = *(int*)(ctrl + 0x350);
                char** items = *(char***)(ctrl + 0x348);
                if (items && is_readable(items) && childCount > 0 && childCount <= 1024) {
                    for (int j = 0; j < childCount; j++) {
                        char* item = items[j];
                        if (item && is_readable(item)) {
                            *(int*)(item + 0x14) = geom.itemHeight;
                            int rowX = *(int*)(item + 0x8);
                            int rowY = *(int*)(item + 0xc);
                            int rowW = *(int*)(item + 0x10);

                            // Text button (0xa8):
                            *(int*)(item + 0xb0) = rowX + geom.textOffset;
                            *(int*)(item + 0xb4) = rowY;
                            *(int*)(item + 0xb8) = rowW - geom.textDeduct;
                            *(int*)(item + 0xbc) = geom.itemHeight;

                            // Text highlight (0x130):
                            *(int*)(item + 0x138) = rowX + geom.textOffset;
                            *(int*)(item + 0x13c) = rowY;
                            *(int*)(item + 0x140) = rowW - geom.textDeduct;
                            *(int*)(item + 0x144) = geom.itemHeight;

                            // Text label (0x1b8):
                            *(int*)(item + 0x1c0) = rowX + geom.textOffset;
                            *(int*)(item + 0x1c4) = rowY;
                            *(int*)(item + 0x1c8) = rowW - geom.textDeduct;
                            *(int*)(item + 0x1cc) = geom.itemHeight;

                            // Icon texture (0x248), arch border (0x2d0), icon highlight (0x358):
                            int iconY = rowY + geom.iconTopOffset;
                            int halfIconW = (geom.iconWidth + 1) / 2;
                            *(int*)(item + 0x250) = rowX;
                            *(int*)(item + 0x254) = iconY;
                            *(int*)(item + 0x258) = geom.iconWidth;
                            *(int*)(item + 0x25c) = geom.iconHeight;
                            *(int*)(item + 0x260) = halfIconW;

                            *(int*)(item + 0x2d8) = rowX;
                            *(int*)(item + 0x2dc) = iconY;
                            *(int*)(item + 0x2e0) = geom.iconWidth;
                            *(int*)(item + 0x2e4) = geom.iconHeight;
                            *(int*)(item + 0x2e8) = halfIconW;

                            *(int*)(item + 0x360) = rowX;
                            *(int*)(item + 0x364) = iconY;
                            *(int*)(item + 0x368) = geom.iconWidth;
                            *(int*)(item + 0x36c) = geom.iconHeight;
                            *(int*)(item + 0x370) = halfIconW;

                            // Quantity badge label (0x3e0):
                            int qTop = geom.itemHeight - (int)(18 * scale + 0.5f) + geom.badgeTopOffset;
                            *(int*)(item + 0x3e8) = rowX + geom.badgeOffset;
                            *(int*)(item + 0x3ec) = rowY + qTop;
                        }
                    }
                }
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
    
    // CSWGuiClassSelection (0x1005af890): enforce initial scaled slot geometry on the 6 slots
    if (vtable == (void*)0x1005af890) {
        for (int i = 0; i < 6; i++) {
            size_t slotOffset = (size_t)i * 0x320;
            Rect slotRect = getScaledClassSlotRect(i, targetW, targetH);
            char* btn = panel + 0x90 + slotOffset;
            char* modelCtrl = panel + 0x2d0 + slotOffset;
            if (is_readable(btn)) {
                ((void(*)(char*, Rect*))0x1004a5adc)(btn, &slotRect);
            }
            if (is_readable(modelCtrl)) {
                ((void(*)(char*, Rect*))0x1004aaca6)(modelCtrl, &slotRect);
            }
        }
    }
    
    // 4. Centering flag:
    // All menu windows (CSWGuiInGameMenu container, Equip, Inventory, Character,
    // Abilities, Journal, Map, Messages, Options, CSWGuiLoadScreen, MainMenu, SaveLoad, etc.)
    // receive flags 0x20 | 0x40 (0x60).
    // The engine's centering logic (CSWGuiWindow::Draw / 0x10049dd86) centers each window
    // at (screenWidth - targetWidth) / 2 and (screenHeight - targetHeight) / 2.
    // This guarantees that all menu screens, the loading screen, the menu backdrop curtain,
    // and the 8 category tab buttons all share the exact same horizontal center!
    if (isTopLevelMenu(vtable)) {
        panel[0x5c] = (panel[0x5c] & ~0x09) | 0x60;
    } else {
        panel[0x5c] = (panel[0x5c] & ~0x68) | 0x01;
    }
    
    s_scaledPanels.insert(panel);
    
    // CSWGuiInGameMessages (0x1005ae790): Refresh messages into the scaled listbox geometry.
    // On initial tab activation, KotOR populates the messages at 640x480 width before the panel is scaled.
    // When the panel is subsequently scaled, CSWGuiListBox::SetExtent overrides m_itemHeight with the
    // maximum item height across all messages, causing single-line messages to have huge gaps.
    // Invoking CSWGuiInGameMessages::Show (0x100305ae2) immediately re-wraps and re-populates the messages
    // for the full widescreen listbox width, producing clean, compact line spacing and scrolling to the bottom
    // on the very first click (eliminating the need to tab away and back).
    if (vtable == (void*)0x1005ae790) {
        void** pApp = (void**)0x100677cf0;
        if (pApp && is_readable(pApp) && *pApp && is_readable(*pApp)) {
            typedef void (*PanelShowFn)(void*);
            PanelShowFn showFn = (PanelShowFn)0x100305ae2;
            showFn(panel);
        }
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
    char** panels = *(char***)(mgr + 0xd8);
    int panelCount = *(int*)(mgr + 0xe0);
    if (!panels || !is_readable(panels) || panelCount <= 0 || panelCount > 256) return;
    for (int i = 0; i < panelCount; i++) {
        char* panel = panels[i];
        if (panel && is_readable(panel)) {
            scaleMenuPanelTree(panel);
        }
    }
}

/*
  Hook_ClassSelectionUpdate:
  Detour hook for CSWGuiClassSelection::Update (0x100337886).
  Ticks the 3D character models and dynamically maintains calibrated widescreen slot rects
  for the 6 class character models and selection buttons.
*/
extern "C" void Hook_ClassSelectionUpdate(char* panel, float delta) {
    if (!panel || !is_readable(panel)) return;
    
    scaleMenuPanelTree(panel);
    
    int targetH = g_targetHeight;
    int targetW = (g_targetHeight * 4) / 3;
    float menuScale = ReadIniMenuScale();
    if (menuScale > 0.0f) {
        targetH = (int)(480 * menuScale);
        targetW = (int)(640 * menuScale);
    }
    
    for (int i = 0; i < 6; i++) {
        size_t slotOffset = (size_t)i * 0x320;
        
        // Synchronize 3D model active pose / animation with panel manager
        if (*(int*)0x1005d3b9c != 0 && (panel[0x5c] & 0x80) != 0) {
            void* r13 = *(void**)(panel + 0x370 + slotOffset);
            void* rdi = *(void**)(panel + 0x3a0 + slotOffset);
            if (rdi && is_readable(rdi)) {
                void* rax = *(void**)((char*)rdi + 0x80);
                if (rax && is_readable(rax)) {
                    void* rcx = *(void**)rax;
                    if (rcx && is_readable(rcx)) {
                        void* anim = ((void*(*)(void*, int))*(void**)((char*)rcx + 0x18))(rax, 0xff);
                        if (anim && is_readable(anim)) {
                            void* animRc = *(void**)anim;
                            if (animRc && is_readable(animRc)) {
                                void* curAnim = ((void*(*)(void*))*(void**)((char*)animRc + 0x210))(anim);
                                if (curAnim != r13) {
                                    void* mgr = *(void**)(panel + 0x88);
                                    if (mgr && is_readable(mgr)) {
                                        void* mgrVtable = *(void**)mgr;
                                        if (mgrVtable && is_readable(mgrVtable)) {
                                            ((void(*)(void*, void*))*(void**)((char*)mgrVtable + 0x130))(mgr, r13);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        
        // Tick 3D character model animation
        void* modelObj = *(void**)(panel + 0x3a0 + slotOffset);
        if (modelObj && is_readable(modelObj)) {
            ((void(*)(void*, int, float))0x1002a48f0)(modelObj, 0, delta);
        }
        
        // Enforce exact scaled slot geometry on selection button and 3D model
        Rect slotRect = getScaledClassSlotRect(i, targetW, targetH);
        char* btn = panel + 0x90 + slotOffset;
        char* modelCtrl = panel + 0x2d0 + slotOffset;
        if (is_readable(btn)) {
            ((void(*)(char*, Rect*))0x1004a5adc)(btn, &slotRect);
        }
        if (is_readable(modelCtrl)) {
            ((void(*)(char*, Rect*))0x1004aaca6)(modelCtrl, &slotRect);
        }
    }
    
    // Call base CSWGuiPanel::Update
    ((void(*)(char*, float))0x10049ded4)(panel, delta);
}

/*
  Trampoline for CSWGuiMainInterface::Draw (0x100235e44)
*/
__attribute__((naked)) void Original_MainInterfaceDraw(void* hud, float delta) {
    __asm__ volatile (
        "pushq %rbp\n"
        "movq %rsp, %rbp\n"
        "pushq %r15\n"
        "pushq %r14\n"
        "pushq %r13\n"
        "pushq %r12\n"
        "movq $0x100235e50, %rax\n"
        "jmpq *%rax\n"
    );
}

/*
  Hook_MainInterfaceDraw:
  Intercepts the master HUD rendering function (0x100235e44) called every frame during gameplay.
*/
extern "C" void Hook_MainInterfaceDraw(char* hud, float delta) {
    if (hud && is_readable(hud)) {
        char* mgr = *(char**)(hud + 0x20);
        updateEngineGlobals(mgr);
        if (mgr && is_readable(mgr)) {
            // In active gameplay, ensure menu backdrop curtains are dismissed
            *(char*)(mgr + 0xa8) = 0;
            *(char*)(mgr + 0xa9) = 0;
            refreshMenuPanelTrees(mgr);
        }

        // ALWAYS enforce Root Window bounds directly every frame
        *(int*)(hud + 0x8) = 0;
        *(int*)(hud + 0xC) = 0;
        *(int*)(hud + 0x10) = g_targetWidth;
        *(int*)(hud + 0x14) = g_targetHeight;

        // Minimap radar viewport & scissor dimensions
        float hudScale = ReadIniHudScale();
        int minimapBorder = (int)(144 * hudScale + 0.5f); // 216px at 1.50x
        int minimapRadar  = (int)(130 * hudScale + 0.5f); // 196px at 1.50x
        int minimapInset  = (minimapBorder - minimapRadar) / 2; // 10px
        int arrowSize     = (int)(27 * hudScale + 0.5f); // 40px
        int arrowOffset   = (minimapRadar - arrowSize) / 2; // 78px

        *(int*)(hud + 0x7b08) = minimapInset;
        *(int*)(hud + 0x7b0c) = minimapInset;
        *(int*)(hud + 0x7b10) = minimapRadar;
        *(int*)(hud + 0x7b14) = minimapRadar;

        // Minimap click button dimensions (BTN_MINIMAP / Control 17)
        *(int*)(hud + 0x7b28) = minimapInset;
        *(int*)(hud + 0x7b2c) = minimapInset;
        *(int*)(hud + 0x7b30) = minimapRadar;
        *(int*)(hud + 0x7b34) = minimapRadar;

        // Re-center player direction arrow inside the radar
        typedef void (*SetExtentFn)(void*, const Rect*);
        SetExtentFn setExtent = (SetExtentFn)0x1004a56f0;
        if (is_readable(hud + 0x7970)) {
            Rect arrowRect = { arrowOffset, arrowOffset, arrowSize, arrowSize };
            setExtent(hud + 0x7970, &arrowRect);
        }
        
        // One-time HUD scaling when HUD interface instance is created
        if (hud != g_lastScaledHud) {
            g_lastScaledHud = hud;
            
            // Expand Root Window bounds exactly to target screen dimensions
            Rect rootRect = { 0, 0, g_targetWidth, g_targetHeight };
            SetControlRect(hud, rootRect);
            *(int*)(hud + 0x8) = 0;
            *(int*)(hud + 0xC) = 0;
            *(int*)(hud + 0x10) = g_targetWidth;
            *(int*)(hud + 0x14) = g_targetHeight;
            
            int numControls = *(int*)(hud + 0x38);
            char** controls = *(char***)(hud + 0x30);
            
            if (controls && is_readable(controls) && numControls >= 96) {
                
                // -------------------------------------------------------------
                // A. TOP-RIGHT CATEGORY BUTTONS (Scaled 42x42 & Hugging Corner)
                // -------------------------------------------------------------
                int btnIndices[8] = { 30, 29, 27, 28, 23, 24, 25, 26 };
                int btnWidth = (g_targetHeight > 0) ? (int)(42.0f * (float)g_targetHeight / 982.0f + 0.5f) : 42;
                if (btnWidth < 28) btnWidth = 28;
                int btnHeight = btnWidth;
                int btnSpacing = btnWidth + 1; // 1px clean separation between buttons
                int topMargin = 2;
                
                // Anchored 4px from the true right edge of the screen
                int lastBtnRight = g_targetWidth - 4;
                int lastBtnLeft = lastBtnRight - btnWidth;
                int firstBtnLeft = lastBtnLeft - (7 * btnSpacing);
                
                for (int i = 0; i < 8; i++) {
                    int idx = btnIndices[i];
                    if (idx < numControls && controls[idx]) {
                        Rect r = { firstBtnLeft + (i * btnSpacing), topMargin, btnWidth, btnHeight };
                        SetControlRect(controls[idx], r);
                    }
                }
                
                // Background banner (Control 19: LBL_MENUBG)
                if (19 < numControls && controls[19]) {
                    Rect banner;
                    banner.left = firstBtnLeft + 1;
                    banner.top = topMargin + 2;
                    banner.width = (lastBtnRight - 1) - banner.left;
                    banner.height = btnHeight - 4;
                    SetControlRect(controls[19], banner);
                }
                
                // Top moulding horizontal bar (Controls 0, 5) bridges seamlessly from minimap to buttons
                int mouldIndices[2] = { 0, 5 };
                for (int m = 0; m < 2; m++) {
                    int idx = mouldIndices[m];
                    if (idx < numControls && controls[idx]) {
                        Rect r = *(Rect*)(controls[idx] + 0x8);
                        r.left = minimapBorder - 1;
                        r.width = (firstBtnLeft + 2) - r.left;
                        SetControlRect(controls[idx], r);
                    }
                }
                
                // -------------------------------------------------------------
                // B. BOTTOM-RIGHT ACTION QUEUE & BLUE MOULDING FRAME (Scaled)
                // Scaled as a single unified group from origin (1095, 872) so the
                // blue neon frame (LBL_MOULDING3), buttons, arrows, and action
                // description remain 100% pixel-perfectly aligned.
                // -------------------------------------------------------------
                int actionOrigX = 1095;
                int actionOrigY = 872;
                float actionScale = hudScale;
                int scaledActionWidth = (int)(180 * actionScale);  // 270px at 1.50x
                int scaledActionHeight = (int)(85 * actionScale);  // 127px at 1.50x
                int targetActionLeft = g_targetWidth - 6 - scaledActionWidth;
                int targetActionTop = g_targetHeight - 8 - scaledActionHeight;
                
                for (int i = 0; i < numControls; i++) {
                    if (!controls[i]) continue;
                    Rect orig = *(Rect*)(controls[i] + 0x8);
                    // Match all action bar controls in [1090, 1280] x [870, 960]
                    if (orig.left >= 1090 && orig.left <= 1280 && orig.top >= 870 && orig.top <= 960 && orig.width > 0 && orig.height > 0) {
                        Rect r;
                        r.left = targetActionLeft + (int)((orig.left - actionOrigX) * actionScale);
                        r.top = targetActionTop + (int)((orig.top - actionOrigY) * actionScale);
                        r.width = (int)(orig.width * actionScale);
                        r.height = (int)(orig.height * actionScale);
                        SetControlRect(controls[i], r);
                    }
                }
                *(int*)(hud + 0xd170) = targetActionTop;
                
                // -------------------------------------------------------------
                // C. BOTTOM-LEFT CHARACTER PORTRAIT & STATUS BARS (Scaled)
                // Total group height is 134px (top 822 to bottom 956).
                // Scaled height is 201px at 1.50x. Anchored 8px above display bottom so
                // the character face and vitality/force bars never cut off.
                // -------------------------------------------------------------
                int portraitOrigX = 4;
                int portraitOrigY = 822;
                float portraitScale = hudScale;
                int scaledPortraitHeight = (int)(134 * portraitScale); // 201px at 1.50x
                int targetPortraitY = (g_targetHeight - 8) - scaledPortraitHeight;
                
                for (int i = 0; i < numControls; i++) {
                    if (!controls[i]) continue;
                    Rect orig = *(Rect*)(controls[i] + 0x8);
                    if (orig.left <= 320 && orig.top >= 750 && orig.width > 0 && orig.height > 0) {
                        Rect r;
                        r.left = portraitOrigX + (int)((orig.left - portraitOrigX) * portraitScale);
                        r.top = targetPortraitY + (int)((orig.top - portraitOrigY) * portraitScale);
                        r.width = (int)(orig.width * portraitScale);
                        r.height = (int)(orig.height * portraitScale);
                        SetControlRect(controls[i], r);
                    }
                }
                
                // -------------------------------------------------------------
                // D. TOP-LEFT MINIMAP (Scaled Up inside Border)
                // -------------------------------------------------------------
                if (15 < numControls && controls[15]) { // LBL_MAPBORDER
                    Rect r = { 0, 0, minimapBorder, minimapBorder };
                    SetControlRect(controls[15], r);
                }
                if (16 < numControls && controls[16]) { // LBL_MAPVIEW (radar viewport)
                    Rect r = { minimapInset, minimapInset, minimapRadar, minimapRadar };
                    SetControlRect(controls[16], r);
                }
                if (17 < numControls && controls[17]) { // BTN_MINIMAP
                    Rect r = { minimapInset, minimapInset, minimapRadar, minimapRadar };
                    SetControlRect(controls[17], r);
                }
                if (80 < numControls && controls[80]) { // LBL_ARROW (player direction arrow)
                    Rect r = { minimapInset + arrowOffset, minimapInset + arrowOffset, arrowSize, arrowSize };
                    SetControlRect(controls[80], r);
                }
                
                // -------------------------------------------------------------
                // E. CENTER COMBAT ACTION QUEUE (Scaled Up & Perfectly Aligned)
                // In mipc212x9.gui:
                //   Controls 1, 2, 3: LBL_COMBATBG2, LBL_COMBATBG3, LBL_COMBATBG1 (Background frames)
                //   Control 4: BTN_CLEARALL (Disengage / Clear All button)
                //   Control 90: LBL_QUEUE0 (Active action in red reticle)
                //   Control 91: LBL_QUEUE1 (Queued action 1)
                //   Control 92: LBL_QUEUE2 (Queued action 2)
                //   Control 93: LBL_QUEUE3 (Queued action 3)
                //   Control 94: BTN_CLEARONE (Click hitbox for queued actions)
                //   Control 95: BTN_CLEARONE2 (Click hitbox for active reticle)
                // -------------------------------------------------------------
                float combatScale = ReadIniCombatScale();
                int pillWidth = (int)(131 * combatScale);
                int pillHeight = (int)(90 * combatScale);
                int pillLeft = (g_targetWidth - pillWidth) / 2;
                int targetQueueTop = (g_targetHeight - 8) - pillHeight;
                
                // Background pill frames
                Rect pillRect = { pillLeft, targetQueueTop, pillWidth, pillHeight };
                if (1 < numControls && controls[1]) SetControlRect(controls[1], pillRect); // LBL_COMBATBG2
                if (2 < numControls && controls[2]) SetControlRect(controls[2], pillRect); // LBL_COMBATBG3
                if (3 < numControls && controls[3]) {                                      // LBL_COMBATBG1
                    Rect pillBG1 = { pillLeft, targetQueueTop, pillWidth, (int)(77 * combatScale) };
                    SetControlRect(controls[3], pillBG1);
                }
                
                // Disengage / Clear All button at bottom of pill
                if (4 < numControls && controls[4]) {                                      // BTN_CLEARALL
                    int clearAllW = (int)(125 * combatScale);
                    int clearAllH = (int)(35 * combatScale);
                    Rect rClearAll = { pillLeft + (int)(3 * combatScale), targetQueueTop + (int)(56 * combatScale), clearAllW, clearAllH };
                    SetControlRect(controls[4], rClearAll);
                }
                
                // Active Action in Red Reticle (Control 90: LBL_QUEUE0)
                // Reticle center is at X = pillLeft + 102 * combatScale, Y = targetQueueTop + 28 * combatScale
                int x0 = pillLeft + (int)(102 * combatScale);
                int y0 = targetQueueTop + (int)(28 * combatScale);
                int size0 = (int)(32 * combatScale); // 38px
                Rect r0 = { x0 - size0 / 2, y0 - size0 / 2, size0, size0 };
                if (90 < numControls && controls[90]) SetControlRect(controls[90], r0);
                
                if (95 < numControls && controls[95]) { // BTN_CLEARONE2 (active action click hitbox)
                    int clear0W = (int)(51 * combatScale);
                    int clear0H = (int)(53 * combatScale);
                    Rect rClear0 = { x0 - clear0W / 2, y0 - clear0H / 2, clear0W, clear0H };
                    SetControlRect(controls[95], rClear0);
                }
                
                // Queued Actions (Controls 91, 92, 93: LBL_QUEUE1, LBL_QUEUE2, LBL_QUEUE3)
                // Evenly spaced across the left portion of the pill
                int sizeQueued = (int)(16 * combatScale * 1.25f); // 24px
                int yQueued = targetQueueTop + (int)(41.0f * combatScale);
                int x1 = pillLeft + (int)(pillWidth * 0.52f);
                int x2 = pillLeft + (int)(pillWidth * 0.35f);
                int x3 = pillLeft + (int)(pillWidth * 0.18f);
                
                Rect r1 = { x1 - sizeQueued / 2, yQueued - sizeQueued / 2, sizeQueued, sizeQueued };
                Rect r2 = { x2 - sizeQueued / 2, yQueued - sizeQueued / 2, sizeQueued, sizeQueued };
                Rect r3 = { x3 - sizeQueued / 2, yQueued - sizeQueued / 2, sizeQueued, sizeQueued };
                
                if (91 < numControls && controls[91]) SetControlRect(controls[91], r1);
                if (92 < numControls && controls[92]) SetControlRect(controls[92], r2);
                if (93 < numControls && controls[93]) SetControlRect(controls[93], r3);
                
                if (94 < numControls && controls[94]) { // BTN_CLEARONE (queued actions click hitbox)
                    Rect rClearQueued = { pillLeft + (int)(18 * combatScale), yQueued - (int)(15 * combatScale), (int)(75 * combatScale), (int)(30 * combatScale) };
                    SetControlRect(controls[94], rClearQueued);
                }
            }
        }
        
        // -----------------------------------------------------------------
        // 2. DYNAMIC TOOLTIP SCALING & ACTION BUTTON TITLE CENTERING (Runs EVERY frame)
        // Ensures tooltip text like "EQUIP : UNARMED" or "ABILITIES" is never sliced,
        // and dynamically centers the hovered action button's name (e.g. "CURE", "FLURRY")
        // directly above the active action reticle regardless of screen resolution.
        // -----------------------------------------------------------------
        char* actionDesc = hud + 0xce40;
        char* actionDescBg = hud + 0xcfd8;
        if (is_readable(actionDesc) && is_readable(actionDescBg)) {
            int hoveredIndex = *(int*)(hud + 0x2358);
            if (hoveredIndex >= 0 && hoveredIndex < 6) {
                char* btn = hud + 0x97e0 + (hoveredIndex * 0x910);
                if (is_readable(btn)) {
                    Rect btnRect = *(Rect*)(btn + 0x8);
                    Rect rd = *(Rect*)(actionDesc + 0x8);
                    if (btnRect.width > 0 && rd.width > 0 && rd.height > 0) {
                        int btnCenterX = btnRect.left + (btnRect.width / 2);
                        rd.left = btnCenterX - (rd.width / 2);
                        if (rd.left < 4) rd.left = 4;
                        if (rd.left + rd.width > g_targetWidth - 4) {
                            rd.left = (g_targetWidth - 4) - rd.width;
                        }
                        rd.top = btnRect.top - rd.height - 2;
                        *(int*)(hud + 0xd170) = btnRect.top - 2;
                        SetControlRect(actionDesc, rd);
                        SetControlRect(actionDescBg, rd);
                    }
                }
            } else {
                Rect rd = *(Rect*)(actionDesc + 0x8);
                if (rd.width > 0 && rd.height > 0) {
                    if (rd.left + rd.width > g_targetWidth - 4) {
                        rd.left = (g_targetWidth - 4) - rd.width;
                        SetControlRect(actionDesc, rd);
                        SetControlRect(actionDescBg, rd);
                    }
                }
            }
        }
    }
    
    Original_MainInterfaceDraw(hud, delta);
}


/*
  Trampoline for CSWGuiWindow::Draw (0x10049ded4)
  Base render method for ALL GUI windows, including menus, dialogs, and containers.
*/
__attribute__((naked)) void Original_WindowDraw(void* window, float delta) {
    __asm__ volatile (
        "pushq %rbp\n"
        "movq %rsp, %rbp\n"
        "pushq %r15\n"
        "pushq %r14\n"
        "pushq %r12\n"
        "pushq %rbx\n"
        "subq $0x20, %rsp\n"
        "movq $0x10049dee3, %rax\n"
        "jmpq *%rax\n"
    );
}

static void enforceItemGeometry(char* panel) {
    if (!panel || !is_readable(panel)) return;
    void* vtable = *(void**)panel;
    bool isInventoryOrEquip = (vtable == (void*)0x1005a75c0 || vtable == (void*)0x1005ab508);
    bool isStore = (vtable == (void*)0x1005ad040);
    bool isContainer = (vtable == (void*)0x1005ab758);
    if (!isInventoryOrEquip && !isStore && !isContainer) return;
    
    int numControls = *(int*)(panel + 0x38);
    char** controls = *(char***)(panel + 0x30);
    if (!controls || !is_readable(controls) || numControls <= 0 || numControls > 1024) return;
    
    ScaledItemGeometry geom = GetScaledItemGeometry(g_targetHeight);
    float scale = (g_targetHeight > 0) ? ((float)g_targetHeight / 480.0f) : 1.0f;
    int qHeight = (int)(18 * scale + 0.5f);
    int qTop = geom.itemHeight - qHeight + geom.badgeTopOffset;
    
    for (int i = 0; i < numControls; i++) {
        char* ctrl = controls[i];
        if (!ctrl || !is_readable(ctrl)) continue;
        void* ctrlVtable = *(void**)ctrl;
        if (ctrlVtable != (void*)0x1005b4318) continue;
        
        if (isInventoryOrEquip) {
            Rect* cr = (Rect*)(ctrl + 0x8);
            if (cr->height != 294 && cr->width != 269 && cr->width != 270) {
                auto snapIt = s_menuPanelSnapshots.find(panel);
                if (snapIt != s_menuPanelSnapshots.end() && i < (int)snapIt->second.children.size()) {
                    const Rect& vr = snapIt->second.children[i].vanillaRect;
                    if (vr.height != 294 && vr.width != 269 && vr.width != 270) continue;
                } else {
                    continue;
                }
            }
        }
        if (isStore) {
            auto snapIt = s_menuPanelSnapshots.find(panel);
            if (snapIt != s_menuPanelSnapshots.end() && i < (int)snapIt->second.children.size()) {
                const Rect& vr = snapIt->second.children[i].vanillaRect;
                if (vr.width != 263 || vr.height != 307) continue;
            }
        }
        
        int childCount = *(int*)(ctrl + 0x350);
        char** items = *(char***)(ctrl + 0x348);
        if (items && is_readable(items) && childCount > 0 && childCount <= 1024) {
            for (int j = 0; j < childCount; j++) {
                char* item = items[j];
                if (item && is_readable(item)) {
                    int rowX = *(int*)(item + 0x8);
                    int rowY = *(int*)(item + 0xc);
                    int rowW = *(int*)(item + 0x10);

                    // Ensure item border frames use STRETCH fill mode (fillStyle = 2)
                    // so the arch texture stretches as a single continuous border instead of tiling into 4 mini boxes!
                    *(uint8_t*)(item + 0x27c) = 2; // border 1 (unselected arch)
                    *(uint8_t*)(item + 0x304) = 2; // border 2 (selected arch)
                    *(uint8_t*)(item + 0x38c) = 2; // border 3 (highlight arch)

                    if (isStore || isContainer) {
                        int storeH = geom.containerItemHeight;
                        int halfStoreH = (storeH + 1) / 2;
                        int storeQTop = storeH - qHeight;
                        if (*(int*)(item + 0x14) != storeH || *(int*)(item + 0xbc) != storeH) {
                            *(int*)(item + 0x14) = storeH;
                            *(int*)(item + 0xb0) = rowX + storeH;
                            *(int*)(item + 0xb4) = rowY;
                            *(int*)(item + 0xb8) = rowW - storeH;
                            *(int*)(item + 0xbc) = storeH;

                            *(int*)(item + 0x138) = rowX + storeH;
                            *(int*)(item + 0x13c) = rowY;
                            *(int*)(item + 0x140) = rowW - storeH;
                            *(int*)(item + 0x144) = storeH;

                            *(int*)(item + 0x1c0) = rowX + storeH;
                            *(int*)(item + 0x1c4) = rowY;
                            *(int*)(item + 0x1c8) = rowW - storeH;
                            *(int*)(item + 0x1cc) = storeH;

                            *(int*)(item + 0x250) = rowX;
                            *(int*)(item + 0x254) = rowY;
                            *(int*)(item + 0x258) = storeH;
                            *(int*)(item + 0x25c) = storeH;
                            *(int*)(item + 0x260) = halfStoreH;

                            *(int*)(item + 0x2d8) = rowX;
                            *(int*)(item + 0x2dc) = rowY;
                            *(int*)(item + 0x2e0) = storeH;
                            *(int*)(item + 0x2e4) = storeH;
                            *(int*)(item + 0x2e8) = halfStoreH;

                            *(int*)(item + 0x360) = rowX;
                            *(int*)(item + 0x364) = rowY;
                            *(int*)(item + 0x368) = storeH;
                            *(int*)(item + 0x36c) = storeH;
                            *(int*)(item + 0x370) = halfStoreH;

                            *(int*)(item + 0x3e8) = rowX + storeH - (int)(20 * scale + 0.5f);
                            *(int*)(item + 0x3ec) = rowY + storeQTop;
                        }
                    } else {
                        int targetBadgeX = rowX + geom.badgeOffset;
                        int targetBadgeY = rowY + qTop;
                        int halfIconW = (geom.iconWidth + 1) / 2;

                        if (*(int*)(item + 0xbc) != geom.itemHeight || *(int*)(item + 0x14) != geom.itemHeight ||
                            *(int*)(item + 0x3e8) != targetBadgeX || *(int*)(item + 0x3ec) != targetBadgeY) {
                            *(int*)(item + 0x14) = geom.itemHeight;

                            *(int*)(item + 0xb0) = rowX + geom.textOffset;
                            *(int*)(item + 0xb4) = rowY;
                            *(int*)(item + 0xb8) = rowW - geom.textDeduct;
                            *(int*)(item + 0xbc) = geom.itemHeight;

                            *(int*)(item + 0x138) = rowX + geom.textOffset;
                            *(int*)(item + 0x13c) = rowY;
                            *(int*)(item + 0x140) = rowW - geom.textDeduct;
                            *(int*)(item + 0x144) = geom.itemHeight;

                            *(int*)(item + 0x1c0) = rowX + geom.textOffset;
                            *(int*)(item + 0x1c4) = rowY;
                            *(int*)(item + 0x1c8) = rowW - geom.textDeduct;
                            *(int*)(item + 0x1cc) = geom.itemHeight;

                            int iconY = rowY + geom.iconTopOffset;
                            *(int*)(item + 0x250) = rowX;
                            *(int*)(item + 0x254) = iconY;
                            *(int*)(item + 0x258) = geom.iconWidth;
                            *(int*)(item + 0x25c) = geom.iconHeight;
                            *(int*)(item + 0x260) = halfIconW;

                            *(int*)(item + 0x2d8) = rowX;
                            *(int*)(item + 0x2dc) = iconY;
                            *(int*)(item + 0x2e0) = geom.iconWidth;
                            *(int*)(item + 0x2e4) = geom.iconHeight;
                            *(int*)(item + 0x2e8) = halfIconW;

                            *(int*)(item + 0x360) = rowX;
                            *(int*)(item + 0x364) = iconY;
                            *(int*)(item + 0x368) = geom.iconWidth;
                            *(int*)(item + 0x36c) = geom.iconHeight;
                            *(int*)(item + 0x370) = halfIconW;

                            *(int*)(item + 0x3e8) = targetBadgeX;
                            *(int*)(item + 0x3ec) = targetBadgeY;
                        }
                    }
                }
            }
        }
    }
}

/*
  Hook_WindowDraw:
  Called every frame for every GUI window. Ensures engine globals and in-game menu centering
  remain active even when the HUD is closed or menus are active.
*/
extern "C" void Hook_WindowDraw(char* window, float delta) {
    if (window && is_readable(window)) {
        char* mgr = *(char**)(window + 0x20);
        updateEngineGlobals(mgr);
        
        void* vtable = *(void**)window;
        if (vtable == (void*)0x1005ad420) {
            positionBarkBubble(window);
        }
        else if (vtable == (void*)0x1005d3210) {
            // CSWGuiToolTipPanel: Expand extent and label wrap width so onmouseover
            // descriptions (e.g. "INVENTORY : I") are never scissored at 1280px!
            *(int*)(window + 0x8) = 0;
            *(int*)(window + 0xC) = 0;
            *(int*)(window + 0x10) = g_targetWidth;
            *(int*)(window + 0x14) = g_targetHeight;
            if (is_readable(window + 0x100)) {
                *(int*)(window + 0x100) = 500;
            }
        }
        else {
            scaleMenuPanelTree(window);
            enforceItemGeometry(window);
            if (vtable == (void*)0x1005a6db0 || vtable == (void*)0x1005a6ed8 || vtable == (void*)0x1005a6a70 ||
                vtable == (void*)0x1005a6c88 || vtable == (void*)0x1005a6b98) {
                char* lbReplies = window + 0x20c0;
                if (is_readable(lbReplies)) {
                    *(uint8_t*)(lbReplies + 0x370) &= ~0x8;
                    *(int*)(lbReplies + 0x368) = 0;
                    
                    int childCount = *(int*)(lbReplies + 0x350);
                    char** items = *(char***)(lbReplies + 0x348);
                    Rect lbRect = *(Rect*)(lbReplies + 0x8);
                    int scrollW = *(int*)(lbReplies + 0x168);
                    int itemW = lbRect.width - scrollW - 8;
                    if (items && is_readable(items) && childCount > 0 && childCount <= 64 && itemW > 50) {
                        for (int j = 0; j < childCount; j++) {
                            char* it = items[j];
                            if (it && is_readable(it)) {
                                *(int*)(it + 0x10) = itemW;
                            }
                        }
                    }
                }
                if (vtable == (void*)0x1005a6db0 || vtable == (void*)0x1005a6ed8) {
                    char* lbMsg = window + 0x3940;
                    if (is_readable(lbMsg)) {
                        *(uint8_t*)(lbMsg + 0x370) &= ~0x8;
                        *(int*)(lbMsg + 0x368) = 0;
                    }
                }
            }
            if (mgr) {
                refreshMenuPanelTrees(mgr);
            }
        }
    }
    Original_WindowDraw(window, delta);
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
  The engine hardcodes these 14 relative control bounds for 800x600 resolution. Every time
  a target is acquired or updated, the engine invokes CSWGuiControl::SetBounds (0x1004a17c2)
  and resets these controls back to their small 800x600 dimensions. On high-resolution displays
  (e.g., 1080p, 1440p, 4K, or Apple Retina), the health bar and action icons become tiny and hard to read.

  WindowsScaledKotor vs. mac_Widescreen_Master Translation Analysis:
  - The Windows "Scaled Floating Target + Actions" patch uses uniform UI scaling based on
    vertical resolution:
        scale = targetHeight / 600.0f
    This keeps the 32x32 action icons perfectly 1:1 square, scales spacing uniformly, and
    prevents horizontal stretching.
  - The earlier mac_Widescreen_Master draft used anamorphic scaling:
        x_scale = targetWidth / 800.0f
        y_scale = targetHeight / 600.0f
    On widescreen displays (16:10, 16:9, 21:9), targetWidth/800 is much larger than
    targetHeight/600 (e.g., 1.89x vs 1.63x at 1512x982; 2.40x vs 1.80x at 1080p;
    4.30x vs 2.40x at 21:9), which severely stretches square icons into wide rectangles!
  
  Our Solution:
  We adopt the WindowsScaledKotor uniform scaling model:
    scale = (float)g_targetHeight / 600.0f;
  Every matched floating target control is scaled uniformly with nearest-integer rounding (+0.5f),
  ensuring 1:1 square action icons, crisp readable health bars, and perfect alignment on any display.
*/

static const Rect s_floatingTargetRects[] = {
    // Action Slot 1:
    { 43, 35, 35, 59 },   // BTN_TARGET0 (Slot 1 container button)
    { 45, 49, 32, 32 },   // LBL_TARGET0 (Slot 1 action icon)
    { 43, 36, 35, 12 },   // BTN_TARGETUP0 (Slot 1 cycle up arrow)
    { 44, 80, 35, 12 },   // BTN_TARGETDOWN0 (Slot 1 cycle down arrow)

    // Action Slot 2:
    { 83, 35, 35, 59 },   // BTN_TARGET1 (Slot 2 container button)
    { 85, 49, 32, 32 },   // LBL_TARGET1 (Slot 2 action icon)
    { 83, 36, 35, 12 },   // BTN_TARGETUP1 (Slot 2 cycle up arrow)
    { 84, 80, 35, 12 },   // BTN_TARGETDOWN1 (Slot 2 cycle down arrow)

    // Action Slot 3:
    { 122, 35, 35, 59 },  // BTN_TARGET2 (Slot 3 container button)
    { 124, 49, 32, 32 },  // LBL_TARGET2 (Slot 3 action icon)
    { 122, 36, 35, 12 },  // BTN_TARGETUP2 (Slot 3 cycle up arrow)
    { 123, 80, 35, 12 },  // BTN_TARGETDOWN2 (Slot 3 cycle down arrow)

    // Target Nameplate & Health Bar:
    { 0, 0, 200, 26 },    // LBL_NAME / LBL_NAMEBG (Target nameplate)
    { 0, 27, 200, 6 }     // LBL_HEALTHBG / PB_HEALTH (Target health bar)
};

/*
  Note on 0x1004a17c2:
  In earlier drafts, 0x1004a17c2 was misidentified as CSWGuiControl::SetBounds.
  As confirmed by the binary symbol table in addresses.db, 0x1004a17c2 is actually:
      CSWGuiBorderParams::SetFillImage(CResRef const&, int)
  The detour has been removed from hooks.toml. UniversalSetBounds is retained as an
  exported stub for backward compatibility.
*/
extern "C" void UniversalSetBounds(void* control, const Rect* rect) {
    (void)control;
    (void)rect;
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
  2. If FontScale is omitted or 0 (auto-scale mode), we follow the Windows ScaledKotor formula:
         scale = (float)g_targetHeight / 1080.0f;
     clamped to a minimum of 1.0f on sub-1080p displays so fonts are never shrunk below 1x.
 ==============================================================================================
*/

namespace ScaledFont {

constexpr uint32_t FontInfoOffset = 0x48;       // Offset of CAurFontInfo* in CResTexture (Mac x86_64)
constexpr uint32_t FontHeightOffset = 0x04;      // Font height in pixels
constexpr uint32_t BaselineHeightOffset = 0x08;  // Baseline height in pixels
constexpr uint32_t TextureWidthOffset = 0x0C;    // Texture sheet width
constexpr uint32_t SpacingROffset = 0x10;        // Right spacing / character advance padding
constexpr uint32_t SpacingBOffset = 0x14;        // Bottom spacing / line spacing padding

constexpr uint32_t MaxCachedFonts = 64;
constexpr float FontMetricPixelsPerUnit = 100.0f;
constexpr float FontMetricFloorEpsilon = 0.00001f;

constexpr uint32_t FontMetricOffsets[] = {
    FontHeightOffset, BaselineHeightOffset, TextureWidthOffset,
    SpacingROffset, SpacingBOffset
};

static void* s_scaledFonts[MaxCachedFonts] = {};
static uint32_t s_scaledFontCount = 0;
static float s_cachedIniFontScale = -1.0f;

// Read optional "FontScale" from ~/Library/Application Support/Knights of the Old Republic/swkotor.ini
static float ReadIniFontScale() {
    if (s_cachedIniFontScale >= 0.0f) {
        return s_cachedIniFontScale;
    }
    s_cachedIniFontScale = 0.0f;
    const char* home = getenv("HOME");
    if (!home) return 0.0f;
    
    char path[1024];
    snprintf(path, sizeof(path), "%s/Library/Application Support/Knights of the Old Republic/swkotor.ini", home);
    FILE* f = fopen(path, "r");
    if (!f) return 0.0f;
    
    char line[256];
    bool inGraphics = false;
    while (fgets(line, sizeof(line), f)) {
        char* p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '[') {
            inGraphics = (strncmp(p, "[Graphics Options]", 18) == 0);
            continue;
        }
        if (inGraphics && strncasecmp(p, "FontScale", 9) == 0) {
            char* eq = strchr(p, '=');
            if (eq) {
                float val = (float)atof(eq + 1);
                if (val > 0.0f && val <= 10.0f) {
                    s_cachedIniFontScale = val;
                }
            }
        }
    }
    fclose(f);
    return s_cachedIniFontScale;
}

// Compute effective font scale factor
static float GetEffectiveFontScale() {
    float iniScale = ReadIniFontScale();
    if (iniScale > 0.0f) {
        return iniScale;
    }
    
    // Windows ScaledKotor auto-scaling formula: targetHeight / 1080.0f
    // Clamped to minimum 1.0f on sub-1080p displays
    if (g_targetHeight >= 1080) {
        return (float)g_targetHeight / 1080.0f;
    }
    return 1.0f;
}

// Verify that the memory region contains plausible font metrics
static bool HasSaneFontMetrics(char* fontInfo) {
    if (!fontInfo || !is_readable(fontInfo)) return false;
    float fh = *(float*)(fontInfo + FontHeightOffset);
    float bh = *(float*)(fontInfo + BaselineHeightOffset);
    float tw = *(float*)(fontInfo + TextureWidthOffset);
    return (fh > 0.0f && fh < 512.0f &&
            bh > 0.0f && bh < 512.0f &&
            tw > 0.0f && tw < 4096.0f);
}

static bool IsFontAlreadyScaled(void* fontInfo) {
    for (uint32_t i = 0; i < s_scaledFontCount; ++i) {
        if (s_scaledFonts[i] == fontInfo) return true;
    }
    return false;
}

static void MarkFontScaled(void* fontInfo) {
    if (s_scaledFontCount < MaxCachedFonts) {
        s_scaledFonts[s_scaledFontCount++] = fontInfo;
    }
}

static float FloorFontMetricToPixel(float value) {
    return std::floor(value * FontMetricPixelsPerUnit + FontMetricFloorEpsilon) / FontMetricPixelsPerUnit;
}

static void MultiplyFontInfo(void* fontInfoPtr, float scale) {
    if (!fontInfoPtr || scale <= 0.0f || !is_readable(fontInfoPtr)) return;
    char* fontInfo = static_cast<char*>(fontInfoPtr);
    if (!HasSaneFontMetrics(fontInfo)) return;
    
    for (size_t i = 0; i < 5; ++i) {
        float* metric = reinterpret_cast<float*>(fontInfo + FontMetricOffsets[i]);
        float unrounded = (*metric) * scale;
        *metric = FloorFontMetricToPixel(unrounded);
    }
}

} // namespace ScaledFont

/*
  scaleLoadedTextureMetadata:
  Detour hook replacing 0x1001f8883 (CResTexture::ParseTXI completion).
  Intercepts newly loaded textures and scales CAurFontInfo metadata if present.
*/
extern "C" void scaleLoadedTextureMetadata(void* texture) {
    if (!texture || !is_readable(texture)) return;
    
    // Check if texture has an attached CAurFontInfo object (+0x48 on Mac 64-bit)
    void* fontInfo = *(void**)((char*)texture + ScaledFont::FontInfoOffset);
    if (!fontInfo || !is_readable(fontInfo)) return;
    
    // Ensure each font is scaled exactly once
    if (ScaledFont::IsFontAlreadyScaled(fontInfo)) return;
    ScaledFont::MarkFontScaled(fontInfo);
    
    float scale = ScaledFont::GetEffectiveFontScale();
    if (scale <= 0.0f || scale == 1.0f) return;
    
    ScaledFont::MultiplyFontInfo(fontInfo, scale);
}

/*
  DylibInit:
  Invoked automatically when the dynamic library is loaded by dyld at game launch.
  Initializes display resolution from swkotor.ini and immediately installs baseline
  menu centering displacements and scaled row geometry into game memory.
*/
__attribute__((constructor))
static void DylibInit() {
    InitTargetResolution();
    
    // Force uniform HUD GUI template (mipc212x9) across all resolutions:
    // Safely repoint the "mipc216x12" (0x100233429), "mipc212x10" (0x10023345e),
    // "mipc210x7" (0x1002334b4), and "mipc28x6" (0x1002334d8) string pointers to "mipc212x9"
    // without bypassing register initialization or control flow
    writeMemBytes(0x100233429, (const uint8_t*)"\x48\x8d\x35\x18\x78\x2f\x00", 7);
    writeMemBytes(0x10023345e, (const uint8_t*)"\x48\x8d\x35\xe3\x77\x2f\x00", 7);
    writeMemBytes(0x1002334b4, (const uint8_t*)"\x48\x8d\x35\x8d\x77\x2f\x00", 7);
    writeMemBytes(0x1002334d8, (const uint8_t*)"\x48\x8d\x35\x69\x77\x2f\x00", 7);
    
    int targetH = g_targetHeight;
    int targetW = (g_targetHeight * 4) / 3;
    float menuScale = ReadIniMenuScale();
    if (menuScale > 0.0f) {
        targetH = (int)(960 * menuScale);
        targetW = (int)(1280 * menuScale);
    }
    patchMenuCenteringConstants(targetW, targetH);
    refreshPatchedListConstants();
}
