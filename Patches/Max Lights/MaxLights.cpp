// Light block relocation.
//
// Each light takes five env registers counting down from env[87] and the bone palette sits at
// env[18..68], so the fourth light clips it and skinned geometry warps. Capping skinned programs
// at 3 avoided that but lit one character two ways: danglymesh hair draws as a plain trimesh
// through a program with no palette, so it kept the full count while the face stayed on 3.
//
// Nothing in either build references above env[92], so the block moves to env[96..140] instead.
// That needs more than the 96 env parameters ARB guarantees, so the base follows what the driver
// reports and otherwise falls back to capping everything at 3. Only the ARB form is ever
// generated, its NV alternative being selected by a flag hardcoded to 1, so NV's 96-register
// c[] bank never sees these indices.

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>

#pragma comment(lib, "opengl32.lib")

#include <cstddef>
#include <cstring>

namespace {

constexpr int kRegistersPerLight = 5;     // counted out of the generated per-light text
// The LightManager enables units as GL_LIGHT0 + index + 1, so GL_LIGHT7 is the last it can
// address. GL_MAX_LIGHTS is implementation-dependent with a spec floor of 8, and implementations
// sit on that floor. No program body binds state.light, so the vertex-program path ignores GL
// light units entirely; matching them anyway keeps it consistent with whatever is lit without a
// program, and avoids raising GL_INVALID_ENUM on units that do not exist.
constexpr int kMaxLights = 7;
constexpr int kCappedLights = 3;          // the last count that clears the palette at env[18..68]
constexpr int kStockLightBase = 87;       // top light register once the preamble has run
constexpr int kRelocatedLightBase = 140;
constexpr int kEnvParametersNeeded = kRelocatedLightBase + 1;

// Enable assembles the program inside the VertexProgram object: the body is copied to +8 and the
// generated text appended, and the object's own fields begin at +0x200C. Nothing can grow that
// room, because those field offsets are baked into the code everywhere.
constexpr std::size_t kTextBufferBytes = 0x200C - 8;
// Measured from the generator's own format strings, taking a three-digit register index as the
// worst case: one light's three chunks, then the preamble and the footer.
constexpr std::size_t kGeneratedBytesPerLight = 480;
constexpr std::size_t kGeneratedFixedBytes = 368 + 118;

// Light i spans env[base - 5i] down to env[base - 5i - 4], so this is the lowest register used.
static_assert(kRelocatedLightBase - kMaxLights * kRegistersPerLight + 1 >= 96,
              "the relocated block must stay above the fixed slots at env[88..95]");

// From the ARB_vertex_program spec.
constexpr GLenum kVertexProgramArb = 0x8620;
constexpr GLenum kMaxProgramEnvParametersArb = 0x88B5;

// Both generator and uploader keep the descending register cursor at [ebp-4].
constexpr std::size_t kCursorOffset = 4;
// VertexProgram's constructor frame keeps this at [ebp-4] and the body text at [ebp+8]; the
// object stores its light count here.
constexpr std::size_t kConstructorSelfOffset = 4;
constexpr std::size_t kConstructorSourceOffset = 8;
constexpr std::size_t kLightCountOffset = 0x2010;

bool QueryEnvParameterRoom()
{
    using GetProgramivArb = void(APIENTRY*)(GLenum, GLenum, GLint*);
    const auto getProgramiv =
        reinterpret_cast<GetProgramivArb>(wglGetProcAddress("glGetProgramivARB"));
    if (getProgramiv == nullptr) {
        return false;
    }

    GLint available = 0;
    getProgramiv(kVertexProgramArb, kMaxProgramEnvParametersArb, &available);
    return available >= kEnvParametersNeeded;
}

// Constructors can run before a context is current, and the query needs one. Answering false then
// is fine; caching it is not, or the relocation would stay off for the whole run.
bool RelocationFits()
{
    static int cached = -1;
    if (cached < 0) {
        if (wglGetCurrentContext() == nullptr) {
            return false;
        }
        cached = QueryEnvParameterRoom() ? 1 : 0;
    }
    return cached != 0;
}

// How many lights fit before the assembled text runs past the buffer. The largest skinned body
// leaves room for five, which is why the count cannot simply be the cap.
int LightsThatFit(const char* source)
{
    const std::size_t used = std::strlen(source) + kGeneratedFixedBytes;
    if (used >= kTextBufferBytes) {
        return 0;
    }
    return static_cast<int>((kTextBufferBytes - used) / kGeneratedBytesPerLight);
}

}

extern "C" void __cdecl RelocateLightBlock(unsigned char* framePointer)
{
    if (!RelocationFits()) {
        return;
    }

    int* const cursor = reinterpret_cast<int*>(framePointer - kCursorOffset);
    // Both sites sit just past the preamble, where the cursor has reached the stock base. Any
    // other value means the layout is not the one these addresses were measured against.
    if (*cursor == kStockLightBase) {
        *cursor = kRelocatedLightBase;
    }
}

extern "C" void __cdecl SetProgramLightCount(unsigned char* framePointer)
{
    unsigned char* const self =
        *reinterpret_cast<unsigned char**>(framePointer - kConstructorSelfOffset);
    const char* const source =
        *reinterpret_cast<const char* const*>(framePointer + kConstructorSourceOffset);

    // Without the relocation the palette still caps this at three.
    int count = RelocationFits() ? kMaxLights : kCappedLights;
    // The null check guards strlen; a program with no body is unusable whatever count it gets.
    const int fits = source == nullptr ? kCappedLights : LightsThatFit(source);
    if (fits < count) {
        count = fits;
    }

    *reinterpret_cast<int*>(self + kLightCountOffset) = count;
}
