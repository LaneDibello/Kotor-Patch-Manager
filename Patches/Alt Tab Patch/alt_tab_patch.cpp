#include <windows.h>

#pragma comment(lib, "user32.lib")

namespace {

constexpr LONG_PTR KotorWindowProcAddress = 0x00402800;
constexpr uintptr_t MainWindowAddress = 0x007A39D8;
constexpr uintptr_t ScreenWidthAddress = 0x0078D1D4;
constexpr uintptr_t ScreenHeightAddress = 0x0078D1D8;
constexpr UINT_PTR RestoreTimerId = 0x4B4F544F;
constexpr UINT RestoreTimerIntervalMs = 100;
constexpr DWORD RestoreStablePeriodMs = 500;

bool restoreInProgress = false;
bool windowPositioned = false;
DWORD lastRestoreActivity = 0;

bool kotorOwnsForeground() {
    const HWND foregroundWindow = GetForegroundWindow();
    DWORD foregroundProcessId = 0;
    GetWindowThreadProcessId(foregroundWindow, &foregroundProcessId);
    return foregroundProcessId == GetCurrentProcessId();
}

bool displayMatchesGameResolution() {
    const int gameWidth = *reinterpret_cast<volatile int*>(ScreenWidthAddress);
    const int gameHeight = *reinterpret_cast<volatile int*>(ScreenHeightAddress);
    return GetSystemMetrics(SM_CXSCREEN) == gameWidth &&
           GetSystemMetrics(SM_CYSCREEN) == gameHeight;
}

bool restoreExclusiveDisplayMode() {
    DEVMODEA displayMode = {};
    displayMode.dmSize = sizeof(displayMode);
    if (!EnumDisplaySettingsA(
            nullptr, ENUM_CURRENT_SETTINGS, &displayMode)) {
        return false;
    }

    displayMode.dmPelsWidth =
        *reinterpret_cast<volatile int*>(ScreenWidthAddress);
    displayMode.dmPelsHeight =
        *reinterpret_cast<volatile int*>(ScreenHeightAddress);
    displayMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
    return ChangeDisplaySettingsA(&displayMode, CDS_FULLSCREEN) ==
           DISP_CHANGE_SUCCESSFUL;
}

bool positionRestoredWindow() {
    HWND mainWindow = *reinterpret_cast<HWND*>(MainWindowAddress);
    if (mainWindow == nullptr || IsIconic(mainWindow)) {
        return false;
    }

    const int gameWidth = *reinterpret_cast<volatile int*>(ScreenWidthAddress);
    const int gameHeight = *reinterpret_cast<volatile int*>(ScreenHeightAddress);
    return SetWindowPos(
               mainWindow,
               nullptr,
               0,
               0,
               gameWidth,
               gameHeight,
               SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED |
                   SWP_SHOWWINDOW) != FALSE;
}

LRESULT CALLBACK activationFilter(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam) {
    WNDPROC kotorWindowProc = reinterpret_cast<WNDPROC>(KotorWindowProcAddress);

    if (message == WM_DISPLAYCHANGE && restoreInProgress) {
        lastRestoreActivity = GetTickCount();
    }

    if (message == WM_ACTIVATEAPP && wParam != FALSE) {
        if (!restoreInProgress) {
            windowPositioned = false;
        }
        restoreInProgress = true;
        lastRestoreActivity = GetTickCount();
        SetTimer(window, RestoreTimerId, RestoreTimerIntervalMs, nullptr);

        const LRESULT result =
            CallWindowProcA(kotorWindowProc, window, message, wParam, lParam);
        if (!displayMatchesGameResolution()) {
            if (!restoreExclusiveDisplayMode()) {
                restoreInProgress = false;
                KillTimer(window, RestoreTimerId);
            }
        }
        return result;
    }

    if (message == WM_ACTIVATEAPP && wParam == FALSE && restoreInProgress) {
        lastRestoreActivity = GetTickCount();
        return 0;
    }

    if (message == WM_TIMER && wParam == RestoreTimerId) {
        if (!restoreInProgress) {
            KillTimer(window, RestoreTimerId);
            return 0;
        }

        const DWORD now = GetTickCount();
        if (!kotorOwnsForeground() || !displayMatchesGameResolution()) {
            return 0;
        }

        if (!windowPositioned) {
            if (positionRestoredWindow()) {
                windowPositioned = true;
                lastRestoreActivity = now;
            }
            return 0;
        }

        if (now - lastRestoreActivity >= RestoreStablePeriodMs) {
            restoreInProgress = false;
            KillTimer(window, RestoreTimerId);
        }
        return 0;
    }

    return CallWindowProcA(kotorWindowProc, window, message, wParam, lParam);
}

BOOL CALLBACK installForWindow(HWND window, LPARAM) {
    const LONG_PTR currentWindowProc = GetWindowLongPtrA(window, GWLP_WNDPROC);
    if (currentWindowProc == KotorWindowProcAddress) {
        SetWindowLongPtrA(
            window,
            GWLP_WNDPROC,
            reinterpret_cast<LONG_PTR>(&activationFilter));
    }
    return TRUE;
}

}

extern "C" void __cdecl installActivationFilter() {
    EnumThreadWindows(GetCurrentThreadId(), &installForWindow, 0);
}
