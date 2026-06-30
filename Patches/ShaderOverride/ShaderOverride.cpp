#include <windows.h>

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <string>
#include <vector>
#include <bcrypt.h>

#pragma comment(lib, "bcrypt.lib")

typedef unsigned int GLenum;
typedef int GLsizei;
typedef unsigned int GLuint;
typedef unsigned int GLbitfield;
typedef double GLdouble;
typedef float GLfloat;
typedef unsigned char GLubyte;

constexpr GLenum GL_TEXTURE_2D = 0x0DE1;
constexpr GLenum GL_DEPTH_TEST = 0x0B71;
constexpr GLenum GL_CULL_FACE = 0x0B44;
constexpr GLenum GL_LIGHTING = 0x0B50;
constexpr GLenum GL_BLEND = 0x0BE2;
constexpr GLenum GL_PROJECTION = 0x1701;
constexpr GLenum GL_MODELVIEW = 0x1700;
constexpr GLenum GL_QUADS = 0x0007;
constexpr GLenum GL_UNSIGNED_BYTE = 0x1401;
constexpr GLenum GL_VIEWPORT = 0x0BA2;
constexpr GLbitfield GL_ALL_ATTRIB_BITS = 0xFFFFFFFFu;

typedef HGLRC (WINAPI* WglGetCurrentContextFn)();
typedef BOOL (WINAPI* WglUseFontBitmapsAFn)(HDC, DWORD, DWORD, DWORD);
typedef SHORT (WINAPI* GetAsyncKeyStateFn)(int);
typedef HFONT (WINAPI* CreateFontAFn)(int, int, int, int, int, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, LPCSTR);
typedef BOOL (WINAPI* DeleteObjectFn)(HGDIOBJ);
typedef HGDIOBJ (WINAPI* SelectObjectFn)(HDC, HGDIOBJ);
typedef void (APIENTRY* GlBeginFn)(GLenum);
typedef void (APIENTRY* GlCallListsFn)(GLsizei, GLenum, const void*);
typedef void (APIENTRY* GlColor3fFn)(GLfloat, GLfloat, GLfloat);
typedef void (APIENTRY* GlColor4fFn)(GLfloat, GLfloat, GLfloat, GLfloat);
typedef void (APIENTRY* GlDeleteListsFn)(GLuint, GLsizei);
typedef void (APIENTRY* GlDisableFn)(GLenum);
typedef void (APIENTRY* GlEndFn)();
typedef void (APIENTRY* GlGetIntegervFn)(GLenum, int*);
typedef void (APIENTRY* GlListBaseFn)(GLuint);
typedef void (APIENTRY* GlLoadIdentityFn)();
typedef void (APIENTRY* GlMatrixModeFn)(GLenum);
typedef void (APIENTRY* GlOrthoFn)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);
typedef void (APIENTRY* GlPopAttribFn)();
typedef void (APIENTRY* GlPopMatrixFn)();
typedef void (APIENTRY* GlPushAttribFn)(GLbitfield);
typedef void (APIENTRY* GlPushMatrixFn)();
typedef void (APIENTRY* GlRasterPos2fFn)(GLfloat, GLfloat);
typedef void (APIENTRY* GlVertex2fFn)(GLfloat, GLfloat);

namespace fs = std::filesystem;
namespace {
    constexpr GLenum kTargetFragment = 0x8804;
    constexpr GLenum kTargetVertex = 0x8620;
    constexpr GLenum kFormatAsciiArb = 0x8875;
    constexpr GLuint kFontListBase = 4000;
    constexpr int kVisibleRows = 28;

    using WglGetProcAddressFn = PROC(WINAPI*)(LPCSTR);
    using SwapBuffersFn = BOOL(WINAPI*)(HDC);
    using GlProgramStringArbFn = void(WINAPI*)(GLenum target, GLenum format, GLsizei len, const void* string);
    using GlBindProgramArbFn = void(WINAPI*)(GLenum target, GLuint program);

    struct ShaderEntry {
        GLenum target = 0;
        GLuint programId = 0;
        std::string shaderId;
        std::string legacyShaderId;
        std::string alias;
        std::vector<char> originalSource;
        std::vector<char> overrideSource;
        bool hasOverride = false;
        bool overrideEnabled = false;
        bool disabled = false;
        bool dirty = false;
        bool seen = false;
    };

    WglGetProcAddressFn g_originalWglGetProcAddress = nullptr;
    SwapBuffersFn g_originalSwapBuffers = nullptr;
    GlProgramStringArbFn g_originalProgramStringArb = nullptr;
    GlBindProgramArbFn g_originalBindProgramArb = nullptr;

