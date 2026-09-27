// What the windowing fixes share: the engine entry points they call and the questions they ask the
// configuration.
//
// Aspyr did not port the engine's platform code, they emulated Win32 under it. Every fix here has the
// same shape: the engine asks the shim for something reasonable, the shim answers with something else,
// and the fix supplies the answer the engine expected. Each explains itself at the top of its own file.
//
// Addresses are the link-time ones from the Ghidra program for this build. The port is ET_EXEC with no
// ASLR slide, so they are usable verbatim.

#ifndef K2_ASPYR_WINDOWING_SHIM_H
#define K2_ASPYR_WINDOWING_SHIM_H

namespace Windowing {

    // CExoString::Format. Reuses the string's buffer when the new text fits and reallocates when it does
    // not, so it may be called on a string that already holds text.
    typedef void (*StringFormatFn)(void*, const char*, ...);
    const StringFormatFn CExoStringFormat = reinterpret_cast<StringFormatFn>(0x0819110A);

    // Reads an integer out of the ini's Graphics Options section, or -1 when the key is absent.
    int ReadGraphicsOption(const char* name);

    // Whether the player asked for the stretched image rather than the aspect-correct one.
    bool StretchWanted();

    // True when a mode cannot fill a surface as-is, which is the only case the scaling fixes act on.
    bool NeedsScaling(int modeW, int modeH, int surfaceW, int surfaceH);

}

#endif
