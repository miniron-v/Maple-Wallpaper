// raylib MUST be included before any Windows.h to avoid symbol clashes
// (Rectangle, CloseWindow, DrawText, ShowCursor all conflict).
#include <raylib.h>

#include "api/maplestory_io.hpp"
#include "cache/disk_cache.hpp"
#include "platform/tray.hpp"
#include "platform/wallpaper_host.hpp"
#include "platform/session_monitor.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")

constexpr int HENNESYS_MAP_ID = 100000000;
const std::string MAP_CACHE_KEY = "map_100000000.png";

struct ScreenInfo { int w; int h; };

static ScreenInfo init_fullscreen_window() {
    SetConfigFlags(FLAG_WINDOW_UNDECORATED);
    InitWindow(800, 600, "MapleWallpaper");
    SetTargetFPS(30);
    const int m = GetCurrentMonitor();
    ScreenInfo s{ GetMonitorWidth(m), GetMonitorHeight(m) };
    SetWindowSize(s.w, s.h);
    SetWindowPosition(0, 0);
    return s;
}

static bool try_embed(NativeWindowHandle hwnd, int w, int h) {
    auto workerw = maple::platform::find_workerw();
    if (!workerw.has_value()) return false;
    if (!maple::platform::embed_window(hwnd, workerw.value())) return false;
    SetWindowPosition(0, 0);
    SetWindowSize(w, h);
    return true;
}

// Height-fit scaling: preserves aspect ratio, centers horizontally
static Rectangle compute_source_rect(const Texture2D& tex, int sw, int sh) {
    float scale = static_cast<float>(tex.height) / sh;
    float src_w = sw * scale;
    float src_x = (tex.width - src_w) * 0.5f;
    if (src_x < 0) src_x = 0;
    if (src_w > tex.width) src_w = static_cast<float>(tex.width);
    return { src_x, 0, src_w, static_cast<float>(tex.height) };
}

static Texture2D load_background_texture() {
    Texture2D tex{};
    maple::cache::DiskCache cache;
    if (!cache.init()) return tex;

    std::optional<std::vector<uint8_t>> png;

    if (cache.exists(MAP_CACHE_KEY)) {
        png = cache.load(MAP_CACHE_KEY);
    } else {
        maple::api::MapleStoryIO api;
        png = api.fetch_map_render(HENNESYS_MAP_ID);
        if (png) cache.save(MAP_CACHE_KEY, png->data(), png->size());
    }

    if (!png || png->empty()) return tex;

    Image img = LoadImageFromMemory(".png", png->data(), static_cast<int>(png->size()));
    if (img.data == nullptr) return tex;

    tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

int main() {
    auto [screen_w, screen_h] = init_fullscreen_window();
    auto hwnd = static_cast<NativeWindowHandle>(GetWindowHandle());
    bool embedded = try_embed(hwnd, screen_w, screen_h);

    maple::platform::Tray tray;
    if (!tray.create()) {
        if (embedded) maple::platform::detach_window(hwnd);
        CloseWindow();
        return 1;
    }

    maple::platform::SessionMonitor session_monitor;
    session_monitor.create();

    Texture2D bg = load_background_texture();
    const Rectangle src  = (bg.id != 0) ? compute_source_rect(bg, screen_w, screen_h) : Rectangle{};
    const Rectangle dest = { 0, 0, static_cast<float>(screen_w), static_cast<float>(screen_h) };

    while (!WindowShouldClose() && !tray.should_quit()) {
        tray.poll();
        session_monitor.poll();

        if (session_monitor.is_idle()) {
            maple::platform::SessionMonitor::sleep_ms(500);
            continue;
        }

        BeginDrawing();
        if (bg.id != 0)
            DrawTexturePro(bg, src, dest, { 0, 0 }, 0, WHITE);
        else
            ClearBackground(BLACK);
        EndDrawing();
    }

    if (bg.id != 0) UnloadTexture(bg);
    session_monitor.destroy();
    tray.destroy();
    if (embedded) maple::platform::detach_window(hwnd);
    CloseWindow();
    return 0;
}
