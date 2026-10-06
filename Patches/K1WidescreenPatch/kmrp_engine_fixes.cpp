/*
 ==============================================================================================
  KMRP ENGINE ADAPTATIONS (ported from KMRP for Windows to the Aspyr macOS build)
 ==============================================================================================
  Engine adaptations supporting widescreen resolutions and aspect ratios on the Aspyr
  macOS build. (General engine bug fixes beneficial to all resolutions have been separated
  into the standalone Stray Bug Fixes Patch).

  Target: KOTOR_Exe 1.4.0 (176481), SHA-256 C1FCB8D3...6D71.
 ==============================================================================================
*/
#include <mach/mach.h>
#include <stdint.h>
#include <string.h>

/*
  Video mode follows the widescreen target
  ----------------------------------------------------------------------------------------------
  The widescreen layout is built for g_targetWidth x g_targetHeight (the display's point size,
  or ForceWidth/ForceHeight), but the engine opens its video mode from [Graphics Options]
  Width/Height. In fullscreen those drive a fixed-size CGL surface (kCGLCPSurfaceBackingSize):
  with the default 1024x768 the 1512x982 frame was drawn into a 1024x768 surface and cropped
  to its bottom-left corner (measured 2026-09-28; windowed mode hid it because the surface
  then follows the window).

  The engine's video-mode reader (0x10026e864, Aspyr's ReadAndSetVideoMode) joins all of its
  paths (INI value, "largest valid mode", 800x600 fallback) at 0x10026ed44, with r12 -> width
  and r15 -> height. The detour there hands the engine the layout's resolution instead, so the
  display mode, the surface and the layout always agree.
*/
extern int g_targetWidth;
extern int g_targetHeight;
extern int g_displayPointWidth, g_displayPointHeight;
extern int g_displayPixelWidth, g_displayPixelHeight;
void InitTargetResolution();

/*
  Retina display modes
  ----------------------------------------------------------------------------------------------
  A mode is only usable if it is in Aspyr's display-mode list: the engine looks the requested
  size up there (0x100204d5a) and falls back to the default mode when it is missing. The list
  (built once at startup by 0x10001ddee from SDL's modes, in points) was written to give every
  mode a twin scaled by the display's backing factor, flagged as a HiDPI mode, but the factor
  is the constant 1.0 (movabs at 0x10001de62), so the twins are never added. A target above the
  point size (ForceWidth/ForceHeight at the display's pixel size) was therefore not a valid mode: the engine laid out 3024x1964 but rendered into a 1024x768 surface, cropped
  (measured 2026-09-29 on a 14" MacBook Pro, ForceWidth=3024 ForceHeight=1964).

  The detour sits on the store of that constant (0x10001de6c, mov [rbp-0xa0], rax) and hands
  it the display's real pixel/point ratio in rax. Only when the target needs it, so the list
  stays vanilla otherwise. Everything downstream is Aspyr's own code: the surface is sized
  from the mode (3024x1964), and SDL's window-point mouse coordinates are converted with the
  render/window ratio the port already keeps for every mode (0x100025802; vanilla relies on
  it at 1024x768 in a 1512x982 window), so the cursor tracks without a mouse fix of its own
  (checked with a click on Load Game at 3024x1964).
*/
extern "C" uint64_t KMRP_DisplayModeScale() {
    InitTargetResolution();
    // Whatever size the game starts at, since 2026-10-04: the list is built once, and the size
    // can now be changed while the game runs (K1Widescreen_SetTargetResolution), so a game
    // started at the point size has to find the pixel modes there when the player picks one.
    // Until then the ratio was given only for a target above the display's point size. On a
    // display with as many points as pixels it is 1.0 and the list stays vanilla.
    double scale = 1.0;
    if (g_displayPointWidth > 0 && g_displayPointHeight > 0) {
        const double ratio = static_cast<double>(g_displayPixelWidth) / g_displayPointWidth;
        if (ratio > 1.0) scale = ratio;
    }
    uint64_t bits;
    memcpy(&bits, &scale, sizeof bits);
    return bits;
}

static void SizeDialogueLetterbox(int width, int height);
void ApplyLayoutMode();   // mac_widescreen.cpp

extern "C" void KMRP_UseTargetVideoMode(int* width, int* height) {
    // The layout mode is written here, once, rather than when the library loads: no panel exists
    // yet, and a patch loaded after this one has had its say (K1Widescreen_UseGuiFileLayouts).
    ApplyLayoutMode();
    if (!width || !height || g_targetWidth <= 0 || g_targetHeight <= 0) return;
    *width = g_targetWidth;
    *height = g_targetHeight;
    // No dialogue exists yet. The size can change later (K1Widescreen_SetTargetResolution),
    // which sizes the letterbox again.
    SizeDialogueLetterbox(g_targetWidth, g_targetHeight);
}

