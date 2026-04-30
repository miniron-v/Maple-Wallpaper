#pragma once

#include "platform/native_types.hpp"

namespace maple::platform {

class Tray {
public:
    Tray() = default;
    ~Tray();

    Tray(const Tray&) = delete;
    Tray& operator=(const Tray&) = delete;
    Tray(Tray&&) = delete;
    Tray& operator=(Tray&&) = delete;

    bool create();
    void destroy();
    void poll();
    [[nodiscard]] bool should_quit() const noexcept;

private:
    NativeWindowHandle msg_hwnd_ = nullptr;
    bool quit_requested_ = false;
    bool created_ = false;
};

} // namespace maple::platform