    WglGetCurrentContextFn g_wglGetCurrentContext = nullptr;
    WglUseFontBitmapsAFn g_wglUseFontBitmapsA = nullptr;
    GetAsyncKeyStateFn g_getAsyncKeyState = nullptr;
    CreateFontAFn g_createFontA = nullptr;
    DeleteObjectFn g_deleteObject = nullptr;
    SelectObjectFn g_selectObject = nullptr;
    GlBeginFn g_glBegin = nullptr;
    GlCallListsFn g_glCallLists = nullptr;
    GlColor3fFn g_glColor3f = nullptr;
    GlColor4fFn g_glColor4f = nullptr;
    GlDeleteListsFn g_glDeleteLists = nullptr;
    GlDisableFn g_glDisable = nullptr;
    GlEndFn g_glEnd = nullptr;
    GlGetIntegervFn g_glGetIntegerv = nullptr;
    GlListBaseFn g_glListBase = nullptr;
    GlLoadIdentityFn g_glLoadIdentity = nullptr;
    GlMatrixModeFn g_glMatrixMode = nullptr;
    GlOrthoFn g_glOrtho = nullptr;
    GlPopAttribFn g_glPopAttrib = nullptr;
    GlPopMatrixFn g_glPopMatrix = nullptr;
    GlPushAttribFn g_glPushAttrib = nullptr;
    GlPushMatrixFn g_glPushMatrix = nullptr;
    GlRasterPos2fFn g_glRasterPos2f = nullptr;
    GlVertex2fFn g_glVertex2f = nullptr;

    std::mutex g_logMutex;
    std::mutex g_identMutex;
    std::mutex g_shaderMutex;

    std::map<std::string, std::string> g_shaderAliases;
    std::map<std::string, std::string> g_legacyShaderAliases;
    bool g_shaderAliasesLoaded = false;

    std::map<std::string, ShaderEntry> g_shaders;
    std::vector<std::string> g_shaderOrder;

    GLuint g_currentFragmentProgram = 0;
    GLuint g_currentVertexProgram = 0;
    bool g_internalProgramBind = false;

    bool g_overlayVisible = false;
    int g_selectedIndex = 0;
    int g_scrollOffset = 0;
    bool g_keyDown[256] = {};

    HGLRC g_fontContext = nullptr;
    bool g_fontReady = false;

    const char kDisabledFragmentSource[] =
        "!!ARBfp1.0\n"
        "MOV result.color, fragment.color.primary;\n"
        "END\n";

    const char kDisabledVertexSource[] =
        "!!ARBvp1.0\n"
        "DP4 result.position.x, state.matrix.mvp.row[0], vertex.position;\n"
        "DP4 result.position.y, state.matrix.mvp.row[1], vertex.position;\n"
        "DP4 result.position.z, state.matrix.mvp.row[2], vertex.position;\n"
        "DP4 result.position.w, state.matrix.mvp.row[3], vertex.position;\n"
        "MOV result.color, vertex.color;\n"
        "MOV result.color.secondary, vertex.color.secondary;\n"
        "MOV result.texcoord[0], vertex.texcoord[0];\n"
        "MOV result.texcoord[1], vertex.texcoord[1];\n"
        "MOV result.texcoord[2], vertex.texcoord[2];\n"
        "MOV result.texcoord[3], vertex.texcoord[3];\n"
        "END\n";

    fs::path GetGameDirectory() {
        char exePath[MAX_PATH] = {};
        GetModuleFileNameA(nullptr, exePath, MAX_PATH);
        return fs::path(exePath).parent_path();
    }

    fs::path GetBaseShaderDirectory() {
        return GetGameDirectory() / "shaders";
    }

    fs::path GetLogPath() {
        return GetBaseShaderDirectory() / "shader_override.log";
    }

    fs::path GetIdentPath() {
        return GetBaseShaderDirectory() / "shader_ident.txt";
    }

    void LogMessage(const std::string& message) {
        std::lock_guard<std::mutex> lock(g_logMutex);
        fs::create_directories(GetBaseShaderDirectory());

        std::ofstream out(GetLogPath(), std::ios::app);
        if (out) {
            out << message << "\n";
        }

        OutputDebugStringA((std::string("[ShaderOverride] ") + message + "\n").c_str());
    }

    const char* TargetName(GLenum target) {
        if (target == kTargetFragment) return "fragment";
        if (target == kTargetVertex) return "vertex";
        return "other";
    }

    uint64_t HashShaderSource(const char* data, size_t size) {
        uint64_t hash = 1469598103934665603ull;
        for (size_t i = 0; i < size; ++i) {
            hash ^= static_cast<unsigned char>(data[i]);
            hash *= 1099511628211ull;
        }
        return hash;
    }

    std::string HashToString(uint64_t hash) {
        char buffer[17] = {};
        sprintf_s(buffer, "%016llx", static_cast<unsigned long long>(hash));
        return std::string(buffer);
    }

    std::string MakeShaderId(GLenum target, const char* data, size_t size) {
        return std::string(TargetName(target)) + "_" + HashToString(HashShaderSource(data, size));
    }

