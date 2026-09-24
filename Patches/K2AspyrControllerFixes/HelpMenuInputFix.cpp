// Controller help overlay leaves the game in GUI input with no GUI
//
// The quick menu the Y button raises reaches the help overlay through case 7 of
// CSWGamepadMenuIos::HandleInputEvent, where both halves of the input-class
// handling are inverted.
//
// ShowHelpPage captures the class to return to by reading the current one, but
// the quick menu has already switched to class 2, so it memorises GUI rather than
// the gameplay class the player came from. Case 7 then falls into the tail it
// shares with the menu-dismiss cases, which sets class 0 while the overlay is on
// screen, undoing the class ShowHelpPage just set.
//
// So the overlay runs in gameplay input and closing it restores GUI with no panel
// present, which nothing consumes input in. The hooks file retargets case 7's
// jump past the SetInputClass(0); this corrects what HideHelp restores.

#include <cstdint>

#if !defined(_WIN32)
// Already how a free function is called on i386 System V. The keyword is MSVC's.
#define __cdecl
#endif

namespace {

// Written by ShowHelpPage, handed to SetInputClass by HideHelp.
constexpr uintptr_t kHelpSavedInputClass = 0x08917674;

// From SetInputClass's switch: 0 gameplay, 1 minigame, 2 GUI. A fourth exists;
// what it selects was not established.
constexpr int kInputClassGameplay = 0;
constexpr int kInputClassGui = 2;

} // namespace

// Entry of CSWGuiMainInterface::HideHelp. Only a 2 is rewritten: the quick menu
// is the only GUI that reaches this overlay, so 2 means the capture was taken
// while it owned the class. A 0 or 1 came from gameplay or a minigame.
extern "C" void __cdecl NormaliseHelpReturnInputClass()
{
    int* saved = reinterpret_cast<int*>(kHelpSavedInputClass);
    if (*saved == kInputClassGui) {
        *saved = kInputClassGameplay;
    }
}
