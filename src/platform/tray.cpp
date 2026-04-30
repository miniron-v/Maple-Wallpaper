#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <shellapi.h>

#include "platform/tray.hpp"

namespace maple::platform {

namespace {

constexpr UINT WM_TRAY_CALLBACK = WM_APP + 1;
constexpr UINT MENU_ID_QUIT     = 1001;
constexpr UINT TRAY_ICON_ID     = 1;

// quit flag pointer stored in GWLP_USERDATA of the message-only window
void set_quit_flag(HWND hwnd) {
    auto* flag = reinterpret_cast<bool*>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (flag != nullptr) {
        *flag = true;
    }
}

void show_context_menu(HWND hwnd) {
    POINT pt{};
    GetCursorPos(&pt);

    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, MENU_ID_QUIT, L"\xC885\xB8CC"); // "종료"

    // Required so the menu dismisses when clicking elsewhere
    SetForegroundWindow(hwnd);
    TrackPopupMenu(menu, TPM_BOTTOMALIGN | TPM_LEFTALIGN,
                   pt.x, pt.y, 0, hwnd, nullptr);
    PostMessageW(hwnd, WM_NULL, 0, 0);

    DestroyMenu(menu);
}

LRESULT CALLBACK tray_wnd_proc(HWND hwnd, UINT msg,
                               WPARAM wparam, LPARAM lparam) {
    if (msg == WM_CREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lparam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                          reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        return 0;
    }

    if (msg == WM_TRAY_CALLBACK && LOWORD(lparam) == WM_RBUTTONUP) {
        show_context_menu(hwnd);
        return 0;
    }

    if (msg == WM_COMMAND && LOWORD(wparam) == MENU_ID_QUIT) {
        set_quit_flag(hwnd);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

} // anonymous namespace

// ── Tray implementation ─────────────────────────────────────────

Tray::~Tray() {
    destroy();
}

bool Tray::create() {
    if (created_) return true;

    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = tray_wnd_proc;
    wc.hInstance      = GetModuleHandleW(nullptr);
    wc.lpszClassName  = L"MapleWallpaperTrayMsg";

    if (RegisterClassExW(&wc) == 0
        && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }

    // Pass &quit_requested_ as lpCreateParams → stored via GWLP_USERDATA
    msg_hwnd_ = CreateWindowExW(
        0, wc.lpszClassName, L"", 0,
        0, 0, 0, 0,
        HWND_MESSAGE,
        nullptr,
        wc.hInstance,
        &quit_requested_
    );

    if (msg_hwnd_ == nullptr) return false;

    NOTIFYICONDATAW nid{};
    nid.cbSize           = sizeof(nid);
    nid.hWnd             = msg_hwnd_;
    nid.uID              = TRAY_ICON_ID;
    nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAY_CALLBACK;
    nid.hIcon            = LoadIconW(nullptr, MAKEINTRESOURCEW(32512));
    wcscpy_s(nid.szTip, L"MapleWallpaper");

    if (!Shell_NotifyIconW(NIM_ADD, &nid)) {
        DestroyWindow(msg_hwnd_);
        msg_hwnd_ = nullptr;
        return false;
    }

    created_ = true;
    return true;
}

void Tray::destroy() {
    if (!created_) return;

    NOTIFYICONDATAW nid{};
    nid.cbSize = sizeof(nid);
    nid.hWnd   = msg_hwnd_;
    nid.uID    = TRAY_ICON_ID;
    Shell_NotifyIconW(NIM_DELETE, &nid);

    DestroyWindow(msg_hwnd_);
    msg_hwnd_ = nullptr;
    created_  = false;
}

void Tray::poll() {
    MSG msg{};
    while (PeekMessageW(&msg, msg_hwnd_, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

bool Tray::should_quit() const noexcept {
    return quit_requested_;
}

} // namespace maple::platform
