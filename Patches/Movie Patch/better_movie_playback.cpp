#include "better_movie_playback.h"
#include "mitchell_netravali_filter.h"

#include <gl/GL.h>
#include <cstring>
#include <limits>

namespace BetterMoviePlayback {

namespace {

constexpr DWORD BinkCopyAll = 0x80000000;
constexpr DWORD BinkSurface32R = 4;
constexpr GLenum GlClampToEdge = 0x812F;
constexpr GLenum GlActiveTextureArb = 0x84E0;
constexpr GLenum GlMaxTextureUnitsArb = 0x84E2;
constexpr GLenum GlTexture0Arb = 0x84C0;
constexpr GLenum GlFragmentProgramArb = 0x8804;
constexpr GLenum GlVertexProgramArb = 0x8620;
constexpr GLenum GlFragmentShader = 0x8B30;
constexpr GLenum GlCompileStatus = 0x8B81;
constexpr GLenum GlLinkStatus = 0x8B82;
constexpr GLenum GlCurrentProgram = 0x8B8D;

typedef int (__stdcall *BinkCopyToBufferFn)(
    DWORD, void*, int, DWORD, DWORD, DWORD, DWORD);
typedef void (__stdcall *BinkBufferBlitFn)(DWORD, void*, DWORD);
typedef void (APIENTRY *GlActiveTextureArbFn)(GLenum);
typedef GLuint (APIENTRY *GlCreateShaderFn)(GLenum);
typedef void (APIENTRY *GlShaderSourceFn)(GLuint, GLsizei, const char* const*, const GLint*);
typedef void (APIENTRY *GlCompileShaderFn)(GLuint);
typedef void (APIENTRY *GlGetShaderivFn)(GLuint, GLenum, GLint*);
typedef void (APIENTRY *GlGetShaderInfoLogFn)(GLuint, GLsizei, GLsizei*, char*);
typedef void (APIENTRY *GlDeleteShaderFn)(GLuint);
typedef GLuint (APIENTRY *GlCreateProgramFn)();
typedef void (APIENTRY *GlAttachShaderFn)(GLuint, GLuint);
typedef void (APIENTRY *GlLinkProgramFn)(GLuint);
typedef void (APIENTRY *GlGetProgramivFn)(GLuint, GLenum, GLint*);
typedef void (APIENTRY *GlGetProgramInfoLogFn)(GLuint, GLsizei, GLsizei*, char*);
typedef void (APIENTRY *GlDeleteProgramFn)(GLuint);
typedef void (APIENTRY *GlUseProgramFn)(GLuint);
typedef GLint (APIENTRY *GlGetUniformLocationFn)(GLuint, const char*);
typedef void (APIENTRY *GlUniform1iFn)(GLint, GLint);
typedef void (APIENTRY *GlUniform2fFn)(GLint, GLfloat, GLfloat);

struct OpenGlApi {
    decltype(&::wglGetCurrentContext) wglGetCurrentContext = nullptr;
    decltype(&::wglGetCurrentDC) wglGetCurrentDC = nullptr;
    decltype(&::wglGetProcAddress) wglGetProcAddress = nullptr;
    decltype(&::wglCreateContext) wglCreateContext = nullptr;
    decltype(&::wglDeleteContext) wglDeleteContext = nullptr;
    decltype(&::wglMakeCurrent) wglMakeCurrent = nullptr;
    decltype(&::glBegin) glBegin = nullptr;
    decltype(&::glBindTexture) glBindTexture = nullptr;
    decltype(&::glClear) glClear = nullptr;
    decltype(&::glClearColor) glClearColor = nullptr;
    decltype(&::glColorMask) glColorMask = nullptr;
    decltype(&::glDisable) glDisable = nullptr;
    decltype(&::glDrawBuffer) glDrawBuffer = nullptr;
    decltype(&::glEnable) glEnable = nullptr;
    decltype(&::glEnd) glEnd = nullptr;
    decltype(&::glGenTextures) glGenTextures = nullptr;
    decltype(&::glGetError) glGetError = nullptr;
    decltype(&::glGetIntegerv) glGetIntegerv = nullptr;
    decltype(&::glGetString) glGetString = nullptr;
    decltype(&::glLoadIdentity) glLoadIdentity = nullptr;
    decltype(&::glMatrixMode) glMatrixMode = nullptr;
    decltype(&::glOrtho) glOrtho = nullptr;
    decltype(&::glPixelStorei) glPixelStorei = nullptr;
    decltype(&::glPopAttrib) glPopAttrib = nullptr;
    decltype(&::glPopClientAttrib) glPopClientAttrib = nullptr;
    decltype(&::glPopMatrix) glPopMatrix = nullptr;
    decltype(&::glPushAttrib) glPushAttrib = nullptr;
    decltype(&::glPushClientAttrib) glPushClientAttrib = nullptr;
    decltype(&::glPushMatrix) glPushMatrix = nullptr;
    decltype(&::glTexCoord2f) glTexCoord2f = nullptr;
    decltype(&::glTexEnvi) glTexEnvi = nullptr;
    decltype(&::glTexImage2D) glTexImage2D = nullptr;
    decltype(&::glTexParameteri) glTexParameteri = nullptr;
    decltype(&::glTexSubImage2D) glTexSubImage2D = nullptr;
    decltype(&::glVertex2f) glVertex2f = nullptr;
    decltype(&::glViewport) glViewport = nullptr;
    decltype(&::SwapBuffers) swapBuffers = nullptr;
    decltype(&::ChoosePixelFormat) choosePixelFormat = nullptr;
    decltype(&::GetPixelFormat) getPixelFormat = nullptr;
    decltype(&::SetPixelFormat) setPixelFormat = nullptr;
    decltype(&::GetDC) getDC = nullptr;
    decltype(&::ReleaseDC) releaseDC = nullptr;
};

struct ShaderApi {
    GlCreateShaderFn createShader = nullptr;
    GlShaderSourceFn shaderSource = nullptr;
    GlCompileShaderFn compileShader = nullptr;
    GlGetShaderivFn getShaderiv = nullptr;
    GlGetShaderInfoLogFn getShaderInfoLog = nullptr;
    GlDeleteShaderFn deleteShader = nullptr;
    GlCreateProgramFn createProgram = nullptr;
    GlAttachShaderFn attachShader = nullptr;
    GlLinkProgramFn linkProgram = nullptr;
    GlGetProgramivFn getProgramiv = nullptr;
    GlGetProgramInfoLogFn getProgramInfoLog = nullptr;
    GlDeleteProgramFn deleteProgram = nullptr;
    GlUseProgramFn useProgram = nullptr;
    GlGetUniformLocationFn getUniformLocation = nullptr;
    GlUniform1iFn uniform1i = nullptr;
    GlUniform2fFn uniform2f = nullptr;
};

OpenGlApi OpenGl;
bool OpenGlResolved = false;
BYTE* FramePixels = nullptr;
SIZE_T FramePixelsSize = 0;
GLuint MovieTexture = 0;
int TextureWidth = 0;
int TextureHeight = 0;
HGLRC TextureContext = nullptr;
HGLRC ActiveTextureContext = nullptr;
GlActiveTextureArbFn ActiveTexture = nullptr;
ShaderApi Shader;
HGLRC ShaderContext = nullptr;
bool ShaderResolved = false;
bool ShaderProgramAttempted = false;
GLuint MitchellNetravaliProgram = 0;
GLint MovieTextureUniform = -1;
GLint SourceSizeUniform = -1;
GLint TextureSizeUniform = -1;
HWND Kotor2MovieWindow = nullptr;
HDC Kotor2MovieDc = nullptr;
HGLRC Kotor2MovieContext = nullptr;
bool Kotor2ContextSuccessReported = false;
bool Kotor2ContextFailureReported = false;
bool FilterSuccessReported = false;
bool FilterFallbackReported = false;

template <typename T>
bool resolveProc(HMODULE module, const char* name, T& proc) {
    proc = reinterpret_cast<T>(GetProcAddress(module, name));
    return proc != nullptr;
}

bool resolveOpenGl() {
    if (OpenGlResolved) {
        return OpenGl.wglGetCurrentContext != nullptr;
    }
    OpenGlResolved = true;

    HMODULE opengl32 = GetModuleHandleA("opengl32.dll");
    HMODULE gdi32 = GetModuleHandleA("gdi32.dll");
    HMODULE user32 = GetModuleHandleA("user32.dll");
    if (!opengl32 || !gdi32 || !user32) {
        return false;
    }

    bool ok = true;
#define RESOLVE_GL(name) ok = resolveProc(opengl32, #name, OpenGl.name) && ok
    RESOLVE_GL(wglGetCurrentContext);
    RESOLVE_GL(wglGetCurrentDC);
    RESOLVE_GL(wglGetProcAddress);
    RESOLVE_GL(wglCreateContext);
    RESOLVE_GL(wglDeleteContext);
    RESOLVE_GL(wglMakeCurrent);
    RESOLVE_GL(glBegin);
    RESOLVE_GL(glBindTexture);
    RESOLVE_GL(glClear);
    RESOLVE_GL(glClearColor);
    RESOLVE_GL(glColorMask);
    RESOLVE_GL(glDisable);
    RESOLVE_GL(glDrawBuffer);
    RESOLVE_GL(glEnable);
    RESOLVE_GL(glEnd);
    RESOLVE_GL(glGenTextures);
    RESOLVE_GL(glGetError);
    RESOLVE_GL(glGetIntegerv);
    RESOLVE_GL(glGetString);
    RESOLVE_GL(glLoadIdentity);
    RESOLVE_GL(glMatrixMode);
    RESOLVE_GL(glOrtho);
    RESOLVE_GL(glPixelStorei);
    RESOLVE_GL(glPopAttrib);
    RESOLVE_GL(glPopClientAttrib);
    RESOLVE_GL(glPopMatrix);
    RESOLVE_GL(glPushAttrib);
    RESOLVE_GL(glPushClientAttrib);
    RESOLVE_GL(glPushMatrix);
    RESOLVE_GL(glTexCoord2f);
    RESOLVE_GL(glTexEnvi);
    RESOLVE_GL(glTexImage2D);
    RESOLVE_GL(glTexParameteri);
    RESOLVE_GL(glTexSubImage2D);
    RESOLVE_GL(glVertex2f);
    RESOLVE_GL(glViewport);
#undef RESOLVE_GL
    ok = resolveProc(gdi32, "SwapBuffers", OpenGl.swapBuffers) && ok;
    ok = resolveProc(gdi32, "ChoosePixelFormat", OpenGl.choosePixelFormat) && ok;
    ok = resolveProc(gdi32, "GetPixelFormat", OpenGl.getPixelFormat) && ok;
    ok = resolveProc(gdi32, "SetPixelFormat", OpenGl.setPixelFormat) && ok;
    ok = resolveProc(user32, "GetDC", OpenGl.getDC) && ok;
    ok = resolveProc(user32, "ReleaseDC", OpenGl.releaseDC) && ok;
    if (!ok) {
        OpenGl.wglGetCurrentContext = nullptr;
    }
    return ok;
}

void resetCachedContextResources() {
    MovieTexture = 0;
    TextureWidth = 0;
    TextureHeight = 0;
    TextureContext = nullptr;
    ActiveTextureContext = nullptr;
    ActiveTexture = nullptr;
    Shader = ShaderApi{};
    ShaderContext = nullptr;
    ShaderResolved = false;
    ShaderProgramAttempted = false;
    MitchellNetravaliProgram = 0;
    MovieTextureUniform = -1;
    SourceSizeUniform = -1;
    TextureSizeUniform = -1;
}

void releaseKotor2MovieContextInternal() {
    if (Kotor2MovieContext) {
        if (OpenGl.wglGetCurrentContext() == Kotor2MovieContext) {
            OpenGl.wglMakeCurrent(nullptr, nullptr);
        }
        OpenGl.wglDeleteContext(Kotor2MovieContext);
    }
    if (Kotor2MovieDc && Kotor2MovieWindow) {
        OpenGl.releaseDC(Kotor2MovieWindow, Kotor2MovieDc);
    }
    Kotor2MovieWindow = nullptr;
    Kotor2MovieDc = nullptr;
    Kotor2MovieContext = nullptr;
    resetCachedContextResources();
}

bool reportKotor2ContextFailure() {
    if (!Kotor2ContextFailureReported) {
        OutputDebugStringA(
            "[Movie Patch] Could not create the KotOR 2 movie OpenGL context; using original Bink blit.\n");
        Kotor2ContextFailureReported = true;
    }
    return false;
}

template <typename T>
bool resolveContextProc(const char* name, T& proc) {
    PROC address = OpenGl.wglGetProcAddress(name);
    if (address == nullptr || address == reinterpret_cast<PROC>(1) ||
        address == reinterpret_cast<PROC>(2) || address == reinterpret_cast<PROC>(3) ||
        address == reinterpret_cast<PROC>(-1)) {
        proc = nullptr;
        return false;
    }
    proc = reinterpret_cast<T>(address);
    return true;
}

bool resolveShaderApi(HGLRC context) {
    if (ShaderContext != context) {
        Shader = ShaderApi{};
        ShaderContext = context;
        ShaderResolved = false;
        ShaderProgramAttempted = false;
        MitchellNetravaliProgram = 0;
        MovieTextureUniform = -1;
        SourceSizeUniform = -1;
        TextureSizeUniform = -1;
    }
    if (ShaderResolved) {
        return Shader.createShader != nullptr;
    }
    ShaderResolved = true;

    bool ok = true;
#define RESOLVE_SHADER(member, name) ok = resolveContextProc(name, Shader.member) && ok
    RESOLVE_SHADER(createShader, "glCreateShader");
    RESOLVE_SHADER(shaderSource, "glShaderSource");
    RESOLVE_SHADER(compileShader, "glCompileShader");
    RESOLVE_SHADER(getShaderiv, "glGetShaderiv");
    RESOLVE_SHADER(getShaderInfoLog, "glGetShaderInfoLog");
    RESOLVE_SHADER(deleteShader, "glDeleteShader");
    RESOLVE_SHADER(createProgram, "glCreateProgram");
    RESOLVE_SHADER(attachShader, "glAttachShader");
    RESOLVE_SHADER(linkProgram, "glLinkProgram");
    RESOLVE_SHADER(getProgramiv, "glGetProgramiv");
    RESOLVE_SHADER(getProgramInfoLog, "glGetProgramInfoLog");
    RESOLVE_SHADER(deleteProgram, "glDeleteProgram");
    RESOLVE_SHADER(useProgram, "glUseProgram");
    RESOLVE_SHADER(getUniformLocation, "glGetUniformLocation");
    RESOLVE_SHADER(uniform1i, "glUniform1i");
    RESOLVE_SHADER(uniform2f, "glUniform2f");
#undef RESOLVE_SHADER
    if (!ok) {
        Shader.createShader = nullptr;
        OutputDebugStringA(
            "[Movie Patch] Required GLSL functions are unavailable; using bilinear filtering.\n");
    }
    return ok;
}

void reportShaderCompileFailure(GLuint shader) {
    char log[1024] = {};
    GLsizei length = 0;
    Shader.getShaderInfoLog(shader, sizeof(log) - 1, &length, log);
    OutputDebugStringA("[Movie Patch] Mitchell-Netravali shader compilation failed: ");
    OutputDebugStringA(length > 0 ? log : "no driver log available");
    OutputDebugStringA("\n");
}

void reportProgramLinkFailure(GLuint program) {
    char log[1024] = {};
    GLsizei length = 0;
    Shader.getProgramInfoLog(program, sizeof(log) - 1, &length, log);
    OutputDebugStringA("[Movie Patch] Mitchell-Netravali shader linking failed: ");
    OutputDebugStringA(length > 0 ? log : "no driver log available");
    OutputDebugStringA("\n");
}

bool ensureMitchellNetravaliProgram(HGLRC context) {
    if (!resolveShaderApi(context)) {
        return false;
    }
    if (ShaderProgramAttempted) {
        return MitchellNetravaliProgram != 0;
    }
    ShaderProgramAttempted = true;

    GLuint fragmentShader = Shader.createShader(GlFragmentShader);
    if (fragmentShader == 0) {
        OutputDebugStringA(
            "[Movie Patch] Could not create the Mitchell-Netravali shader; using bilinear filtering.\n");
        return false;
    }
    const char* source = MitchellNetravaliFilter::FragmentShaderSource;
    Shader.shaderSource(fragmentShader, 1, &source, nullptr);
    Shader.compileShader(fragmentShader);
    GLint compiled = GL_FALSE;
    Shader.getShaderiv(fragmentShader, GlCompileStatus, &compiled);
    if (compiled != GL_TRUE) {
        reportShaderCompileFailure(fragmentShader);
        Shader.deleteShader(fragmentShader);
        return false;
    }

    GLuint program = Shader.createProgram();
    if (program == 0) {
        OutputDebugStringA(
            "[Movie Patch] Could not create the Mitchell-Netravali program; using bilinear filtering.\n");
        Shader.deleteShader(fragmentShader);
        return false;
    }
    Shader.attachShader(program, fragmentShader);
    Shader.linkProgram(program);
    Shader.deleteShader(fragmentShader);

    GLint linked = GL_FALSE;
    Shader.getProgramiv(program, GlLinkStatus, &linked);
    if (linked != GL_TRUE) {
        reportProgramLinkFailure(program);
        Shader.deleteProgram(program);
        return false;
    }

    MovieTextureUniform = Shader.getUniformLocation(program, "movieTexture");
    SourceSizeUniform = Shader.getUniformLocation(program, "sourceSize");
    TextureSizeUniform = Shader.getUniformLocation(program, "textureSize");
    if (MovieTextureUniform < 0 || SourceSizeUniform < 0 || TextureSizeUniform < 0) {
        OutputDebugStringA(
            "[Movie Patch] Mitchell-Netravali shader uniforms were not found; using bilinear filtering.\n");
        Shader.deleteProgram(program);
        return false;
    }
    MitchellNetravaliProgram = program;
    return true;
}

int nextPowerOfTwo(int value) {
    int result = 1;
    while (result < value && result <= ((std::numeric_limits<int>::max)() / 2)) {
        result *= 2;
    }
    return result;
}

bool ensureFrameBuffer(int width, int height) {
    if (width <= 0 || height <= 0 ||
        width > MaximumMovieDimension || height > MaximumMovieDimension) {
        return false;
    }

    const SIZE_T rowBytes = static_cast<SIZE_T>(width) * 4;
    if (rowBytes > ((std::numeric_limits<SIZE_T>::max)() / static_cast<SIZE_T>(height))) {
        return false;
    }
    const SIZE_T requiredSize = rowBytes * static_cast<SIZE_T>(height);
    if (requiredSize > FramePixelsSize) {
        HANDLE heap = GetProcessHeap();
        BYTE* resized = FramePixels
            ? static_cast<BYTE*>(HeapReAlloc(heap, 0, FramePixels, requiredSize))
            : static_cast<BYTE*>(HeapAlloc(heap, HEAP_ZERO_MEMORY, requiredSize));
        if (!resized) {
            return false;
        }
        FramePixels = resized;
        FramePixelsSize = requiredSize;
    }
    return true;
}

bool ensureTexture(HGLRC context, int width, int height) {
    const int requiredWidth = nextPowerOfTwo(width);
    const int requiredHeight = nextPowerOfTwo(height);
    if (requiredWidth < width || requiredHeight < height) {
        return false;
    }

    if (TextureContext != context) {
        MovieTexture = 0;
        TextureWidth = 0;
        TextureHeight = 0;
        TextureContext = context;
    }
    if (MovieTexture == 0) {
        OpenGl.glGenTextures(1, &MovieTexture);
        if (MovieTexture == 0) {
            return false;
        }
        OpenGl.glBindTexture(GL_TEXTURE_2D, MovieTexture);
        OpenGl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        OpenGl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        OpenGl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GlClampToEdge);
        OpenGl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GlClampToEdge);
    } else {
        OpenGl.glBindTexture(GL_TEXTURE_2D, MovieTexture);
    }
    if (TextureWidth != requiredWidth || TextureHeight != requiredHeight) {
        OpenGl.glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGBA, requiredWidth, requiredHeight,
            0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        TextureWidth = requiredWidth;
        TextureHeight = requiredHeight;
    }
    return true;
}

