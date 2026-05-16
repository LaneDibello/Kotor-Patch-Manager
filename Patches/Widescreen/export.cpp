#include "Common.h"
#include "GameAPI/CExoString.h"
#include "GameAPI/GameVersion.h"

extern "C" void* __cdecl getResolutionString(DWORD width, DWORD height) {
    char buffer[16];
    sprintf_s(buffer, sizeof(buffer), "%lux%lu",
        static_cast<unsigned long>(width),
        static_cast<unsigned long>(height));
    CExoString str(buffer);
    return str.GetPtr();
}

namespace {
    struct Rect {
        int left;
        int top;
        int width;
        int height;
    };

    using SetExtentFn = void(__thiscall*)(void* self, Rect* rect);

    volatile int* const ScreenWidth = reinterpret_cast<volatile int*>(0x0078D1D4);
    volatile int* const ScreenHeight = reinterpret_cast<volatile int*>(0x0078D1D8);
    constexpr int BaseWidth = 800;
    constexpr int BaseHeight = 600;
    int g_guiBaseWidth = BaseWidth;
    int g_guiBaseHeight = BaseHeight;
    constexpr int BottomInset = 8;

    int scaleCoordinate(int value, int target, int source) {
        if (source <= 0 || target <= 0) {
            return value;
        }

        return static_cast<int>((static_cast<long long>(value) * target) / source);
    }

    bool readShort(void* address, short& value) {
        __try {
            value = *static_cast<short*>(address);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    bool readRect(void* control, Rect& rect) {
        if (!control) {
            return false;
        }

        __try {
            char* base = static_cast<char*>(control);
            rect.left = *reinterpret_cast<int*>(base + 0x04);
            rect.top = *reinterpret_cast<int*>(base + 0x08);
            rect.width = *reinterpret_cast<int*>(base + 0x0C);
            rect.height = *reinterpret_cast<int*>(base + 0x10);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    void setRect(void* control, Rect& rect) {
        __try {
            void** vtable = *static_cast<void***>(control);
            auto setExtent = reinterpret_cast<SetExtentFn>(vtable[1]);
            setExtent(control, &rect);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
        }
    }

}

extern "C" void __cdecl scaleGuiExtent(Rect* rect) {
    if (!rect) {
        return;
    }

    __try {
        if (rect->left == 0 && rect->top == 0 && rect->width > 0 && rect->height > 0) {
            if ((rect->width == 640 && rect->height == 480) ||
                (rect->width == 640 && rect->height <= 120) ||
                (rect->width == 800 && rect->height == 600) ||
                (rect->width >= 1000 && rect->height >= 700)) {
                g_guiBaseWidth = rect->width;
                g_guiBaseHeight = (rect->width == 640 && rect->height <= 120) ? 480 : rect->height;
            }
        }

        const int width = *ScreenWidth;
        const int height = *ScreenHeight;
        rect->left = scaleCoordinate(rect->left, width, g_guiBaseWidth);
        rect->width = scaleCoordinate(rect->width, width, g_guiBaseWidth);
        rect->top = scaleCoordinate(rect->top, height, g_guiBaseHeight);
        rect->height = scaleCoordinate(rect->height, height, g_guiBaseHeight);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

extern "C" void __cdecl fixDialogueLayoutRect(void* dialog, Rect* rect) {
    if (!dialog || !rect) {
        return;
    }

    __try {
        void* parent = *reinterpret_cast<void**>(static_cast<char*>(dialog) + 0x18);
        short parentHeightShort = 0;
        if (!readShort(static_cast<char*>(parent) + 0x6E, parentHeightShort)) {
            return;
        }

        const int parentHeight = static_cast<int>(parentHeightShort);
        if (parentHeight <= 0 || rect->height <= 0) {
            return;
        }

        const int maxTop = parentHeight - rect->height - BottomInset;
        if (rect->top > parentHeight / 2 && rect->top > maxTop) {
            rect->top = maxTop < 0 ? 0 : maxTop;
        }

        if (rect->top > parentHeight / 2) {
            char* base = static_cast<char*>(dialog);
            *reinterpret_cast<int*>(base + 0x19C8) = 0;
            *reinterpret_cast<int*>(base + 0x19CC) = 0;
            *reinterpret_cast<int*>(base + 0x19D4) = rect->height;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

extern "C" void __cdecl fixDialoguePanelOverflow(void* dialog) {
    if (!dialog) {
        return;
    }

    __try {
        void* parent = *reinterpret_cast<void**>(static_cast<char*>(dialog) + 0x18);
        short parentHeightShort = 0;
        if (!readShort(static_cast<char*>(parent) + 0x6E, parentHeightShort)) {
            return;
        }

        const int parentHeight = static_cast<int>(parentHeightShort);
        if (parentHeight <= 0) {
            return;
        }

        Rect dialogRect = {};
        if (readRect(dialog, dialogRect) && dialogRect.height > 0) {
            bool changed = false;

            int maxTop = parentHeight - dialogRect.height - BottomInset;
            if (maxTop < 0) {
                maxTop = 0;
            }

            if (dialogRect.top > maxTop) {
                dialogRect.top = maxTop;
                changed = true;
            }

            if (changed) {
                setRect(dialog, dialogRect);
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

// DLL Entry Point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        if (!GameVersion::Initialize()) {
            return FALSE;
        }
        break;

    case DLL_PROCESS_DETACH:
        GameVersion::Reset();
        break;
    }
    return TRUE;
}
