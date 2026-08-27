#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace {

struct Rect {
    int left;
    int top;
    int width;
    int height;
};

constexpr int MapControlOffset = 0x68;
constexpr int ControlRectOffset = 0x04;
constexpr int SetExtentVtableIndex = 1;
constexpr int MapObjectFrameOffset = -0x180;
constexpr int MapXScaleFrameOffset = -0x14;
constexpr int MapYScaleFrameOffset = -0x20;
constexpr int MainInterfaceFrameOffset = -0xa8;
constexpr int MiniMapControlOffset = 0x5ab4;
constexpr int MiniMapScaledWidthOffset = 0x5d54;
constexpr int MiniMapScaledHeightOffset = 0x5d58;
constexpr int NativeMapWidth = 588;
constexpr int NativeMapHeight = 294;
constexpr int PixelTolerance = 1;
constexpr float ScaleTolerance = 0.001f;

void setControlRect(void* control, const Rect& rect) {
    auto** vtable = *reinterpret_cast<void***>(control);
    using SetRect = void(__thiscall*)(void*, const Rect*);
    reinterpret_cast<SetRect>(vtable[SetExtentVtableIndex])(control, &rect);
}

}

extern "C" void __cdecl preserveAreaMapAspect(void* framePointer) {
    if (framePointer == nullptr) {
        return;
    }

    __try {
        auto* frame = static_cast<unsigned char*>(framePointer);
        auto* map = *reinterpret_cast<unsigned char**>(frame + MapObjectFrameOffset);
        auto* xScale = reinterpret_cast<float*>(frame + MapXScaleFrameOffset);
        if (map == nullptr || xScale == nullptr) {
            return;
        }

        const Rect root =
            *reinterpret_cast<const Rect*>(map + ControlRectOffset);
        auto* mapControl = map + MapControlOffset;
        Rect mapRect =
            *reinterpret_cast<const Rect*>(mapControl + ControlRectOffset);
        if (mapRect.height <= 0 || mapRect.width <= 0) {
            return;
        }

        const int aspectWidth = mapRect.height * NativeMapWidth / NativeMapHeight;
        const float aspectScale =
            static_cast<float>(aspectWidth) / static_cast<float>(NativeMapWidth);
        if (*xScale <= aspectScale + ScaleTolerance) {
            return;
        }

        *xScale = aspectScale;
        if (mapRect.width > aspectWidth + PixelTolerance) {
            mapRect.left = root.left + (root.width - aspectWidth) / 2;
            mapRect.width = aspectWidth;
            setControlRect(mapControl, mapRect);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

extern "C" void __cdecl preserveAreaMapProjection(void* framePointer) {
    if (framePointer == nullptr) {
        return;
    }

    __try {
        auto* frame = static_cast<unsigned char*>(framePointer);
        auto* xScale = reinterpret_cast<float*>(frame + MapXScaleFrameOffset);
        const float yScale = *reinterpret_cast<float*>(frame + MapYScaleFrameOffset);
        if (yScale > 0.0f && *xScale > yScale) {
            *xScale = yScale;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

extern "C" void __cdecl preserveMiniMapAspect(void* framePointer) {
    if (framePointer == nullptr) {
        return;
    }

    __try {
        auto* frame = static_cast<unsigned char*>(framePointer);
        auto* mainInterface =
            *reinterpret_cast<unsigned char**>(frame + MainInterfaceFrameOffset);
        if (mainInterface == nullptr) {
            return;
        }

        const int height =
            *reinterpret_cast<const int*>(mainInterface + MiniMapScaledHeightOffset);
        if (height <= 0) {
            return;
        }

        const int aspectWidth = height * NativeMapWidth / NativeMapHeight;
        *reinterpret_cast<int*>(mainInterface + MiniMapScaledWidthOffset) = aspectWidth;
        reinterpret_cast<Rect*>(
            mainInterface + MiniMapControlOffset + ControlRectOffset)->width = aspectWidth;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
    UNREFERENCED_PARAMETER(instance);
    UNREFERENCED_PARAMETER(reason);
    UNREFERENCED_PARAMETER(reserved);
    return TRUE;
}