void resolveActiveTexture(HGLRC context) {
    if (ActiveTextureContext == context) {
        return;
    }
    ActiveTextureContext = context;
    resolveContextProc("glActiveTextureARB", ActiveTexture);
}

void originalBinkBlit(
    DWORD bink,
    DWORD buffer,
    unsigned int rectCount,
    const MovieFilterAddresses& addresses) {
    BinkBufferBlitFn blit =
        *reinterpret_cast<BinkBufferBlitFn*>(addresses.binkBufferBlitIatAddress);
    if (blit && bink && buffer) {
        blit(buffer, reinterpret_cast<void*>(bink + BinkFrameBufferOffset), rectCount);
    }
}

bool renderFilteredFrame(DWORD bink, const MovieFilterAddresses& addresses) {
    if (!resolveOpenGl()) {
        return false;
    }

    HGLRC context = OpenGl.wglGetCurrentContext();
    HDC dc = OpenGl.wglGetCurrentDC();
    if (!context || !dc) {
        return false;
    }
    resolveActiveTexture(context);
    const bool shaderAvailable = resolveShaderApi(context);

    DWORD widthValue = 0;
    DWORD heightValue = 0;
    if (!safeReadDword(reinterpret_cast<void*>(bink), widthValue) ||
        !safeReadDword(reinterpret_cast<void*>(bink + 4), heightValue)) {
        return false;
    }
    const int width = static_cast<int>(widthValue);
    const int height = static_cast<int>(heightValue);
    if (!ensureFrameBuffer(width, height)) {
        return false;
    }

    BinkCopyToBufferFn copy =
        *reinterpret_cast<BinkCopyToBufferFn*>(addresses.binkCopyToBufferIatAddress);
    if (!copy) {
        return false;
    }
    const int copyResult = copy(
        bink, FramePixels, width * 4, static_cast<DWORD>(height), 0, 0,
        BinkCopyAll | BinkSurface32R);
    if (copyResult != 0) {
        return false;
    }

    while (OpenGl.glGetError() != GL_NO_ERROR) {
    }

    GLint previousMatrixMode = GL_MODELVIEW;
    GLint previousActiveTexture = GlTexture0Arb;
    GLint previousProgram = 0;
    OpenGl.glGetIntegerv(GL_MATRIX_MODE, &previousMatrixMode);
    if (ActiveTexture) {
        OpenGl.glGetIntegerv(GlActiveTextureArb, &previousActiveTexture);
    }
    if (shaderAvailable) {
        OpenGl.glGetIntegerv(GlCurrentProgram, &previousProgram);
    }
    OpenGl.glPushAttrib(GL_ALL_ATTRIB_BITS);
    OpenGl.glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);

    if (ActiveTexture) {
        GLint textureUnits = 1;
        OpenGl.glGetIntegerv(GlMaxTextureUnitsArb, &textureUnits);
        if (textureUnits < 1) {
            textureUnits = 1;
        } else if (textureUnits > MaximumTextureUnitsToReset) {
            textureUnits = MaximumTextureUnitsToReset;
        }
        for (GLint unit = 0; unit < textureUnits; ++unit) {
            ActiveTexture(GlTexture0Arb + unit);
            OpenGl.glDisable(GL_TEXTURE_1D);
            OpenGl.glDisable(GL_TEXTURE_2D);
        }
        ActiveTexture(GlTexture0Arb);
    }

    const int targetWidthValue =
        *reinterpret_cast<volatile int*>(addresses.screenWidthAddress);
    const int targetHeightValue =
        *reinterpret_cast<volatile int*>(addresses.screenHeightAddress);
    const int targetWidth = targetWidthValue > 0 ? targetWidthValue : BaseWidth;
    const int targetHeight = targetHeightValue > 0 ? targetHeightValue : BaseHeight;
    int scaledWidth = targetWidth;
    int scaledHeight = static_cast<int>(
        (static_cast<long long>(targetWidth) * height) / width);
    if (scaledHeight > targetHeight) {
        scaledHeight = targetHeight;
        scaledWidth = static_cast<int>(
            (static_cast<long long>(targetHeight) * width) / height);
    }
    const int left = (targetWidth - scaledWidth) / 2;
    const int bottom = (targetHeight - scaledHeight) / 2;

    OpenGl.glViewport(0, 0, targetWidth, targetHeight);
    OpenGl.glDrawBuffer(GL_BACK);
    OpenGl.glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    OpenGl.glDisable(GL_ALPHA_TEST);
    OpenGl.glDisable(GL_BLEND);
    OpenGl.glDisable(GL_CULL_FACE);
    OpenGl.glDisable(GL_DEPTH_TEST);
    OpenGl.glDisable(GL_FOG);
    OpenGl.glDisable(GL_LIGHTING);
    OpenGl.glDisable(GL_SCISSOR_TEST);
    OpenGl.glDisable(GL_STENCIL_TEST);
    const char* extensions = reinterpret_cast<const char*>(OpenGl.glGetString(GL_EXTENSIONS));
    if (extensions && std::strstr(extensions, "GL_ARB_fragment_program")) {
        OpenGl.glDisable(GlFragmentProgramArb);
    }
    if (extensions && std::strstr(extensions, "GL_ARB_vertex_program")) {
        OpenGl.glDisable(GlVertexProgramArb);
    }
    OpenGl.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    OpenGl.glClear(GL_COLOR_BUFFER_BIT);

    OpenGl.glMatrixMode(GL_PROJECTION);
    OpenGl.glPushMatrix();
    OpenGl.glLoadIdentity();
    OpenGl.glOrtho(0.0, targetWidth, 0.0, targetHeight, -1.0, 1.0);
    OpenGl.glMatrixMode(GL_MODELVIEW);
    OpenGl.glPushMatrix();
    OpenGl.glLoadIdentity();

    if (!ensureTexture(context, width, height)) {
        OpenGl.glPopMatrix();
        OpenGl.glMatrixMode(GL_PROJECTION);
        OpenGl.glPopMatrix();
        OpenGl.glMatrixMode(previousMatrixMode);
        OpenGl.glPopClientAttrib();
        OpenGl.glPopAttrib();
        if (ActiveTexture) {
            ActiveTexture(previousActiveTexture);
        }
        return false;
    }

    OpenGl.glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    OpenGl.glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    OpenGl.glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    OpenGl.glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    OpenGl.glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
    OpenGl.glPixelStorei(GL_UNPACK_LSB_FIRST, GL_FALSE);
    OpenGl.glTexSubImage2D(
        GL_TEXTURE_2D, 0, 0, 0, width, height,
        GL_RGBA, GL_UNSIGNED_BYTE, FramePixels);
    OpenGl.glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    OpenGl.glEnable(GL_TEXTURE_2D);

    const bool useMitchellNetravali = ensureMitchellNetravaliProgram(context);
    if (useMitchellNetravali) {
        Shader.useProgram(MitchellNetravaliProgram);
        Shader.uniform1i(MovieTextureUniform, 0);
        Shader.uniform2f(SourceSizeUniform,
            static_cast<GLfloat>(width), static_cast<GLfloat>(height));
        Shader.uniform2f(TextureSizeUniform,
            static_cast<GLfloat>(TextureWidth), static_cast<GLfloat>(TextureHeight));
    } else if (shaderAvailable) {
        Shader.useProgram(0);
    }

    const GLfloat maxU = static_cast<GLfloat>(width) / TextureWidth;
    const GLfloat maxV = static_cast<GLfloat>(height) / TextureHeight;
    OpenGl.glBegin(GL_QUADS);
    OpenGl.glTexCoord2f(0.0f, maxV);
    OpenGl.glVertex2f(static_cast<GLfloat>(left), static_cast<GLfloat>(bottom));
    OpenGl.glTexCoord2f(maxU, maxV);
    OpenGl.glVertex2f(static_cast<GLfloat>(left + scaledWidth), static_cast<GLfloat>(bottom));
    OpenGl.glTexCoord2f(maxU, 0.0f);
    OpenGl.glVertex2f(static_cast<GLfloat>(left + scaledWidth), static_cast<GLfloat>(bottom + scaledHeight));
    OpenGl.glTexCoord2f(0.0f, 0.0f);
    OpenGl.glVertex2f(static_cast<GLfloat>(left), static_cast<GLfloat>(bottom + scaledHeight));
    OpenGl.glEnd();

    const GLenum error = OpenGl.glGetError();
    if (shaderAvailable) {
        Shader.useProgram(static_cast<GLuint>(previousProgram));
    }
    OpenGl.glPopMatrix();
    OpenGl.glMatrixMode(GL_PROJECTION);
    OpenGl.glPopMatrix();
    OpenGl.glMatrixMode(previousMatrixMode);
    OpenGl.glPopClientAttrib();
    OpenGl.glPopAttrib();
    if (ActiveTexture) {
        ActiveTexture(previousActiveTexture);
    }
    return error == GL_NO_ERROR && OpenGl.swapBuffers(dc) != FALSE;
}

}