// The letterbox for a resolution changed while the game runs. Not static: mac_widescreen.cpp
// calls it from K1Widescreen_SetTargetResolution.
void ResizeDialogueLetterbox(int width, int height) { SizeDialogueLetterbox(width, height); }

/*
  Dialogue letterbox sized from the screen height
  ----------------------------------------------------------------------------------------------
  Vanilla keeps a 21:9 band clear in conversations: each bar is (H - W / (7/3)) / 2, the 7/3 a
  float at 0x100570a14 read by exactly six sites (CSWGuiDialogLetterbox::SetTop and SetBottom,
  the CSWGuiDialogTop constructor and SetReply, the CSWGuiDialogCinematic constructor and
  Reset). Derived from the WIDTH, the bars shrink on wide screens and go negative once the
  screen is wider than 21:9 (3440x1440: -17px, 2560x1080: -8px), taking the subtitle and the
  replies with them. KMRP fixes the same formula on Windows with bar = H / 6 (J0-o's "Scaled
  Letterbox"); here that is one float, W/(1.5*W/H) = 2H/3, so all six sites follow at once.

  The reply panel under the bottom bar is also a hard-coded 100px tall (constructor
  0x100244aa6 and Reset 0x10024505b), whatever the bar's height, and LB_REPLIES keeps the
  98px its layout gives it. With enlarged text only two replies fit and the rest scroll
  (measured 2026-09-29 at 1512x982, FontScale=2: 2 of 3 visible, 67px of bar left unused).
  The panel now spans the bar down to the bottom safe margin (never less than the vanilla
  100), and a replace hook in CSWGuiDialogCinematic::SetExtent stretches LB_REPLIES to the
  panel (hooks file, "Dialogue letterbox"). Reset derives its "restore the message label"
  delta from the same immediate, so that bookkeeping stays exact.
*/
namespace {

const uintptr_t kLetterboxAspect = 0x100570a14;     // float 7/3 (__TEXT,__const)
const uintptr_t kReplyPanelHeightCtor = 0x100244aa9;  // imm32 of mov [rsi+0xc], 100
const uintptr_t kReplyPanelHeightReset = 0x10024505d; // imm32 of mov r15d, 100
const int kSafeMarginY = 36;  // CSWGuiManager::GetSafeMargin (0x1004a011e) returns 48, 36

// Overwrites len bytes of the (read-only) image at addr, but only if they still hold the
// vanilla bytes; anything else means another patch owns the site, so it is left alone.
bool ReplaceImageBytes(uintptr_t addr, const void* expected, const void* value, size_t len) {
    if (memcmp(reinterpret_cast<const void*>(addr), expected, len) != 0) return false;
    vm_address_t page = addr & ~static_cast<uintptr_t>(0xFFF);
    if (vm_protect(mach_task_self(), page, 0x2000, FALSE,
                   VM_PROT_READ | VM_PROT_WRITE | VM_PROT_COPY) != KERN_SUCCESS) {
        return false;
    }
    memcpy(reinterpret_cast<void*>(addr), value, len);
    vm_protect(mach_task_self(), page, 0x2000, FALSE, VM_PROT_READ | VM_PROT_EXECUTE);
    return true;
}

}  // namespace

static void SizeDialogueLetterbox(int width, int height) {
    // What the last call wrote, which the next one replaces: the vanilla values the first time.
    // A site that held anything else the first time is another patch's and stays so.
    static unsigned char writtenAspect[4] = {0x54, 0x55, 0x15, 0x40};  // 2.333333f, one ulp under 7/3
    static int writtenPanel = 100;
    static int lastWidth = 0, lastHeight = 0;
    if (width <= 0 || height <= 0 || (width == lastWidth && height == lastHeight)) return;
    lastWidth = width;
    lastHeight = height;

    const float aspect = 1.5f * static_cast<float>(width) / static_cast<float>(height);
    if (ReplaceImageBytes(kLetterboxAspect, writtenAspect, &aspect, sizeof aspect))
        memcpy(writtenAspect, &aspect, sizeof aspect);

    // The bar exactly as the engine will compute it from that float.
    const float live = *reinterpret_cast<const float*>(kLetterboxAspect);
    const int bar = (height - static_cast<int>(static_cast<float>(width) / live)) / 2;
    int panel = bar - kSafeMarginY;
    if (panel < 100) panel = 100;
    const bool ctor = ReplaceImageBytes(kReplyPanelHeightCtor, &writtenPanel, &panel, sizeof panel);
    const bool reset = ReplaceImageBytes(kReplyPanelHeightReset, &writtenPanel, &panel, sizeof panel);
    if (ctor || reset) writtenPanel = panel;
}

