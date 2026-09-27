// Gives every mode that cannot fill the screen a second row in the resolution list, marked
// "(stretched)", so the choice is made there rather than by editing an ini.
//
// The list was built for this. It formats a row as "%d x %d %s" and fills the %s with "(stretched)" when
// the mode's dmDisplayFixedOutput is DMDFO_STRETCH, and it treats two rows with the same text as
// duplicates. What the port lost is the supply: the shim enumerates each mode once and hardcodes
// dmDisplayFixedOutput to DMDFO_DEFAULT, so there was never a second row to mark.
//
// StretchVariantOfMode and ReportStretchVariant restore the supply by making the mode list twice as long,
// every odd index being the stretched reading of the mode below it. MarkScaledResolutions then labels the
// odd ones. Modes that already fill the screen produce two identical labels and the list's own duplicate
// check drops one, so only modes with a choice to offer get two rows.
//
// Doubling the list changes mode indices for every caller of EnumDisplaySettingsA, not just this one.
// CClientExoAppInternal::ReadVideoModeSettings scans for the largest valid mode, where duplicates do not
// affect the result; SetVideoMode and OnResolutionChosen take an index straight from a list row and so map
// back through the same halving.

// Already how a free function is called on i386 System V. The keyword is MSVC's.
#if !defined(_WIN32)
#define __cdecl
#endif

#include "WindowingShim.h"

#include <cstddef>

namespace {

    const int* const gDesktopWidth = reinterpret_cast<const int*>(0x088CEF8C);
    const int* const gDesktopHeight = reinterpret_cast<const int*>(0x088CEF90);

    // Set by CAurInternalGL::Initialize, so it is settled before the options screen can be opened.
    const unsigned char* const gFullscreen = reinterpret_cast<const unsigned char*>(0x088D0464);

    // Set by StretchVariantOfMode for the enumeration in progress, consumed by ReportStretchVariant when
    // that same call fills in the DEVMODE. Nothing else enumerates modes in between, and the UI that
    // drives this is single threaded.
    int gPendingStretchVariant = 0;

}

// EnumDisplaySettingsA's entry. Halves the mode index so two indices name each mode, keeping the parity
// for the field the stock code clears. Index -1 and -2 answer from SDL rather than the cached list and are
// passed through untouched.
extern "C" void __cdecl StretchVariantOfMode(unsigned* args)
{
    const std::size_t kModeIndex = 2;
    const unsigned kLowestReservedIndex = 0xFFFFFFFEu;

    if (args[kModeIndex] >= kLowestReservedIndex) {
        gPendingStretchVariant = 0;
        return;
    }
    gPendingStretchVariant = static_cast<int>(args[kModeIndex] & 1u);
    args[kModeIndex] >>= 1;
}

// Replaces the store that clears dmDisplayFixedOutput at the end of EnumDisplaySettingsA. The original
// instruction is skipped, so this is the only write to the field.
extern "C" void __cdecl ReportStretchVariant(unsigned char* mode)
{
    const std::size_t kDisplayFixedOutput = 0x38;
    *reinterpret_cast<int*>(mode + kDisplayFixedOutput) = gPendingStretchVariant;
}

// Relabels the stretched reading of a mode, after CExoString::Format has composed "W x H @ R Hz" and
// before the text reaches the control. The port only ever builds that format, not the one the marker was
// written into, so the text has to be recomposed rather than switched.
//
// Recomposed into "W x H (stretched)", the list's own original format, rather than the Hz one with a
// marker appended: in the narrow list box of a low resolution, appending to the longer format pushed the
// numbers out of view. The refresh rate is the part that can go, since both readings of a mode share it
// and the row directly above still shows it.
//
// Windowed presents are 1:1 whatever the desktop is doing, so a window lists no marker and no second row:
// the duplicate labels match and the list drops one.
extern "C" void __cdecl MarkScaledResolutions(void* label, int modeW, int modeH, int modeIndex)
{
    if ((modeIndex & 1) == 0 || *gFullscreen == 0) {
        return;
    }

    if (!Windowing::NeedsScaling(modeW, modeH, *gDesktopWidth, *gDesktopHeight)) {
        return;
    }

    Windowing::CExoStringFormat(label, "%d x %d %s", modeW, modeH, "(stretched)");
}
