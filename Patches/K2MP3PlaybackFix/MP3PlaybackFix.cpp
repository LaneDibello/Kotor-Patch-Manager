// Loose MP3 music for KOTOR II.
//
// CExoStreamingSoundSourceInternal::InitializeSource rewrites every ResType_MP3
// candidate to Left(name, len - 3) + "wav" before probing disk, discarding the ".mp3"
// the resource system resolved. Miles is told ".mp3" regardless, which is why the
// shipped .wav-named music decodes.
//
// PreferLooseMp3Candidates runs after that rewrite and before the first open. Both
// spellings are three characters, so it edits in place.
//
// Off Windows the candidates are still in the engine's spelling here, and its fopen is
// a wrapper that converts them first. ResolvePath calls that converter rather than
// reproducing it.
//
// The candidates are frame locals, so each build has its own entry point for its own
// offsets over shared work.

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#if !defined(_WIN32)
// create-patch finds the exports by this spelling.
#define __cdecl
#endif

namespace {

// RIFF lengths are little endian, and the engine's ByteSwap is empty on both shipping
// architectures.
std::uint32_t ReadLe32(const unsigned char* bytes)
{
    return static_cast<std::uint32_t>(bytes[0]) |
           (static_cast<std::uint32_t>(bytes[1]) << 8) |
           (static_cast<std::uint32_t>(bytes[2]) << 16) |
           (static_cast<std::uint32_t>(bytes[3]) << 24);
}

// CExoString keeps its character pointer first. memcpy because the slot is untyped.
char* TextOf(const unsigned char* exoString)
{
    char* text = nullptr;
    std::memcpy(&text, exoString, sizeof(text));
    return text;
}

// Wider than the 1024 the engine's own path buffers use, because a macOS volume prefix
// expands into an absolute path and can outgrow its input.
constexpr std::size_t kPathLimit = 2048;

#if !defined(_WIN32)

// The width DOS2MacPath builds into, and what its callers hand it.
constexpr std::size_t kEnginePathLimit = 1024;

// The engine's own path conversion, which its fopen wrapper runs over every name before
// opening it. macOS drops a volume alias and expands {kMacVolumeName} through
// CoreFoundation; Linux folds the case. Calling it rather than reproducing it is what
// keeps the probe asking the question the open will ask.
//
// Pinned to the build the hooks target, exactly like the frame offsets below.
#if defined(__APPLE__)
constexpr std::uintptr_t kDos2MacPath = 0x100480ABD;
#else
constexpr std::uintptr_t kDos2MacPath = 0x080B52FD;
#endif

bool ResolvePath(const char* path, char* out, std::size_t capacity)
{
    // DOS2MacPath takes no length and its callers pass kEnginePathLimit, so a name that
    // would overrun it is refused here rather than handed over.
    if (capacity < kEnginePathLimit || std::strlen(path) >= kEnginePathLimit) {
        return false;
    }

    out[0] = '\0';
    reinterpret_cast<void (*)(const char*, char*)>(kDos2MacPath)(path, out);
    return out[0] != '\0';
}

#else

// Windows hands fopen the name as written.
bool ResolvePath(const char* path, char* out, std::size_t capacity)
{
    const std::size_t length = std::strlen(path);
    if (length >= capacity) {
        return false;
    }
    std::memcpy(out, path, length + 1);
    return true;
}

#endif

// Whether the engine's own arithmetic lands inside the file. This is its entire
// discriminator: KOTOR's wrapper holds the data chunk's offset here and lands on the
// audio, while an ordinary RIFF/WAVE holds filesize - 8 and lands at EOF.
bool EngineStartsInsideFile(std::uint32_t sizeField, long size)
{
    return size > 0 && sizeField + 8 < static_cast<std::uint32_t>(size);
}

// The data chunk header's offset, which is what the engine's size field holds: it adds
// 8 to reach the audio. Shipped files declare 50 and their data chunk sits at 50. False
// if the file is not RIFF/WAVE or has no data chunk.
bool RiffDataChunkOffset(std::FILE* file, std::uint32_t* outField = nullptr)
{
    unsigned char preamble[12];
    if (std::fseek(file, 0, SEEK_SET) != 0 ||
        std::fread(preamble, 1, sizeof(preamble), file) != sizeof(preamble) ||
        std::memcmp(preamble, "RIFF", 4) != 0 ||
        std::memcmp(preamble + 8, "WAVE", 4) != 0) {
        return false;
    }

    // Odd-sized chunks carry a pad byte. The bound stops a malformed file walking forever.
    std::uint32_t offset = 12;
    for (int guard = 0; guard < 64; ++guard) {
        unsigned char chunk[8];
        if (std::fseek(file, static_cast<long>(offset), SEEK_SET) != 0 ||
            std::fread(chunk, 1, sizeof(chunk), file) != sizeof(chunk)) {
            return false;
        }

        if (std::memcmp(chunk, "data", 4) == 0) {
            if (outField != nullptr) {
                *outField = offset;
            }
            return true;
        }

        const std::uint32_t size = ReadLe32(chunk + 4);
        offset += 8 + size + (size & 1);
    }
    return false;
}

// Whether the engine would find this path and make something of what it finds. Takes a
// path already put through ResolvePath. Raw MP3 always works, failing the RIFF test and
// taking a fallback that reads the whole file.
bool GameWouldPlay(const char* path)
{
    std::FILE* file = std::fopen(path, "rb");
    if (file == nullptr) {
        return false;
    }

    unsigned char header[8] = {0};
    const bool readHeader = std::fread(header, 1, sizeof(header), file) == sizeof(header);
    const long size = (std::fseek(file, 0, SEEK_END) == 0) ? std::ftell(file) : -1;

    bool playable = true;
    if (readHeader && std::memcmp(header, "RIFF", 4) == 0) {
        if (!EngineStartsInsideFile(ReadLe32(header + 4), size)) {
            // Only worth pointing at when CorrectRiffOffset can aim it somewhere.
            playable = RiffDataChunkOffset(file);
        }
    }

    std::fclose(file);
    return playable;
}

// Swap a trailing "wav" for "mp3" when that is the file the engine would play. The
// engine compares extensions case-insensitively, so this does too.
void PreferLooseMp3(char* path)
{
    if (path == nullptr) {
        return;
    }

    // A bare extension is not a name the engine built.
    const std::size_t length = std::strlen(path);
    if (length < 4) {
        return;
    }

    char* extension = path + length - 3;
    if ((extension[0] | 0x20) != 'w' ||
        (extension[1] | 0x20) != 'a' ||
        (extension[2] | 0x20) != 'v') {
        return;
    }

    char original[3];
    std::memcpy(original, extension, sizeof(original));
    std::memcpy(extension, "mp3", sizeof(original));

    char resolved[kPathLimit];
    if (!ResolvePath(path, resolved, sizeof(resolved)) || !GameWouldPlay(resolved)) {
        std::memcpy(extension, original, sizeof(original));
    }
}

// Unused alternate slots stay default-constructed, and CExoString's default constructor
// zeroes the pointer, so an empty slot ends the walk.
void PreferLooseMp3Candidates(unsigned char* primary, unsigned char* alternates,
                              std::size_t stride, std::size_t count)
{
    PreferLooseMp3(TextOf(primary));

    for (std::size_t index = 0; index < count; ++index) {
        char* text = TextOf(alternates + index * stride);
        if (text == nullptr || *text == '\0') {
            break;
        }
        PreferLooseMp3(text);
    }
}

// Aim the engine's size field at the data chunk when its own arithmetic would overrun
// the file, which is what an ordinary RIFF/WAVE declaring filesize - 8 causes. Shipped
// music already holds the right value and is left alone.
void CorrectRiffOffset(std::uint32_t* sizeField, std::FILE* file)
{
    if (sizeField == nullptr || file == nullptr) {
        return;
    }

    if (std::fseek(file, 0, SEEK_END) != 0) {
        return;
    }

    if (EngineStartsInsideFile(*sizeField, std::ftell(file))) {
        return;  // KOTOR's own wrapper, or otherwise already sane
    }

    std::uint32_t corrected = 0;
    if (RiffDataChunkOffset(file, &corrected)) {
        *sizeField = corrected;
    }
}

#if defined(_WIN32)

// Windows reopens the file instead of borrowing the engine's stream: a FILE belongs to
// the runtime that made it, and these builds link theirs statically. They are also the
// only ones that record which candidate opened, at EBP-0x14.
void CorrectRiffOffsetByPath(std::uint32_t* sizeField, const char* path)
{
    if (sizeField == nullptr || path == nullptr || *path == '\0') {
        return;
    }

    std::FILE* file = std::fopen(path, "rb");
    if (file == nullptr) {
        return;
    }
    CorrectRiffOffset(sizeField, file);
    std::fclose(file);
}

#else

// Linux and macOS share the system C library with the module, so the engine's own
// stream is usable and is exactly the file it opened. Neither records which candidate
// that was, so reopening by path would be a guess.
std::FILE* StreamFile(const unsigned char* self, std::size_t fileOffset)
{
    if (self == nullptr) {
        return nullptr;
    }
    std::FILE* file = nullptr;
    std::memcpy(&file, self + fileOffset, sizeof(file));
    return file;
}

#endif  // !_WIN32

}  // namespace

