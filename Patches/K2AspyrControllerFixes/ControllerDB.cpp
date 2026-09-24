// Controller database for KOTOR II's native Linux build
//
// Aspyr linked SDL statically, so the game's controller database is the fourteen
// entries SDL shipped with in 2014. IDirectInput_Mac::EnumDevices gates its loop
// on SDL_IsGameController, so an unlisted pad is absent from the game rather than
// mis-mapped.
//
// Mappings come from a gamecontrollerdb.txt beside the game binary when there is
// one, otherwise from the snapshot compiled into this module. SDL's own fourteen
// load before either and are only ever added to.
//
// Not through SDL_GAMECONTROLLERCONFIG: the database is around 200 KB joined,
// Linux caps one environment string at MAX_ARG_STRLEN (32 * 4096), and
// CASLDisplayDeviceList's constructor feeds an unchecked popen to fread, so the
// resulting E2BIG segfaults the game before the window opens.

#include "ControllerDbEmbedded.h"

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <unistd.h>
#include <vector>

#if !defined(_WIN32)
// Already how a free function is called on i386 System V. The keyword is MSVC's.
#define __cdecl
#endif

namespace {

// SDL_GameControllerAddMapping in KOTOR2 Aspyr native Linux, SHA-256 ED043D21.
// Callers write the argument to [ESP] and clean the stack, so it is cdecl.
constexpr uintptr_t kAddMappingAddress = 0x086718C0;

using AddMappingFn = int(__cdecl*)(const char*);

const char kDatabaseName[] = "gamecontrollerdb.txt";

// SDL_GameControllerAddMapping does no platform filtering, so another platform's
// row would be accepted and could shadow the Linux row for the same pad.
const char kPlatformTag[] = "platform:Linux,";

bool gMappingsAdded = false;

// Upstream tags an "xinput,XInput Controller,..." row for every platform. Its
// first field is a Windows XInput token, not a GUID, and SDL's parser would turn
// it into a mostly-zero value matching no device here.
bool HasGuidPrefix(const std::string& row)
{
    if (row.size() < 33 || row[32] != ',') {
        return false;
    }
    for (std::size_t i = 0; i < 32; ++i) {
        if (std::isxdigit(static_cast<unsigned char>(row[i])) == 0) {
            return false;
        }
    }
    return true;
}

// Beside the game binary, where a player would look, rather than beside this
// module. /proc/self/exe is the game whichever launcher started it.
std::string DatabasePath()
{
    char exePath[4096];
    const ssize_t written = readlink("/proc/self/exe", exePath, sizeof(exePath));
    if (written <= 0) {
        return std::string();
    }

    const std::string self(exePath, static_cast<std::size_t>(written));
    // npos + 1 is 0, so a path with no slash degrades to the working directory.
    return self.substr(0, self.rfind('/') + 1) + kDatabaseName;
}

// Empty when there is no readable file, which is the ordinary case.
std::vector<std::string> ReadExternalMappings()
{
    std::vector<std::string> rows;

    std::ifstream file(DatabasePath().c_str());
    std::string row;
    while (std::getline(file, row)) {
        if (!row.empty() && row.back() == '\r') {
            row.pop_back();
        }
        if (row.empty() || row[0] == '#') {
            continue;
        }
        if (row.find(kPlatformTag) == std::string::npos) {
            continue;
        }
        if (!HasGuidPrefix(row)) {
            continue;
        }
        rows.push_back(row);
    }

    return rows;
}

} // namespace

// Entry of IDirectInput_Mac::EnumDevices, before it reads SDL_NumJoysticks, so
// SDL has initialised and the mappings survive to the enumeration a few
// instructions later. The guard matters: CExoRawInputInternal enumerates twice,
// to count and to create, and ReinitJoysticks again on a device change.
extern "C" void __cdecl LoadControllerMappings()
{
    if (gMappingsAdded) {
        return;
    }
    gMappingsAdded = true;

    const AddMappingFn addMapping =
        reinterpret_cast<AddMappingFn>(kAddMappingAddress);
    int accepted = 0;

    const std::vector<std::string> external = ReadExternalMappings();
    if (!external.empty()) {
        for (std::size_t i = 0; i < external.size(); ++i) {
            // 1 added, 0 replaced, -1 rejected.
            if (addMapping(external[i].c_str()) >= 0) {
                ++accepted;
            }
        }
        std::fprintf(stderr, "[K2ControllerFixes] %d of %zu mappings from %s\n",
                     accepted, external.size(), kDatabaseName);
        return;
    }

    for (int i = 0; i < kEmbeddedLinuxMappingCount; ++i) {
        if (addMapping(kEmbeddedLinuxMappings[i]) >= 0) {
            ++accepted;
        }
    }
    std::fprintf(stderr, "[K2ControllerFixes] %d of %d built-in mappings\n",
                 accepted, kEmbeddedLinuxMappingCount);
}
