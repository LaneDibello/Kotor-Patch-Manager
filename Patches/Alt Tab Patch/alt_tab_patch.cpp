#include <windows.h>

#pragma comment(lib, "user32.lib")

namespace {

constexpr LONG_PTR KotorWindowProcAddress = 0x00402800;
constexpr uintptr_t MainWindowAddress = 0x007A39D8;
constexpr uintptr_t ScreenWidthAddress = 0x0078D1D4;
constexpr uintptr_t ScreenHeightAddress = 0x0078D1D8;
constexpr uintptr_t InputManagerAddress = 0x007A39E4;
constexpr uintptr_t ShowCursorAddress = 0x005DF5F0;
constexpr uintptr_t RefreshCursorAddress = 0x005DF5B0;
constexpr UINT_PTR RestoreTimerId = 0x4B4F544F;
constexpr UINT RestoreTimerIntervalMs = 100;
constexpr DWORD RestoreStablePeriodMs = 3000;

bool restoreInProgress = false;
DWORD lastRestoreActivity = 0;
WNDPROC previousWindowProc = nullptr;

bool gameIsFullscreen(HWND window) {
    const LONG_PTR style = GetWindowLongPtrA(window, GWL_STYLE);
    return (style & WS_POPUP) != 0 && (style & WS_CAPTION) == 0;
}

void restoreCursor() {
    void* inputManager =
        *reinterpret_cast<void* const volatile*>(InputManagerAddress);
    if (inputManager == nullptr) {
        return;
    }

    using ShowCursorFunction = void(__thiscall*)(void*);
    using RefreshCursorFunction = void(__thiscall*)(void*, int);
    reinterpret_cast<ShowCursorFunction>(ShowCursorAddress)(inputManager);
    reinterpret_cast<RefreshCursorFunction>(RefreshCursorAddress)(
        inputManager, 0);
}

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

bool alignGameWindow(HWND window) {
    RECT clientRect = {};
    POINT clientOrigin = {};
    const int gameWidth =
        *reinterpret_cast<volatile int*>(ScreenWidthAddress);
    const int gameHeight =
        *reinterpret_cast<volatile int*>(ScreenHeightAddress);
    if (!GetClientRect(window, &clientRect) ||
        !ClientToScreen(window, &clientOrigin) ||
        gameWidth <= 0 || gameHeight <= 0) {
        return false;
    }

    const int clientWidth = clientRect.right - clientRect.left;
    const int clientHeight = clientRect.bottom - clientRect.top;
    if (clientOrigin.x == 0 && clientOrigin.y == 0 &&
        clientWidth == gameWidth && clientHeight == gameHeight) {
        return false;
    }

    RECT desiredWindowRect = {0, 0, gameWidth, gameHeight};
    const DWORD style = static_cast<DWORD>(
        GetWindowLongPtrA(window, GWL_STYLE));
    const DWORD extendedStyle = static_cast<DWORD>(
        GetWindowLongPtrA(window, GWL_EXSTYLE));
    if (!AdjustWindowRectEx(
            &desiredWindowRect, style, FALSE, extendedStyle)) {
        return false;
    }

    return SetWindowPos(
        window,
        nullptr,
        desiredWindowRect.left,
        desiredWindowRect.top,
        desiredWindowRect.right - desiredWindowRect.left,
        desiredWindowRect.bottom - desiredWindowRect.top,
        SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED) != FALSE;
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

LRESULT CALLBACK activationFilter(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam) {
    WNDPROC nextWindowProc = previousWindowProc != nullptr
        ? previousWindowProc
        : reinterpret_cast<WNDPROC>(KotorWindowProcAddress);

    if (!gameIsFullscreen(window)) {
        if (restoreInProgress) {
            restoreInProgress = false;
            KillTimer(window, RestoreTimerId);
        }
        return CallWindowProcA(nextWindowProc, window, message, wParam, lParam);
    }

    if (message == WM_DISPLAYCHANGE && restoreInProgress) {
        lastRestoreActivity = GetTickCount();
    }

    if (message == WM_ACTIVATEAPP && wParam != FALSE) {
        restoreInProgress = true;
        lastRestoreActivity = GetTickCount();
        const bool timerStarted = SetTimer(
            window, RestoreTimerId, RestoreTimerIntervalMs, nullptr) != 0;

        if (!displayMatchesGameResolution() &&
            !restoreExclusiveDisplayMode()) {
            restoreInProgress = false;
            KillTimer(window, RestoreTimerId);
        }
        const LRESULT result = CallWindowProcA(
            nextWindowProc, window, message, wParam, lParam);
        HWND mainWindow = *reinterpret_cast<HWND*>(MainWindowAddress);
        if (mainWindow == nullptr) {
            mainWindow = window;
        }
        if (restoreInProgress && IsIconic(mainWindow)) {
            ShowWindow(mainWindow, SW_RESTORE);
            lastRestoreActivity = GetTickCount();
        }
        if (restoreInProgress && alignGameWindow(mainWindow)) {
            lastRestoreActivity = GetTickCount();
        }
        if (!timerStarted) {
            restoreInProgress = false;
        }
        return result;
    }

    if (message == WM_ACTIVATEAPP && wParam == FALSE && restoreInProgress) {
        restoreInProgress = false;
        KillTimer(window, RestoreTimerId);
        return CallWindowProcA(nextWindowProc, window, message, wParam, lParam);
    }

    if (message == WM_TIMER && wParam == RestoreTimerId) {
        if (!restoreInProgress) {
            KillTimer(window, RestoreTimerId);
            return 0;
        }

        const DWORD now = GetTickCount();
        if (!kotorOwnsForeground()) {
            restoreInProgress = false;
            KillTimer(window, RestoreTimerId);
            return 0;
        }

        if (!displayMatchesGameResolution()) {
            if (!restoreExclusiveDisplayMode()) {
                restoreInProgress = false;
                KillTimer(window, RestoreTimerId);
                return 0;
            }
            lastRestoreActivity = now;
            return 0;
        }

        HWND mainWindow = *reinterpret_cast<HWND*>(MainWindowAddress);
        if (mainWindow != nullptr && IsIconic(mainWindow)) {
            ShowWindow(mainWindow, SW_RESTORE);
            lastRestoreActivity = now;
            return 0;
        }

        if (mainWindow != nullptr && alignGameWindow(mainWindow)) {
            lastRestoreActivity = now;
            return 0;
        }

        if (now - lastRestoreActivity >= RestoreStablePeriodMs) {
            restoreInProgress = false;
            KillTimer(window, RestoreTimerId);
            restoreCursor();
        }
        return 0;
    }

    return CallWindowProcA(nextWindowProc, window, message, wParam, lParam);
}

}

extern "C" void __cdecl installActivationFilter() {
    HWND mainWindow = *reinterpret_cast<HWND*>(MainWindowAddress);
    const LONG_PTR filterWindowProc =
        reinterpret_cast<LONG_PTR>(&activationFilter);
    if (mainWindow != nullptr &&
        GetWindowLongPtrA(mainWindow, GWLP_WNDPROC) != filterWindowProc) {
        previousWindowProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrA(
            mainWindow,
            GWLP_WNDPROC,
            filterWindowProc));
    }
}