    std::string Md5Hex(const char* data, size_t size) {
        BCRYPT_ALG_HANDLE algorithm = nullptr;
        BCRYPT_HASH_HANDLE hash = nullptr;
        UCHAR digest[16] = {};
        std::string result;

        if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_MD5_ALGORITHM, nullptr, 0) < 0) {
            return "";
        }

        if (BCryptCreateHash(algorithm, &hash, nullptr, 0, nullptr, 0, 0) < 0) {
            BCryptCloseAlgorithmProvider(algorithm, 0);
            return "";
        }

        if (BCryptHashData(hash, reinterpret_cast<PUCHAR>(const_cast<char*>(data)), static_cast<ULONG>(size), 0) >= 0 &&
            BCryptFinishHash(hash, digest, sizeof(digest), 0) >= 0) {
            char buffer[33] = {};
            for (size_t i = 0; i < sizeof(digest); ++i) {
                sprintf_s(buffer + (i * 2), 3, "%02x", digest[i]);
            }
            result = buffer;
        }

        BCryptDestroyHash(hash);
        BCryptCloseAlgorithmProvider(algorithm, 0);
        return result;
    }

    std::string MakeLegacyShaderId(GLenum target, const char* data, size_t size) {
        const char* prefix = target == kTargetFragment ? "fp" : (target == kTargetVertex ? "vp" : "un");
        const std::string digest = Md5Hex(data, size);
        return digest.empty() ? "" : std::string(prefix) + digest;
    }

    void EnsureShaderDirectories() {
        const fs::path base = GetBaseShaderDirectory();
        fs::create_directories(base / "dump");
    }

    void WriteFileIfMissing(const fs::path& path, const char* data, size_t size) {
        if (fs::exists(path)) {
            return;
        }

        std::ofstream out(path, std::ios::binary);
        if (!out) {
            return;
        }

        out.write(data, static_cast<std::streamsize>(size));
    }

    std::string Trim(const std::string& value) {
        const char* whitespace = " \t\r\n";
        const size_t start = value.find_first_not_of(whitespace);
        if (start == std::string::npos) {
            return "";
        }

        const size_t end = value.find_last_not_of(whitespace);
        return value.substr(start, end - start + 1);
    }

    void LoadShaderAliasesIfNeeded() {
        std::lock_guard<std::mutex> lock(g_identMutex);
        if (g_shaderAliasesLoaded) {
            return;
        }

        g_shaderAliasesLoaded = true;
        std::ifstream in(GetIdentPath());
        if (!in) {
            LogMessage("shader_ident.txt not found, using hash names only");
            return;
        }

        std::string line;
        while (std::getline(in, line)) {
            line = Trim(line);
            if (line.empty() || line[0] == '#') {
                continue;
            }

            const size_t pos = line.find('=');
            if (pos == std::string::npos) {
                continue;
            }

            const std::string id = Trim(line.substr(0, pos));
            const std::string alias = Trim(line.substr(pos + 1));
            if (!id.empty() && !alias.empty()) {
                if (id.rfind("fp", 0) == 0 || id.rfind("vp", 0) == 0) {
                    g_legacyShaderAliases[id] = alias;
                } else {
                    g_shaderAliases[id] = alias;
                }
            }
        }

        LogMessage("Loaded shader_ident.txt entries=" + std::to_string(g_shaderAliases.size())
            + " legacy_entries=" + std::to_string(g_legacyShaderAliases.size()));
    }

    bool ResolveRuntimeApis() {
        HMODULE opengl32 = GetModuleHandleA("opengl32.dll");
        HMODULE gdi32 = GetModuleHandleA("gdi32.dll");
        HMODULE user32 = GetModuleHandleA("user32.dll");
        if (!opengl32 || !gdi32 || !user32) {
            return false;
        }

        g_wglGetCurrentContext = reinterpret_cast<WglGetCurrentContextFn>(GetProcAddress(opengl32, "wglGetCurrentContext"));
        g_wglUseFontBitmapsA = reinterpret_cast<WglUseFontBitmapsAFn>(GetProcAddress(opengl32, "wglUseFontBitmapsA"));
        g_glBegin = reinterpret_cast<GlBeginFn>(GetProcAddress(opengl32, "glBegin"));
        g_glCallLists = reinterpret_cast<GlCallListsFn>(GetProcAddress(opengl32, "glCallLists"));
        g_glColor3f = reinterpret_cast<GlColor3fFn>(GetProcAddress(opengl32, "glColor3f"));
        g_glColor4f = reinterpret_cast<GlColor4fFn>(GetProcAddress(opengl32, "glColor4f"));
        g_glDeleteLists = reinterpret_cast<GlDeleteListsFn>(GetProcAddress(opengl32, "glDeleteLists"));
        g_glDisable = reinterpret_cast<GlDisableFn>(GetProcAddress(opengl32, "glDisable"));
        g_glEnd = reinterpret_cast<GlEndFn>(GetProcAddress(opengl32, "glEnd"));
        g_glGetIntegerv = reinterpret_cast<GlGetIntegervFn>(GetProcAddress(opengl32, "glGetIntegerv"));
        g_glListBase = reinterpret_cast<GlListBaseFn>(GetProcAddress(opengl32, "glListBase"));
        g_glLoadIdentity = reinterpret_cast<GlLoadIdentityFn>(GetProcAddress(opengl32, "glLoadIdentity"));
        g_glMatrixMode = reinterpret_cast<GlMatrixModeFn>(GetProcAddress(opengl32, "glMatrixMode"));
        g_glOrtho = reinterpret_cast<GlOrthoFn>(GetProcAddress(opengl32, "glOrtho"));
        g_glPopAttrib = reinterpret_cast<GlPopAttribFn>(GetProcAddress(opengl32, "glPopAttrib"));
        g_glPopMatrix = reinterpret_cast<GlPopMatrixFn>(GetProcAddress(opengl32, "glPopMatrix"));
        g_glPushAttrib = reinterpret_cast<GlPushAttribFn>(GetProcAddress(opengl32, "glPushAttrib"));
        g_glPushMatrix = reinterpret_cast<GlPushMatrixFn>(GetProcAddress(opengl32, "glPushMatrix"));
        g_glRasterPos2f = reinterpret_cast<GlRasterPos2fFn>(GetProcAddress(opengl32, "glRasterPos2f"));
        g_glVertex2f = reinterpret_cast<GlVertex2fFn>(GetProcAddress(opengl32, "glVertex2f"));

        g_createFontA = reinterpret_cast<CreateFontAFn>(GetProcAddress(gdi32, "CreateFontA"));
        g_deleteObject = reinterpret_cast<DeleteObjectFn>(GetProcAddress(gdi32, "DeleteObject"));
        g_selectObject = reinterpret_cast<SelectObjectFn>(GetProcAddress(gdi32, "SelectObject"));
        g_getAsyncKeyState = reinterpret_cast<GetAsyncKeyStateFn>(GetProcAddress(user32, "GetAsyncKeyState"));

        return g_wglGetCurrentContext && g_wglUseFontBitmapsA && g_getAsyncKeyState
            && g_createFontA && g_deleteObject && g_selectObject
            && g_glBegin && g_glCallLists && g_glColor3f && g_glColor4f
            && g_glDeleteLists && g_glDisable && g_glEnd && g_glGetIntegerv && g_glListBase
            && g_glLoadIdentity && g_glMatrixMode && g_glOrtho
            && g_glPopAttrib && g_glPopMatrix && g_glPushAttrib
            && g_glPushMatrix && g_glRasterPos2f && g_glVertex2f;
    }
    std::string LookupAlias(const std::string& shaderId) {
        LoadShaderAliasesIfNeeded();

        std::lock_guard<std::mutex> lock(g_identMutex);
        auto it = g_shaderAliases.find(shaderId);
        if (it == g_shaderAliases.end()) {
            return "";
        }
        return it->second;
    }

    std::string LookupLegacyAlias(const std::string& legacyShaderId) {
        LoadShaderAliasesIfNeeded();

        std::lock_guard<std::mutex> lock(g_identMutex);
        auto it = g_legacyShaderAliases.find(legacyShaderId);
        if (it == g_legacyShaderAliases.end()) {
            return "";
        }
        return it->second;
    }

    void DumpShaderSource(GLenum target, const std::string& shaderId, const char* data, size_t size) {
        const std::string alias = LookupAlias(shaderId);
        const std::string dumpName = alias.empty() ? shaderId : alias;
        const fs::path dumpPath = GetBaseShaderDirectory() / "dump" / (dumpName + ".arb");
        WriteFileIfMissing(dumpPath, data, size);
    }

    bool LoadFile(const fs::path& path, std::vector<char>& outContent) {
        if (!fs::exists(path)) {
            return false;
        }

        std::ifstream in(path, std::ios::binary);
        if (!in) {
            return false;
        }

        outContent.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
        return !outContent.empty();
    }

    void AddOverrideCandidates(std::vector<fs::path>& candidates, const fs::path& baseDir, const std::string& name) {
        if (name.empty()) {
            return;
        }

        candidates.push_back(baseDir / (name + ".arb"));
        candidates.push_back(baseDir / (name + ".txt"));
    }

    bool LoadOverrideForShader(const std::string& shaderId, std::vector<char>& outContent, std::string& loadedName, bool logMiss) {
        const fs::path shadersDir = GetBaseShaderDirectory();

        const std::string alias = LookupAlias(shaderId);
        std::vector<fs::path> candidates;
        AddOverrideCandidates(candidates, shadersDir, alias);
        AddOverrideCandidates(candidates, shadersDir, shaderId);

        for (const fs::path& candidate : candidates) {
            if (LoadFile(candidate, outContent)) {
                loadedName = candidate.string();
                return true;
            }
        }

        if (logMiss) {
            std::string checked = "Override miss for " + shaderId + " checked:";
            for (const fs::path& candidate : candidates) {
                checked += " [" + candidate.string() + "]";
            }
            LogMessage(checked);
        }
        return false;
    }

    const char* GetDisabledSource(GLenum target) {
        return target == kTargetVertex ? kDisabledVertexSource : kDisabledFragmentSource;
    }

    size_t GetDisabledSourceSize(GLenum target) {
        return std::strlen(GetDisabledSource(target));
    }

    GLuint& CurrentProgramRef(GLenum target) {
        return target == kTargetVertex ? g_currentVertexProgram : g_currentFragmentProgram;
    }

    void RefreshOverrideState(ShaderEntry& entry, bool logMiss) {
        std::vector<char> overrideContent;
        std::string loadedName;
        entry.hasOverride = LoadOverrideForShader(entry.shaderId, overrideContent, loadedName, logMiss);
        entry.overrideSource = std::move(overrideContent);
    }

    void RegisterShader(const std::string& shaderId, const std::string& legacyShaderId, GLenum target, GLuint programId, const char* shaderText, size_t shaderSize) {
        std::lock_guard<std::mutex> lock(g_shaderMutex);
        auto [it, inserted] = g_shaders.try_emplace(shaderId);
        ShaderEntry& entry = it->second;
        if (inserted) {
            g_shaderOrder.push_back(shaderId);
            entry.shaderId = shaderId;
            entry.legacyShaderId = legacyShaderId;
            entry.target = target;
            entry.alias = LookupAlias(shaderId);
            if (entry.alias.empty()) {
                entry.alias = LookupLegacyAlias(legacyShaderId);
            }
        }

        entry.target = target;
        entry.legacyShaderId = legacyShaderId;
        entry.alias = LookupAlias(shaderId);
        if (entry.alias.empty()) {
            entry.alias = LookupLegacyAlias(legacyShaderId);
        }
        entry.programId = programId;
        entry.originalSource.assign(shaderText, shaderText + shaderSize);
        entry.seen = true;
        RefreshOverrideState(entry, false);
        if (inserted && entry.hasOverride) {
            entry.overrideEnabled = true;
        }
    }

    void ApplyShaderEntry(ShaderEntry& entry) {
        if (!entry.programId || !g_originalBindProgramArb || !g_originalProgramStringArb || entry.originalSource.empty()) {
            entry.dirty = false;
            return;
        }

        const char* sourcePtr = nullptr;
        size_t sourceSize = 0;
        std::string mode = "original";

        RefreshOverrideState(entry, true);

        if (entry.disabled) {
            sourcePtr = GetDisabledSource(entry.target);
            sourceSize = GetDisabledSourceSize(entry.target);
            mode = "disabled";
        } else if (entry.overrideEnabled && entry.hasOverride && !entry.overrideSource.empty()) {
            sourcePtr = entry.overrideSource.data();
            sourceSize = entry.overrideSource.size();
            mode = "override";
        } else {
            sourcePtr = entry.originalSource.data();
            sourceSize = entry.originalSource.size();
        }

        const GLuint previousProgram = CurrentProgramRef(entry.target);
        g_internalProgramBind = true;
        g_originalBindProgramArb(entry.target, entry.programId);
        g_originalProgramStringArb(entry.target, kFormatAsciiArb, static_cast<GLsizei>(sourceSize), sourcePtr);
        g_originalBindProgramArb(entry.target, previousProgram);
        g_internalProgramBind = false;
        entry.dirty = false;

        LogMessage("Reapplied " + entry.shaderId + " mode=" + mode + " program=" + std::to_string(entry.programId));
    }

    void ApplyDirtyShaders() {
        std::lock_guard<std::mutex> lock(g_shaderMutex);
        for (auto& pair : g_shaders) {
            if (pair.second.dirty) {
                ApplyShaderEntry(pair.second);
            }
        }
    }

    bool KeyPressedOnce(int vk) {
        const SHORT state = g_getAsyncKeyState(vk);
        const bool down = (state & 0x8000) != 0;
        const bool pressed = down && !g_keyDown[vk];
        g_keyDown[vk] = down;
        return pressed;
    }

    void ClampSelection() {
        const int count = static_cast<int>(g_shaderOrder.size());
        if (count <= 0) {
            g_selectedIndex = 0;
            g_scrollOffset = 0;
            return;
        }

        if (g_selectedIndex < 0) {
            g_selectedIndex = 0;
        }
        if (g_selectedIndex >= count) {
            g_selectedIndex = count - 1;
        }
        if (g_selectedIndex < g_scrollOffset) {
            g_scrollOffset = g_selectedIndex;
        }
        if (g_selectedIndex >= g_scrollOffset + kVisibleRows) {
            g_scrollOffset = g_selectedIndex - kVisibleRows + 1;
        }
        if (g_scrollOffset < 0) {
            g_scrollOffset = 0;
        }
    }

    void ToggleSelectedEntry(bool disableToggle) {
        std::lock_guard<std::mutex> lock(g_shaderMutex);
        if (g_shaderOrder.empty() || g_selectedIndex < 0 || g_selectedIndex >= static_cast<int>(g_shaderOrder.size())) {
            return;
        }

        ShaderEntry& entry = g_shaders[g_shaderOrder[g_selectedIndex]];
        if (disableToggle) {
            entry.disabled = !entry.disabled;
        } else {
            entry.overrideEnabled = !entry.overrideEnabled;
            if (!entry.overrideEnabled) {
                entry.disabled = false;
            }
        }
        entry.dirty = true;
        LogMessage("Menu toggle " + entry.shaderId + " override=" + std::to_string(entry.overrideEnabled) + " disabled=" + std::to_string(entry.disabled));
    }

    void HandleOverlayInput() {
        if (KeyPressedOnce(VK_F10)) {
            g_overlayVisible = !g_overlayVisible;
            LogMessage(std::string("Overlay ") + (g_overlayVisible ? "opened" : "closed"));
        }

        if (!g_overlayVisible) {
            return;
        }

        if (KeyPressedOnce(VK_UP)) {
            --g_selectedIndex;
        }
        if (KeyPressedOnce(VK_DOWN)) {
            ++g_selectedIndex;
        }
        if (KeyPressedOnce(VK_PRIOR)) {
            g_selectedIndex -= kVisibleRows;
        }
        if (KeyPressedOnce(VK_NEXT)) {
            g_selectedIndex += kVisibleRows;
        }
        ClampSelection();

        if (KeyPressedOnce(VK_RETURN)) {
            const bool shiftDown = (g_getAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
            ToggleSelectedEntry(shiftDown);
        }
    }

    bool EnsureFont(HDC hdc) {
        HGLRC currentContext = g_wglGetCurrentContext();
        if (!currentContext) {
            return false;
        }

        if (g_fontReady && g_fontContext == currentContext) {
            return true;
        }

        HFONT font = g_createFontA(
            -14,
            0,
            0,
            0,
            FW_NORMAL,
            FALSE,
            FALSE,
            FALSE,
            ANSI_CHARSET,
            OUT_TT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            ANTIALIASED_QUALITY,
            FF_DONTCARE | DEFAULT_PITCH,
            "Consolas");
        if (!font) {
            return false;
        }

        HGDIOBJ oldFont = g_selectObject(hdc, font);
        g_glDeleteLists(kFontListBase, 96);
        const BOOL ok = g_wglUseFontBitmapsA(hdc, 32, 96, kFontListBase);
        g_selectObject(hdc, oldFont);
        g_deleteObject(font);

        if (!ok) {
            LogMessage("wglUseFontBitmapsA failed");
            return false;
        }

        g_fontContext = currentContext;
        g_fontReady = true;
        return true;
    }

    void DrawTextLine(float x, float y, const std::string& text) {
        g_glRasterPos2f(x, y);
        g_glListBase(kFontListBase - 32);
        g_glCallLists(static_cast<GLsizei>(text.size()), GL_UNSIGNED_BYTE, text.c_str());
    }

    std::string EntryDisplayName(const ShaderEntry& entry) {
        if (!entry.alias.empty()) {
            return entry.alias;
        }
        return entry.shaderId;
    }

    std::string EntryModeLabel(const ShaderEntry& entry) {
        if (entry.disabled) {
            return "OFF";
        }
        if (entry.overrideEnabled && entry.hasOverride) {
            return "OVR";
        }
        if (entry.overrideEnabled && !entry.hasOverride) {
            return "REQ";
        }
        return "ORG";
    }

    void DrawOverlay(HDC hdc) {
        if (!g_overlayVisible) {
            return;
        }
        if (!EnsureFont(hdc)) {
            return;
        }

        std::vector<ShaderEntry> entries;
        {
            std::lock_guard<std::mutex> lock(g_shaderMutex);
            for (const std::string& shaderId : g_shaderOrder) {
                entries.push_back(g_shaders[shaderId]);
            }
        }

        ClampSelection();

        int viewport[4] = { 0, 0, 1280, 720 };
        g_glGetIntegerv(GL_VIEWPORT, viewport);
        const int viewportWidth = viewport[2] > 0 ? viewport[2] : 1280;
        const int viewportHeight = viewport[3] > 0 ? viewport[3] : 720;

        g_glPushAttrib(GL_ALL_ATTRIB_BITS);
        g_glDisable(GL_TEXTURE_2D);
        g_glDisable(GL_DEPTH_TEST);
        g_glDisable(GL_CULL_FACE);
        g_glDisable(GL_LIGHTING);
        g_glDisable(GL_BLEND);

        g_glMatrixMode(GL_PROJECTION);
        g_glPushMatrix();
        g_glLoadIdentity();
        g_glOrtho(0.0, static_cast<GLdouble>(viewportWidth), static_cast<GLdouble>(viewportHeight), 0.0, -1.0, 1.0);
        g_glMatrixMode(GL_MODELVIEW);
        g_glPushMatrix();
        g_glLoadIdentity();

        g_glColor4f(0.0f, 0.0f, 0.0f, 0.35f);
        g_glBegin(GL_QUADS);
        g_glVertex2f(0.0f, 0.0f);
        g_glVertex2f(800.0f, 0.0f);
        g_glVertex2f(800.0f, 600.0f);
        g_glVertex2f(0.0f, 600.0f);
        g_glEnd();

        g_glColor3f(1.0f, 1.0f, 0.0f);
        DrawTextLine(12.0f, 24.0f, "Shader Override  F10 hide  Up/Down nav  Enter toggle  Shift+Enter off");

        float y = 52.0f;
        const int maxIndex = min(static_cast<int>(entries.size()), g_scrollOffset + kVisibleRows);
        for (int i = g_scrollOffset; i < maxIndex; ++i) {
            const ShaderEntry& entry = entries[i];
            const bool selected = (i == g_selectedIndex);
            if (selected) {
                g_glColor4f(0.2f, 0.35f, 0.65f, 0.45f);
                g_glBegin(GL_QUADS);
                g_glVertex2f(8.0f, y - 14.0f);
                g_glVertex2f(792.0f, y - 14.0f);
                g_glVertex2f(792.0f, y + 4.0f);
                g_glVertex2f(8.0f, y + 4.0f);
                g_glEnd();
            }

            if (entry.disabled) {
                g_glColor3f(1.0f, 0.35f, 0.35f);
            } else if (entry.overrideEnabled && entry.hasOverride) {
                g_glColor3f(0.35f, 1.0f, 0.35f);
            } else if (entry.overrideEnabled) {
                g_glColor3f(1.0f, 0.7f, 0.35f);
            } else {
                g_glColor3f(0.9f, 0.9f, 0.9f);
            }

            const std::string line =
                (selected ? "> " : "  ")
                + EntryModeLabel(entry)
                + "  "
                + TargetName(entry.target)
                + "  "
                + EntryDisplayName(entry)
                + "  [program "
                + std::to_string(entry.programId)
                + "]";
            DrawTextLine(16.0f, y, line);
            y += 20.0f;
        }

        g_glMatrixMode(GL_MODELVIEW);
        g_glPopMatrix();
        g_glMatrixMode(GL_PROJECTION);
        g_glPopMatrix();
        g_glPopAttrib();
    }
    void WINAPI MyGlProgramString(GLenum target, GLenum format, GLsizei len, const void* string) {
        if (!g_originalProgramStringArb) {
            LogMessage("MyGlProgramString invoked before original function was captured");
            return;
        }

        if (format != kFormatAsciiArb || string == nullptr || len <= 0) {
            g_originalProgramStringArb(target, format, len, string);
            return;
        }

        const char* shaderText = static_cast<const char*>(string);
        const size_t shaderSize = static_cast<size_t>(len);
        const std::string shaderId = MakeShaderId(target, shaderText, shaderSize);
        const std::string legacyShaderId = MakeLegacyShaderId(target, shaderText, shaderSize);
        std::string alias = LookupAlias(shaderId);
        if (alias.empty()) {
            alias = LookupLegacyAlias(legacyShaderId);
        }
        const GLuint programId = CurrentProgramRef(target);

        LogMessage(
            std::string("Intercepted ")
            + shaderId
            + (legacyShaderId.empty() ? "" : " legacy=" + legacyShaderId)
            + (alias.empty() ? "" : " alias=" + alias)
            + " len="
            + std::to_string(len));

        DumpShaderSource(target, shaderId, shaderText, shaderSize);
        RegisterShader(shaderId, legacyShaderId, target, programId, shaderText, shaderSize);

        std::lock_guard<std::mutex> lock(g_shaderMutex);
        ShaderEntry& entry = g_shaders[shaderId];

        const char* sourcePtr = shaderText;
        size_t sourceSize = shaderSize;

        if (entry.disabled) {
            sourcePtr = GetDisabledSource(target);
            sourceSize = GetDisabledSourceSize(target);
        } else if (entry.overrideEnabled && entry.hasOverride && !entry.overrideSource.empty()) {
            sourcePtr = entry.overrideSource.data();
            sourceSize = entry.overrideSource.size();
            LogMessage("Using override for " + shaderId + " len=" + std::to_string(sourceSize));
        }

        g_originalProgramStringArb(target, format, static_cast<GLsizei>(sourceSize), sourcePtr);
    }

    void WINAPI MyGlBindProgram(GLenum target, GLuint program) {
        if (!g_internalProgramBind) {
            CurrentProgramRef(target) = program;
        }
        if (g_originalBindProgramArb) {
            g_originalBindProgramArb(target, program);
        }
    }

    BOOL WINAPI MySwapBuffers(HDC hdc) {
        HandleOverlayInput();
        ApplyDirtyShaders();
        DrawOverlay(hdc);
        return g_originalSwapBuffers ? g_originalSwapBuffers(hdc) : FALSE;
    }

    PROC WINAPI MyWglGetProcAddress(LPCSTR name) {
        if (!g_originalWglGetProcAddress) {
            return nullptr;
        }

        PROC resolved = g_originalWglGetProcAddress(name);
        if (!name) {
            return resolved;
        }

        if (std::strcmp(name, "glProgramString") == 0 || std::strcmp(name, "glProgramStringARB") == 0) {
            g_originalProgramStringArb = reinterpret_cast<GlProgramStringArbFn>(resolved);
            LogMessage(std::string("Resolved ") + name + " -> hooked");
            return reinterpret_cast<PROC>(&MyGlProgramString);
        }

        if (std::strcmp(name, "glBindProgramARB") == 0 || std::strcmp(name, "glBindProgram") == 0) {
            g_originalBindProgramArb = reinterpret_cast<GlBindProgramArbFn>(resolved);
            LogMessage(std::string("Resolved ") + name + " -> hooked");
            return reinterpret_cast<PROC>(&MyGlBindProgram);
        }

        return resolved;
    }

    bool PatchMainModuleIat(const char* importedModule, const char* functionName, void* replacement, void** originalOut) {
        HMODULE module = GetModuleHandleA(nullptr);
        if (!module) {
            return false;
        }

        auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(module);
        auto* nt = reinterpret_cast<IMAGE_NT_HEADERS*>(reinterpret_cast<BYTE*>(module) + dos->e_lfanew);
        auto& importDir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
        if (!importDir.VirtualAddress) {
            return false;
        }

        auto* importDesc = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(reinterpret_cast<BYTE*>(module) + importDir.VirtualAddress);
        for (; importDesc->Name; ++importDesc) {
            const char* modName = reinterpret_cast<const char*>(reinterpret_cast<BYTE*>(module) + importDesc->Name);
            if (_stricmp(modName, importedModule) != 0) {
                continue;
            }

            auto* thunk = reinterpret_cast<IMAGE_THUNK_DATA*>(reinterpret_cast<BYTE*>(module) + importDesc->FirstThunk);
            auto* origThunk = reinterpret_cast<IMAGE_THUNK_DATA*>(reinterpret_cast<BYTE*>(module) + importDesc->OriginalFirstThunk);

            for (; origThunk->u1.AddressOfData; ++origThunk, ++thunk) {
                if (IMAGE_SNAP_BY_ORDINAL(origThunk->u1.Ordinal)) {
                    continue;
                }

                auto* importByName = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(reinterpret_cast<BYTE*>(module) + origThunk->u1.AddressOfData);
                if (std::strcmp(reinterpret_cast<const char*>(importByName->Name), functionName) != 0) {
                    continue;
                }

                DWORD oldProtect = 0;
                if (!VirtualProtect(&thunk->u1.Function, sizeof(void*), PAGE_READWRITE, &oldProtect)) {
                    return false;
                }

                if (originalOut) {
                    *originalOut = reinterpret_cast<void*>(thunk->u1.Function);
                }

                thunk->u1.Function = reinterpret_cast<ULONG_PTR>(replacement);
                DWORD ignored = 0;
                VirtualProtect(&thunk->u1.Function, sizeof(void*), oldProtect, &ignored);
                return true;
            }
        }

        return false;
    }

    DWORD WINAPI InstallHookThread(LPVOID) {
        EnsureShaderDirectories();
        LogMessage("InstallHookThread started");

        HMODULE opengl32 = nullptr;
        for (int i = 0; i < 240; ++i) {
            opengl32 = GetModuleHandleA("opengl32.dll");
            if (opengl32) {
                break;
            }

            if ((i % 20) == 0) {
                LogMessage("Waiting for opengl32.dll...");
            }
            Sleep(250);
        }

        if (!opengl32) {
            LogMessage("Timed out waiting for opengl32.dll");
            return 0;
        }

        if (!ResolveRuntimeApis()) {
            LogMessage("Failed to resolve runtime OpenGL/User32/GDI32 APIs");
            return 0;
        }

        if (!PatchMainModuleIat("OPENGL32.dll", "wglGetProcAddress", reinterpret_cast<void*>(&MyWglGetProcAddress), reinterpret_cast<void**>(&g_originalWglGetProcAddress)) &&
            !PatchMainModuleIat("opengl32.dll", "wglGetProcAddress", reinterpret_cast<void*>(&MyWglGetProcAddress), reinterpret_cast<void**>(&g_originalWglGetProcAddress))) {
            LogMessage("Failed to patch main module IAT for wglGetProcAddress");
            return 0;
        }

        if (!PatchMainModuleIat("GDI32.dll", "SwapBuffers", reinterpret_cast<void*>(&MySwapBuffers), reinterpret_cast<void**>(&g_originalSwapBuffers)) &&
            !PatchMainModuleIat("gdi32.dll", "SwapBuffers", reinterpret_cast<void*>(&MySwapBuffers), reinterpret_cast<void**>(&g_originalSwapBuffers))) {
            LogMessage("Failed to patch main module IAT for SwapBuffers");
            return 0;
        }

        if (!g_originalWglGetProcAddress) {
            LogMessage("Original wglGetProcAddress was null after IAT patch");
            return 0;
        }
        if (!g_originalSwapBuffers) {
            LogMessage("Original SwapBuffers was null after IAT patch");
            return 0;
        }

        LogMessage("Installed IAT hook for wglGetProcAddress");
        LogMessage("Installed IAT hook for SwapBuffers");
        return 0;
    }
}

extern "C" void __cdecl ShaderOverride_NoOp() {
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID) {
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        LogMessage("DLL_PROCESS_ATTACH");
        {
            HANDLE thread = CreateThread(nullptr, 0, &InstallHookThread, nullptr, 0, nullptr);
            if (thread) {
                CloseHandle(thread);
            } else {
                LogMessage("CreateThread failed");
            }
        }
        break;
    case DLL_PROCESS_DETACH:
        LogMessage("DLL_PROCESS_DETACH");
        break;
    }
    return TRUE;
}

