volatile int* const ScreenWidth = reinterpret_cast<volatile int*>(ScreenWidthAddress);
volatile int* const ScreenHeight = reinterpret_cast<volatile int*>(ScreenHeightAddress);

int screenWidth() {
    const int width = *ScreenWidth;
    return width > 0 ? width : BaseWidth;
}

int screenHeight() {
    const int height = *ScreenHeight;
    return height > 0 ? height : BaseHeight;
}

bool safeReadDword(const void* address, DWORD& value) {
    __try {
        value = *reinterpret_cast<const DWORD*>(address);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        value = 0;
        return false;
    }
}

bool prepareKotor2MovieContext(void* movie) {
    if (!resolveOpenGl()) {
        return reportKotor2ContextFailure();
    }

    DWORD windowValue = 0;
    if (!movie ||
        !safeReadDword(static_cast<char*>(movie) + MovieWindowOffset, windowValue) ||
        windowValue == 0) {
        return reportKotor2ContextFailure();
    }
    HWND window = reinterpret_cast<HWND>(windowValue);

    if (Kotor2MovieContext && Kotor2MovieWindow == window) {
        const bool contextCurrent =
            OpenGl.wglGetCurrentContext() == Kotor2MovieContext ||
            OpenGl.wglMakeCurrent(Kotor2MovieDc, Kotor2MovieContext) != FALSE;
        return contextCurrent || reportKotor2ContextFailure();
    }
    releaseKotor2MovieContextInternal();

    HDC dc = OpenGl.getDC(window);
    if (!dc) {
        return reportKotor2ContextFailure();
    }

    PIXELFORMATDESCRIPTOR descriptor = {};
    descriptor.nSize = sizeof(descriptor);
    descriptor.nVersion = 1;
    descriptor.dwFlags =
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    descriptor.iPixelType = PFD_TYPE_RGBA;
    descriptor.cColorBits = 32;
    descriptor.iLayerType = PFD_MAIN_PLANE;

    int pixelFormat = OpenGl.getPixelFormat(dc);
    if (pixelFormat == 0) {
        pixelFormat = OpenGl.choosePixelFormat(dc, &descriptor);
        if (pixelFormat == 0 ||
            OpenGl.setPixelFormat(dc, pixelFormat, &descriptor) == FALSE) {
            OpenGl.releaseDC(window, dc);
            return reportKotor2ContextFailure();
        }
    }

    HGLRC context = OpenGl.wglCreateContext(dc);
    if (!context || OpenGl.wglMakeCurrent(dc, context) == FALSE) {
        if (context) {
            OpenGl.wglDeleteContext(context);
        }
        OpenGl.releaseDC(window, dc);
        return reportKotor2ContextFailure();
    }

    Kotor2MovieWindow = window;
    Kotor2MovieDc = dc;
    Kotor2MovieContext = context;
    if (!Kotor2ContextSuccessReported) {
        OutputDebugStringA(
            "[Movie Patch] KotOR 2 movie OpenGL context created.\n");
        Kotor2ContextSuccessReported = true;
    }
    return true;
}

