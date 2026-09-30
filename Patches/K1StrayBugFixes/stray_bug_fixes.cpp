/*
 ==============================================================================================
  STRAY BUG FIXES (ported from KMRP for Windows to the Aspyr macOS build)
 ==============================================================================================
  Pure bug fixes to the vanilla KotOR engine, beneficial to all players regardless of
  resolution or aspect ratio. Most fixes are byte patches declared in
  kotor1-steam-aspyr-macos.hooks.toml; this file provides C++ detour implementations.

  Target: KOTOR_Exe 1.4.0 (176481), SHA-256 C1FCB8D3...6D71.
 ==============================================================================================
*/

#include <stdint.h>
#include <string.h>

/*
  Leading newline in GUI text
  ----------------------------------------------------------------------------------------------
  Item and quest descriptions are composed by prefixing "\n" to each property line, so a
  description that opens with a property block starts with a newline and renders an empty
  first line (vanilla behaviour; ~16px at 800x600, very visible once text is enlarged).

  CSWGuiTextParams::SetText (0x1004a3714) is the one setter every GUI text control goes
  through. The detour sits right after its CExoString assignment (0x1004a3726) and trims the
  leading newlines in place, before the string reaches the text object. Trimming at set time
  keeps the line-breaker and the renderer, two separate passes over the same string, in
  agreement about where lines start.

  CExoString on the 64-bit port: { char* +0x00; uint32 capacity +0x08 }, NUL-terminated
  (see CExoString::operator= at 0x10034ce1e and CStr at 0x10034d2c8), so a memmove within
  the buffer is all it takes.
*/
extern "C" void KMRP_TrimLeadingNewlines(char** exoString) {
    if (!exoString) return;
    char* s = *exoString;
    if (!s || *s != '\n') return;
    size_t skip = 0;
    while (s[skip] == '\n') ++skip;
    memmove(s, s + skip, strlen(s + skip) + 1);
}