/*
  HUD minimap keeps the vanilla zoom when the radar is enlarged
  ----------------------------------------------------------------------------------------------
  Every HUD template authors the radar (LBL_MAPVIEW) at 120x120 and the map texture (LBL_MAP)
  at 512x512, and the minimap is drawn in radar pixels. When the radar is enlarged (the
  widescreen HUD makes it 130 * HudScale, 195px at 1512x982), the map stays 512px and the
  player sees 1.6x more area with everything smaller (measured 2026-09-29 against vanilla at
  the same spot). KMRP fixes the same thing on Windows (gold v14: .kmz map, .kfg fog).

  The draw (0x100237848) opens a GL viewport of the radar's size (AurGUISetupViewport, which
  also pushes {x, y, w, h} on the viewport stack at 0x1005f4b40), centres LBL_MAP on the player
  (left = radar/2 - x*scale), draws it, then the fog pass (0x100238684), then the arrow.
  Quads are normalised to the viewport by dividing by the stack entry's w/h (0x1001be62e,
  0x1001be6b6); the fog pass divides by the radar size at hud+0x7b10/0x7b14 instead.
  So the fix replays the vanilla 120px radar inside the enlarged viewport:
    0x100237974  place LBL_MAP for a 120px radar (the size stays 512, so its texture
                 coordinates are untouched);
    0x100237a2f  once the viewport is open, make the stack entry and the radar size read 120:
                 map and fog normalise against 120 and land enlarged by radar/120;
    0x100237a4f  after the fog, restore both, so the arrow keeps its own geometry.
  With a 120px radar nothing changes. Rejected: enlarging LBL_MAP's extent instead. The label
  maps its texture by extent, so the map tiled rather than zoomed (tested 2026-09-29).
*/
namespace {
const int kVanillaRadar = 120;
const uintptr_t kViewportStack = 0x1005f4b40;  // 10-byte entries {x, y, w, h, flags}
const uintptr_t kViewportTop = 0x1005f4b34;    // int index of the active entry

struct MinimapZoom {
    bool active = false;
    int radarW = 0, radarH = 0;
    short viewW = 0, viewH = 0;
};
MinimapZoom g_minimap;

inline int& HudInt(char* hud, int offset) { return *reinterpret_cast<int*>(hud + offset); }
inline short* ActiveViewport() {
    const int top = *reinterpret_cast<const int*>(kViewportTop);
    return reinterpret_cast<short*>(kViewportStack + static_cast<uintptr_t>(top) * 10);
}
inline bool RadarEnlarged(char* hud) {
    return HudInt(hud, 0x7b10) > kVanillaRadar && HudInt(hud, 0x7b14) > kVanillaRadar;
}
}  // namespace

extern "C" void KMRP_MinimapMapRect(char* hud, int* rect) {
    if (!hud || !rect || !RadarEnlarged(hud)) return;
    rect[0] += kVanillaRadar / 2 - HudInt(hud, 0x7b10) / 2;
    rect[1] += kVanillaRadar / 2 - HudInt(hud, 0x7b14) / 2;
}

extern "C" void KMRP_MinimapZoomBegin(char* hud) {
    g_minimap.active = false;
    if (!hud || !RadarEnlarged(hud)) return;
    short* view = ActiveViewport();
    g_minimap.radarW = HudInt(hud, 0x7b10);
    g_minimap.radarH = HudInt(hud, 0x7b14);
    g_minimap.viewW = view[2];
    g_minimap.viewH = view[3];
    view[2] = view[3] = kVanillaRadar;
    HudInt(hud, 0x7b10) = HudInt(hud, 0x7b14) = kVanillaRadar;
    g_minimap.active = true;
}

extern "C" void KMRP_MinimapZoomEnd(char* hud) {
    if (!hud || !g_minimap.active) return;
    short* view = ActiveViewport();
    view[2] = g_minimap.viewW;
    view[3] = g_minimap.viewH;
    HudInt(hud, 0x7b10) = g_minimap.radarW;
    HudInt(hud, 0x7b14) = g_minimap.radarH;
    g_minimap.active = false;
}
