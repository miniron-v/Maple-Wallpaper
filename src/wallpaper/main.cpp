// raylib MUST be included before any Windows.h to avoid symbol clashes
// (Rectangle, CloseWindow, DrawText, ShowCursor all conflict).
#include <raylib.h>

#include "platform/tray.hpp"
#include "platform/wallpaper_host.hpp"
#include "platform/session_monitor.hpp"

// Ensure WIN32 subsystem works with a standard main() entry point
#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")

int main() {
    // -- Raylib window setup (undecorated) --
    SetConfigFlags(FLAG_WINDOW_UNDECORATED);
    InitWindow(800, 600, "MapleWallpaper");
    SetTargetFPS(30);

    // -- Get primary monitor size and resize to fullscreen --
    const int monitor = GetCurrentMonitor();
    const int screen_w = GetMonitorWidth(monitor);
    const int screen_h = GetMonitorHeight(monitor);
    SetWindowSize(screen_w, screen_h);
    SetWindowPosition(0, 0);

    // -- WorkerW embedding --
    auto raylib_hwnd = static_cast<NativeWindowHandle>(GetWindowHandle());
    auto workerw = maple::platform::find_workerw();

    bool embedded = false;
    if (workerw.has_value()) {
        embedded = maple::platform::embed_window(raylib_hwnd, workerw.value());

        if (embedded) {
            // After re-parenting, reposition within the WorkerW
            SetWindowPosition(0, 0);
            SetWindowSize(screen_w, screen_h);
        }
    }

    // -- System tray --
    maple::platform::Tray tray;
    if (!tray.create()) {
        if (embedded) maple::platform::detach_window(raylib_hwnd);
        CloseWindow();
        return 1;
    }

    // -- Session monitor (optional — failure is non-fatal) --
    maple::platform::SessionMonitor session_monitor;
    session_monitor.create();

    // -- Main loop --
    while (!WindowShouldClose() && !tray.should_quit()) {
        tray.poll();
        session_monitor.poll();

        if (session_monitor.is_idle()) {
            // Screen not visible: skip rendering, yield CPU
            maple::platform::SessionMonitor::sleep_ms(500);
            continue;
        }

        BeginDrawing();
        ClearBackground(BLACK);
        EndDrawing();
    }

    session_monitor.destroy();

    // -- Cleanup --
    tray.destroy();

    if (embedded) {
        maple::platform::detach_window(raylib_hwnd);
    }

    CloseWindow();
    return 0;
}
