#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/GL.h>

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "gdi32.lib")

namespace {
constexpr int kFirstGlyph = 32;
constexpr int kGlyphCount = 96;
constexpr int kCharCellWidth = 12;
constexpr int kCharCellHeight = 20;
constexpr int kMaxTextLength = 512;

struct FontState {
    HGLRC context = nullptr;
    GLuint listBase = 0;
};

FontState g_font;

void DeleteFontLists()
{
    if (g_font.listBase != 0) {
        glDeleteLists(g_font.listBase, kGlyphCount);
    }
    g_font = {};
}

bool EnsureFont()
{
    HGLRC context = wglGetCurrentContext();
    HDC dc = wglGetCurrentDC();
    if (context == nullptr || dc == nullptr) {
        return false;
    }

    if (g_font.listBase != 0 && g_font.context == context) {
        return true;
    }

    DeleteFontLists();

    HFONT font = CreateFontA(
        18, 10, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY,
        FIXED_PITCH | FF_MODERN, "Terminal");
    if (font == nullptr) {
        font = CreateFontA(
            18, 10, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY,
            FIXED_PITCH | FF_MODERN, "Fixedsys");
    }
    if (font == nullptr) {
        font = static_cast<HFONT>(GetStockObject(SYSTEM_FIXED_FONT));
    }

    HGDIOBJ oldFont = SelectObject(dc, font);
    GLuint listBase = glGenLists(kGlyphCount);
    if (listBase == 0) {
        SelectObject(dc, oldFont);
        if (font != GetStockObject(SYSTEM_FIXED_FONT)) {
            DeleteObject(font);
        }
        return false;
    }

    if (!wglUseFontBitmapsA(dc, kFirstGlyph, kGlyphCount, listBase)) {
        glDeleteLists(listBase, kGlyphCount);
        SelectObject(dc, oldFont);
        if (font != GetStockObject(SYSTEM_FIXED_FONT)) {
            DeleteObject(font);
        }
        return false;
    }

    SelectObject(dc, oldFont);
    if (font != GetStockObject(SYSTEM_FIXED_FONT)) {
        DeleteObject(font);
    }

    g_font.context = context;
    g_font.listBase = listBase;
    return true;
}

int StringLength(const char* text)
{
    int length = 0;
    while (text[length] != '\0' && length < kMaxTextLength) {
        ++length;
    }
    return length;
}

void DrawTextLine(const char* text, int column, int line)
{
    if (text == nullptr || text[0] == '\0' || !EnsureFont()) {
        return;
    }

    GLint viewport[4] = {};
    glGetIntegerv(GL_VIEWPORT, viewport);
    int width = viewport[2];
    int height = viewport[3];
    if (width <= 0 || height <= 0) {
        return;
    }

    char buffer[kMaxTextLength + 1] = {};
    int length = StringLength(text);
    for (int i = 0; i < length; ++i) {
        unsigned char ch = static_cast<unsigned char>(text[i]);
        buffer[i] = (ch < kFirstGlyph || ch >= kFirstGlyph + kGlyphCount)
            ? ' '
            : static_cast<char>(ch);
    }
    buffer[length] = '\0';

    int rows = height / kCharCellHeight;
    int columns = width / kCharCellWidth;
    if (rows <= 0 || columns <= 0) {
        return;
    }

    if (line >= rows) {
        line = rows - 1;
    }
    if (line < 0) {
        line += rows;
        if (line < 0) {
            line = 0;
        }
    }

    if (column < 0) {
        column = column + 1 + (columns - length);
    }
    if (column + length > columns) {
        column = columns - length;
    }
    if (column < 0) {
        column = 0;
    }

    int rasterLine = rows - line - 1;
    float x = static_cast<float>(column) / static_cast<float>(columns);
    float y = static_cast<float>(rasterLine) / static_cast<float>(rows);

    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_LIST_BIT | GL_TRANSFORM_BIT);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    GLfloat color[4] = {};
    glGetFloatv(GL_CURRENT_COLOR, color);
    if (color[3] <= 0.01f) {
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    }

    glRasterPos2f(x, y);
    glListBase(g_font.listBase - kFirstGlyph);
    glCallLists(length, GL_UNSIGNED_BYTE, buffer);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopAttrib();
}
}

extern "C" void __cdecl DrawVisibleConsoleText(const char** textSlot, const int* columnSlot, const int* lineSlot)
{
    if (textSlot == nullptr || columnSlot == nullptr || lineSlot == nullptr) {
        return;
    }

    DrawTextLine(*textSlot, *columnSlot, *lineSlot);
}