void releaseKotor2MovieContext() {
    if (OpenGlResolved && OpenGl.wglGetCurrentContext) {
        releaseKotor2MovieContextInternal();
    }
}

bool blitMovieMitchellNetravaliImpl(
    void* movie,
    unsigned int rectCount,
    const MovieFilterAddresses& addresses) {
    DWORD bink = 0;
    DWORD buffer = 0;
    if (!movie ||
        !safeReadDword(static_cast<char*>(movie) + MovieBinkOffset, bink) ||
        !safeReadDword(static_cast<char*>(movie) + MovieBufferOffset, buffer) ||
        bink == 0 || buffer == 0) {
        return false;
    }

    const bool rendered = renderFilteredFrame(bink, addresses);
    if (!rendered) {
        if (!FilterFallbackReported) {
            OutputDebugStringA(
                "[Movie Patch] Filtered presentation failed; using original Bink blit.\n");
            FilterFallbackReported = true;
        }
        originalBinkBlit(bink, buffer, rectCount, addresses);
    } else if (!FilterSuccessReported) {
        OutputDebugStringA(
            "[Movie Patch] Mitchell-Netravali frame presented successfully.\n");
        FilterSuccessReported = true;
    }
    return rendered;
}

void blitMovieOriginalImpl(
    void* movie,
    unsigned int rectCount,
    const MovieFilterAddresses& addresses) {
    DWORD bink = 0;
    DWORD buffer = 0;
    if (movie &&
        safeReadDword(static_cast<char*>(movie) + MovieBinkOffset, bink) &&
        safeReadDword(static_cast<char*>(movie) + MovieBufferOffset, buffer) &&
        bink != 0 && buffer != 0) {
        originalBinkBlit(bink, buffer, rectCount, addresses);
    }
}

}
