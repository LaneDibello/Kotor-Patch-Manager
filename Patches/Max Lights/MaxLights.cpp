// Light block relocation.
//
// Each light takes five env registers counting down from env[87] and the bone palette sits at
// env[18..68], so the fourth light clips it and skinned geometry warps. Capping skinned programs
// at 3 avoided that but lit one character two ways: danglymesh hair draws as a plain trimesh
// through a program with no palette, so it kept the full count while the face stayed on 3.
//
// No program body references above env[92], so the block moves to env[96..140]. Windows rewrites
// the register cursor through RelocateLightBlock below; Clang folded that cursor to immediates, so
// the Linux hooks patch them directly and only the light count comes from here.
//
// This needs more than ARB's guaranteed 96 env parameters. Measured 256 on both radeonsi and
// llvmpipe, so it is an assumption about implementations rather than the spec; a driver at the
// floor would fail to compile rather than degrade.

#include <cstddef>
#include <cstring>

#if !defined(_WIN32)
// Already how a free function is called on i386 System V. The keyword is MSVC's.
#define __cdecl
#endif

namespace {

constexpr int kRegistersPerLight = 5;     // counted out of the generated per-light text
// The LightManager enables units as GL_LIGHT0 + index + 1, so GL_LIGHT7 is the last it can address,
// against a GL_MAX_LIGHTS whose spec floor of 8 is what implementations report. The vertex-program
// path ignores GL light units, so this could be higher, but matching them avoids GL_INVALID_ENUM.
constexpr int kMaxLights = 7;
constexpr int kCappedLights = 3;          // stock behaviour, used when the body cannot be measured
// The top light register once the preamble has run, and where the block moves to. The Linux hooks
// carry both as immediates (0x57 -> 0x8C, plus 0x52 -> 0x87 for the shifted mode), unchecked.
constexpr int kStockLightBase = 87;
constexpr int kRelocatedLightBase = 140;

// Enable assembles the program inside the object: body copied to +8, generated text appended, and
// the object's fields begin at +0x200C. Those offsets are baked into the code, so this cannot grow.
constexpr std::size_t kTextBufferBytes = 0x200C - 8;
// From the generator's format strings, with a three-digit register index as the worst case: one
// light's three chunks, then the preamble and footer.
constexpr std::size_t kGeneratedBytesPerLight = 480;
constexpr std::size_t kGeneratedFixedBytes = 368 + 118;

// Light i spans env[base - 5i] down to env[base - 5i - 4], so this is the lowest register used.
static_assert(kRelocatedLightBase - kMaxLights * kRegistersPerLight + 1 >= 96,
              "the relocated block must stay above the fixed slots at env[88..95]");

constexpr std::size_t kLightCountOffset = 0x2010;

#if defined(_WIN32)
// The cursor is a stack slot at [ebp-4]; the constructor's frame keeps this at [ebp-4] too, and
// the body text at [ebp+8].
constexpr std::size_t kCursorOffset = 4;
constexpr std::size_t kConstructorSelfOffset = 4;
constexpr std::size_t kConstructorSourceOffset = 8;
#endif

// How many lights fit before the assembled text overruns the buffer. The largest skinned body
// leaves room for five, which is why the count is not simply the cap.
int LightsThatFit(const char* source)
{
    const std::size_t used = std::strlen(source) + kGeneratedFixedBytes;
    if (used >= kTextBufferBytes) {
        return 0;
    }
    return static_cast<int>((kTextBufferBytes - used) / kGeneratedBytesPerLight);
}

void ApplyLightCount(unsigned char* self, const char* source)
{
    // The null check guards strlen; a program with no body is unusable whatever count it gets.
    int count = source == nullptr ? kCappedLights : LightsThatFit(source);
    if (count > kMaxLights) {
        count = kMaxLights;
    }

    *reinterpret_cast<int*>(self + kLightCountOffset) = count;
}

}

#if defined(_WIN32)

extern "C" void __cdecl RelocateLightBlock(unsigned char* framePointer)
{
    int* const cursor = reinterpret_cast<int*>(framePointer - kCursorOffset);
    // Both sites sit just past the preamble, where the cursor has reached the stock base. Any
    // other value means this is not the layout these addresses were measured against.
    if (*cursor == kStockLightBase) {
        *cursor = kRelocatedLightBase;
    }
}

extern "C" void __cdecl SetProgramLightCount(unsigned char* framePointer)
{
    ApplyLightCount(*reinterpret_cast<unsigned char**>(framePointer - kConstructorSelfOffset),
                    *reinterpret_cast<const char* const*>(framePointer + kConstructorSourceOffset));
}

#else

// The constructor takes both in registers here, EDX and ECX, so the hook receives them directly.
extern "C" void __cdecl SetProgramLightCount(unsigned char* self, const char* source)
{
    ApplyLightCount(self, source);
}

#endif
