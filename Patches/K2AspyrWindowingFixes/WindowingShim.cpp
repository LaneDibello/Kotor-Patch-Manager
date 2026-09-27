// Reading the game's own ini, and the configuration question the fixes ask of it.
//
// The engine's CExoIni resolves the path, which differs per platform. It also works before the engine has
// read the ini itself: the filename global is a zero-initialised std::string until then, so an early read
// finds nothing and answers -1.

#include "WindowingShim.h"

#include <cstdlib>

namespace {

    // Engine entry points. CExoString and CExoIni both construct into caller-provided storage, so the
    // locals below are the objects themselves rather than handles to them.
    typedef void (*StringInitFn)(void*);
    typedef void (*StringFromCStrFn)(void*, const char*);
    typedef int (*IniEntryFn)(void*, void*, void*, void*, void*);

    const StringInitFn CExoStringInit = reinterpret_cast<StringInitFn>(0x081908E8);
    const StringFromCStrFn CExoStringFromCStr = reinterpret_cast<StringFromCStrFn>(0x081908FA);
    const StringInitFn CExoStringDestroy = reinterpret_cast<StringInitFn>(0x08190A36);
    const StringInitFn CExoIniInit = reinterpret_cast<StringInitFn>(0x08195C8C);
    const StringInitFn CExoIniDestroy = reinterpret_cast<StringInitFn>(0x08195CC4);
    const IniEntryFn CExoIniReadEntry = reinterpret_cast<IniEntryFn>(0x08195CEE);

    // The stretched flag for the mode in use. The options screen writes it from the chosen entry's
    // dmDisplayFixedOutput and reads it back to decide which entry to highlight, but nothing initialises
    // it, so it starts at zero however the last run ended. Seeding it from the ini is what makes the
    // player's choice survive a restart.
    int* const gChosenStretch = reinterpret_cast<int*>(0x0890B094);

    // CExoString is {char* buffer; int capacity}, from CExoString::CStrConstructor.
    struct CExoString {
        char* buffer;
        int capacity;
    };

    // CExoIni holds nothing but a pointer to its heap-allocated internal.
    struct CExoIni {
        void* internal;
    };

    // The ini's filename, as a libc++ std::string: bit 0 of the first byte marks the long form, whose
    // character pointer sits at +8, and the short form's characters start at +1.
    const char* IniFileName()
    {
        const unsigned char* const self = reinterpret_cast<const unsigned char*>(0x08913854);
        if ((self[0] & 1) != 0) {
            return *reinterpret_cast<const char* const*>(0x0891385C);
        }
        return reinterpret_cast<const char*>(self + 1);
    }

    // The three strings every entry needs.
    struct EntryKeys {
        CExoString file;
        CExoString section;
        CExoString key;

        explicit EntryKeys(const char* name)
        {
            CExoStringFromCStr(&file, IniFileName());
            CExoStringFromCStr(&section, "Graphics Options");
            CExoStringFromCStr(&key, name);
        }

        ~EntryKeys()
        {
            CExoStringDestroy(&key);
            CExoStringDestroy(&section);
            CExoStringDestroy(&file);
        }
    };

}

namespace Windowing {

    int ReadGraphicsOption(const char* name)
    {
        CExoString value;
        CExoIni ini;
        CExoStringInit(&value);
        CExoIniInit(&ini);

        {
            EntryKeys keys(name);
            CExoIniReadEntry(&ini, &value, &keys.file, &keys.section, &keys.key);
        }

        // atoi to match how the engine reads every other integer key out of this file.
        const int result = value.buffer != 0 ? std::atoi(value.buffer) : -1;

        CExoIniDestroy(&ini);
        CExoStringDestroy(&value);
        return result;
    }

    // Seeded from the ini on first use, then left to the options screen, which rewrites the global every
    // time a resolution is chosen. Reading the live global rather than a cached copy is what lets a
    // choice made in the options list take effect on the next frame instead of the next launch.
    bool StretchWanted()
    {
        static bool seeded = false;
        if (!seeded) {
            seeded = true;
            *gChosenStretch = ReadGraphicsOption("Stretched") == 1 ? 1 : 0;
        }
        return *gChosenStretch != 0;
    }

    bool NeedsScaling(int modeW, int modeH, int surfaceW, int surfaceH)
    {
        if (modeW <= 0 || modeH <= 0 || surfaceW <= 0 || surfaceH <= 0) {
            return false;
        }
        // Cross multiplication rather than division keeps 4:3 against 1280x960 exact.
        return modeW * surfaceH != surfaceW * modeH;
    }

}
