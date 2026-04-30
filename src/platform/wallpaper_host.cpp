#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include "platform/wallpaper_host.hpp"

namespace maple::platform {

// Callback state for EnumWindows -- file-local to avoid polluting the namespace.
namespace {

struct WorkerWSearchResult {
    HWND workerw = nullptr;
};

BOOL CALLBACK enum_windows_proc(HWND hwnd, LPARAM lparam) {
    auto* result = reinterpret_cast<WorkerWSearchResult*>(lparam);

    // We are looking for a WorkerW that has SHELLDLL_DefView as a child.
    HWND shell_view = FindWindowExW(hwnd, nullptr, L"SHELLDLL_DefView", nullptr);
    if (shell_view != nullptr) {
        // The *next* WorkerW sibling (the one without SHELLDLL_DefView) is our target.
        HWND target = FindWindowExW(nullptr, hwnd, L"WorkerW", nullptr);
        if (target != nullptr) {
            result->workerw = target;
        }
        return FALSE; // stop enumeration
    }

    return TRUE; // keep searching
}

} // anonymous namespace

std::optional<NativeWindowHandle> find_workerw() {
    // Step 1: Find Progman
    HWND progman = FindWindowW(L"Progman", nullptr);
    if (progman == nullptr) {
        return std::nullopt;
    }

    // Step 2: Send the undocumented 0x052C message to spawn WorkerW windows
    DWORD_PTR msg_result = 0;
    SendMessageTimeoutW(
        progman,
        0x052C,       // undocumented message that creates WorkerW
        0xD,          // wParam
        0x1,          // lParam
        SMTO_NORMAL,
        1000,         // timeout ms
        &msg_result
    );

    // Step 3: Enumerate top-level windows to find the right WorkerW
    WorkerWSearchResult search{};
    EnumWindows(enum_windows_proc, reinterpret_cast<LPARAM>(&search));

    if (search.workerw == nullptr) {
        return std::nullopt;
    }

    return search.workerw;
}

bool embed_window(NativeWindowHandle child, NativeWindowHandle workerw) {
    if (child == nullptr || workerw == nullptr) {
        return false;
    }

    // Remove window decorations (WS_CAPTION, WS_THICKFRAME, etc.) just in case
    LONG style = GetWindowLong(child, GWL_STYLE);
    style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX |
                WS_MAXIMIZEBOX | WS_SYSMENU);
    SetWindowLong(child, GWL_STYLE, style);

    // Re-parent the raylib window into the WorkerW
    SetParent(child, workerw);

    return true;
}

void detach_window(NativeWindowHandle child) {
    if (child == nullptr) {
        return;
    }

    // Restore as a top-level window (parent = desktop)
    SetParent(child, nullptr);
}

} // namespace maple::platform
