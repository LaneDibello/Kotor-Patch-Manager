#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace {

// The four stdcall arguments the game pushed for PostMessageA, in stack order.
struct PostMessageArgs {
    HWND hwnd;
    UINT msg;
    WPARAM wParam;
    LPARAM lParam;
};

bool foregroundWindowIsOurs() {
    HWND foreground = GetForegroundWindow();
    if (foreground == nullptr) {
        return false;
    }
    DWORD processId = 0;
    GetWindowThreadProcessId(foreground, &processId);
    return processId == GetCurrentProcessId();
}

}

extern "C" void __cdecl dropSyntheticDeactivate(PostMessageArgs* args) {
    if (args == nullptr) {
        return;
    }
    if (args->msg != WM_ACTIVATEAPP || args->wParam != 0 || args->lParam != 0) {
        return;
    }
    if (!foregroundWindowIsOurs()) {
        return;
    }
    // Turn the post into PostThreadMessage(WM_NULL), which nothing acts on.
    args->hwnd = nullptr;
    args->msg = WM_NULL;
}