#if defined(_WIN32)

// GOG Aspyr, hook at 0x0063BD6B. 16 alternates at EBP-0xAC.
extern "C" void __cdecl K2LooseMp3GogAspyr(unsigned char* framePointer)
{
    PreferLooseMp3Candidates(framePointer - 0xBC, framePointer - 0xAC, 8, 16);
}

// Steam Aspyr, hook at 0x0070CDCB. 32 alternates at EBP-0x12C.
extern "C" void __cdecl K2LooseMp3SteamAspyr(unsigned char* framePointer)
{
    PreferLooseMp3Candidates(framePointer - 0x13C, framePointer - 0x12C, 8, 32);
}

// Hook at 0x0063BED4, the load of the RIFF size field.
extern "C" void __cdecl K2Mp3RiffOffsetGogAspyr(unsigned char* framePointer)
{
    CorrectRiffOffsetByPath(reinterpret_cast<std::uint32_t*>(framePointer - 0xD8),
                            TextOf(framePointer - 0x14));
}

// Hook at 0x0070CF34, the same load.
extern "C" void __cdecl K2Mp3RiffOffsetSteamAspyr(unsigned char* framePointer)
{
    CorrectRiffOffsetByPath(reinterpret_cast<std::uint32_t*>(framePointer - 0x158),
                            TextOf(framePointer - 0x14));
}

