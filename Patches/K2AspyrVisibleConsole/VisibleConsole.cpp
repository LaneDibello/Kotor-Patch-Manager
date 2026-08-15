#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/GL.h>
#include <cstring>

#pragma comment(lib, "opengl32.lib")

namespace {
constexpr uintptr_t kConsoleGlyphsAddress = 0x009F5508;
constexpr int kFirstGlyph = 32;
constexpr int kGlyphCount = 95;
constexpr int kGlyphWidth = 8;
constexpr int kGlyphHeight = 13;
constexpr int kGlyphAdvance = 10;
constexpr int kLineAdvance = 14;

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
