#pragma once

#include <memory>

#include "platform/native_types.hpp"

namespace maple::platform {

struct SessionFlags;

class SessionMonitor {
public:
    SessionMonitor();
    ~SessionMonitor();

    SessionMonitor(const SessionMonitor&) = delete;
    SessionMonitor& operator=(const SessionMonitor&) = delete;
    SessionMonitor(SessionMonitor&&) = delete;
    SessionMonitor& operator=(SessionMonitor&&) = delete;

    bool create();
    void destroy();
    void poll();

    [[nodiscard]] bool is_idle() const noexcept;

    // Wraps Win32 Sleep() so callers avoid Windows.h
    static void sleep_ms(unsigned int ms);

private:
    NativeWindowHandle msg_hwnd_ = nullptr;
    std::unique_ptr<SessionFlags> flags_;
    bool created_ = false;
    void* power_notify_handle_ = nullptr;
};

} // namespace maple::platform