#elif defined(__APPLE__)

// Steam Aspyr macOS x86_64, hook at 0x1002E4517. CExoString is 16 bytes wide here.
extern "C" void __cdecl K2LooseMp3SteamAspyrMacOs(unsigned char* framePointer)
{
    PreferLooseMp3Candidates(framePointer - 0x8A48, framePointer - 0x230, 16, 32);
}

// Hook at 0x1002E49A1. `this` is still in R15 there, so it comes in directly.
extern "C" void __cdecl K2Mp3RiffOffsetMacOs(unsigned char* framePointer,
                                             unsigned char* self)
{
    CorrectRiffOffset(reinterpret_cast<std::uint32_t*>(framePointer - 0x8AA4),
                      StreamFile(self, 0x258));
}

#else

// Steam Aspyr native Linux, hook at 0x084577E8. Clang kept `this` in EBP and addressed
// the frame off ESP, so this counts upward from the stack pointer.
extern "C" void __cdecl K2LooseMp3SteamAspyrLinux(unsigned char* stackPointer)
{
    PreferLooseMp3Candidates(stackPointer + 0x8990, stackPointer + 0x8880, 8, 32);
}

// Hook at 0x08457CBC. The field is at ESP+0x885C, `this` the argument at ESP+0x89B0.
extern "C" void __cdecl K2Mp3RiffOffsetLinux(unsigned char* stackPointer)
{
    unsigned char* self = nullptr;
    std::memcpy(&self, stackPointer + 0x89B0, sizeof(self));
    CorrectRiffOffset(reinterpret_cast<std::uint32_t*>(stackPointer + 0x885C),
                      StreamFile(self, 0x13C));
}

#endif
