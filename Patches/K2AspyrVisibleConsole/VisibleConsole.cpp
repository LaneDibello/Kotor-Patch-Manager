// Visible Console
//
// KOTOR2's cheat console still reads input, still keeps its list of posted strings, and
// still ages them out. What Aspyr removed is the last step. AurPrintString, the call that
// put a line of text on the screen, is an empty function in their builds, so the console
// works and draws nothing.
//
// The font it drew with was left alone: 95 glyphs from space onward, 8 by 13, one byte a
// row, bottom row first, which is what glBitmap wants. That is enough to put the text
// back without touching the console, by drawing the strings the game is already handing
// to a function that discards them.
//
// Two hooks, because there are two shapes of call site. Where the game still calls the
// stub, the arguments are on the stack and DrawConsoleBitmapTextHook reads them from
// there. Where the compiler folded the empty call away, the hook has to land mid-function
// with only the PostedString live in a register, and DrawPostedStringHook takes the
// column and row off the object. Which sites exist is a property of the build, so the
// hooks file for each one picks the entry it needs.

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#pragma comment(lib, "opengl32.lib")
#endif

#include <GL/gl.h>

#include <cstdint>
#include <cstring>

#if !defined(_WIN32)
// Already how a free function is called on i386 System V. The keyword is MSVC's.
#define __cdecl
#endif

namespace {

// The glyph table, still sitting where the drawing code used to read it.
#if defined(_WIN32)
constexpr uintptr_t kConsoleGlyphsAddress = 0x009F5508;  // KOTOR2 Aspyr, Windows
#else
constexpr uintptr_t kConsoleGlyphsAddress = 0x088CF3F4;  // KOTOR2 Aspyr, Linux
#endif

constexpr int kFirstGlyph = 32;
constexpr int kGlyphCount = 95;
constexpr int kGlyphWidth = 8;
constexpr int kGlyphHeight = 13;
constexpr int kGlyphAdvance = 10;
constexpr int kLineAdvance = 14;

// A PostedString keeps the console cell it was posted to in these two fields. Its text is
// the first field, so the object pointer doubles as the string.
constexpr std::size_t kPostedStringColumn = 0x42C;
constexpr std::size_t kPostedStringRow = 0x430;

GLuint gFontListBase = 0;

bool EnsureConsoleFont()
{
    if (gFontListBase != 0 && glIsList(gFontListBase + kFirstGlyph) == GL_TRUE) {
        return true;
    }

    gFontListBase = glGenLists(128);
    if (gFontListBase == 0) {
        return false;
    }

    GLint unpackAlignment = 4;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpackAlignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    const auto* glyphs = reinterpret_cast<const GLubyte*>(kConsoleGlyphsAddress);
    for (int index = 0; index < kGlyphCount; ++index) {
        glNewList(gFontListBase + kFirstGlyph + index, GL_COMPILE);
        glBitmap(
            kGlyphWidth,
            kGlyphHeight,
            0.0f,
            2.0f,
            static_cast<GLfloat>(kGlyphAdvance),
            0.0f,
            glyphs + index * kGlyphHeight);
        glEndList();
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, unpackAlignment);
    return true;
}

void DrawConsoleBitmapText(const char* text, int column, int row)
{
    if (text == nullptr || text[0] == '\0') {
        return;
    }

    GLint viewport[4] = {};
    glGetIntegerv(GL_VIEWPORT, viewport);
    if (viewport[2] < 300 || viewport[3] < 100 || !EnsureConsoleFont()) {
        return;
    }

    const int columns = viewport[2] / kGlyphAdvance;
    const int rows = viewport[3] / kLineAdvance;
    if (column < 0) {
        column += columns - static_cast<int>(std::strlen(text)) + 1;
    }
    if (column < 0) {
        column = 0;
    } else if (column >= columns) {
        column = columns - 1;
    }
    if (row < 0) {
        row += rows;
    }
    if (row < 0) {
        row = 0;
    } else if (row >= rows) {
        row = rows - 1;
    }

    glPushAttrib(GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT |
                 GL_LIGHTING_BIT | GL_LIST_BIT | GL_TEXTURE_BIT);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    GLint previousMatrixMode = GL_MODELVIEW;
    glGetIntegerv(GL_MATRIX_MODE, &previousMatrixMode);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, 1.0, -1.0, 1.0, 0.0, 10.0);

    glRasterPos2f(
        static_cast<GLfloat>(column) / static_cast<GLfloat>(columns),
        1.0f - (2.0f * static_cast<GLfloat>(
            row * kLineAdvance + kGlyphHeight - 2) / static_cast<GLfloat>(viewport[3])));
    glListBase(gFontListBase);
    glCallLists(
        static_cast<GLsizei>(std::strlen(text)),
        GL_UNSIGNED_BYTE,
        reinterpret_cast<const GLubyte*>(text));

    glPopMatrix();
    glMatrixMode(previousMatrixMode);
    glPopAttrib();
}

}

extern "C" void __cdecl DrawConsoleBitmapTextHook(
    const char* const* textSlot,
    const int* columnSlot,
    const int* rowSlot)
{
    if (textSlot == nullptr || columnSlot == nullptr || rowSlot == nullptr) {
        return;
    }

    DrawConsoleBitmapText(*textSlot, *columnSlot, *rowSlot);
}

extern "C" void __cdecl DrawPostedStringHook(const char* postedString)
{
    if (postedString == nullptr) {
        return;
    }

    int column = 0;
    int row = 0;
    std::memcpy(&column, postedString + kPostedStringColumn, sizeof(column));
    std::memcpy(&row, postedString + kPostedStringRow, sizeof(row));

    DrawConsoleBitmapText(postedString, column, row);
}
