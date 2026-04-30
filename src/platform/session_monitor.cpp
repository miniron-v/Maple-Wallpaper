#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <WtsApi32.h>

#include "platform/session_monitor.hpp"

namespace maple::platform {

struct SessionFlags {
    bool session_locked = false;
    bool monitor_off = false;
};

namespace {

const GUID k_display_state_guid =
    {0x6FE69556, 0x704A, 0x47A0,
     {0x8F, 0x24, 0xC2, 0x8D, 0x93, 0x6F, 0xDA, 0x47}};

LRESULT CALLBACK session_wnd_proc(HWND hwnd, UINT msg,
                                  WPARAM wparam, LPARAM lparam) {
    if (msg == WM_CREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lparam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                          reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        return 0;
    }

    auto* flags = reinterpret_cast<SessionFlags*>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    if (flags == nullptr) {
        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }

    if (msg == WM_WTSSESSION_CHANGE) {
        if (wparam == WTS_SESSION_LOCK) {
            flags->session_locked = true;
        } else if (wparam == WTS_SESSION_UNLOCK) {
            flags->session_locked = false;
        }
        return 0;
    }

    if (msg == WM_POWERBROADCAST && wparam == PBT_POWERSETTINGCHANGE) {
        auto* setting = reinterpret_cast<POWERBROADCAST_SETTING*>(lparam);
        if (IsEqualGUID(setting->PowerSetting, k_display_state_guid)
            && setting->DataLength == sizeof(DWORD)) {
            DWORD state = 0;
            memcpy(&state, setting->Data, sizeof(DWORD));
            flags->monitor_off = (state == 0);
        }
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

} // anonymous namespace

SessionMonitor::SessionMonitor() = default;

SessionMonitor::~SessionMonitor() {
    destroy();
}

bool SessionMonitor::create() {
    if (created_) return true;

    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = session_wnd_proc;
    wc.hInstance      = GetModuleHandleW(nullptr);
    wc.lpszClassName  = L"MapleWallpaperSessionMsg";

    if (RegisterClassExW(&wc) == 0
        && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }

    flags_ = std::make_unique<SessionFlags>();

    msg_hwnd_ = CreateWindowExW(
        0, wc.lpszClassName, L"", 0,
        0, 0, 0, 0,
        HWND_MESSAGE,
        nullptr,
        wc.hInstance,
        flags_.get()
    );

    if (msg_hwnd_ == nullptr) {
        flags_.reset();
        return false;
    }

    if (!WTSRegisterSessionNotification(msg_hwnd_, NOTIFY_FOR_THIS_SESSION)) {
        DestroyWindow(msg_hwnd_);
        msg_hwnd_ = nullptr;
        flags_.reset();
        return false;
    }

    power_notify_handle_ = RegisterPowerSettingNotification(
        msg_hwnd_, &k_display_state_guid, DEVICE_NOTIFY_WINDOW_HANDLE);

    created_ = true;
    return true;
}

void SessionMonitor::destroy() {
    if (!created_) return;

    if (power_notify_handle_ != nullptr) {
        UnregisterPowerSettingNotification(
            static_cast<HPOWERNOTIFY>(power_notify_handle_));
        power_notify_handle_ = nullptr;
    }

    WTSUnRegisterSessionNotification(msg_hwnd_);

    DestroyWindow(msg_hwnd_);
    msg_hwnd_ = nullptr;
    flags_.reset();
    created_  = false;
}

void SessionMonitor::poll() {
    if (!created_) return;

    MSG msg{};
    while (PeekMessageW(&msg, msg_hwnd_, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

bool SessionMonitor::is_idle() const noexcept {
    if (!flags_) return false;
    return flags_->session_locked || flags_->monitor_off;
}

void SessionMonitor::sleep_ms(unsigned int ms) {
    ::Sleep(static_cast<DWORD>(ms));
}

} // namespace maple::platform
