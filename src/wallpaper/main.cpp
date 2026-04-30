// raylib MUST be included before any Windows.h to avoid symbol clashes
// (Rectangle, CloseWindow, DrawText, ShowCursor all conflict).
#include <raylib.h>

#include "api/maplestory_io.hpp"
#include "api/nexon_api.hpp"
#include "cache/disk_cache.hpp"
#include "core/config.hpp"
#include "platform/process.hpp"
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

static std::optional<std::vector<uint8_t>> fetch_character_png(
    const std::string& api_key, const std::string& name)
{
    maple::api::NexonApi nexon(api_key);
    auto ocid = nexon.fetch_ocid(name);
    if (!ocid) return std::nullopt;

    auto appearance = nexon.fetch_appearance(*ocid);
    if (!appearance) return std::nullopt;

    maple::api::MapleStoryIO io;
    return io.fetch_character_render(*appearance, "stand1");
}

static Texture2D load_character_texture() {
    Texture2D tex{};

    maple::core::ConfigManager cfg_mgr;
    if (!cfg_mgr.load()) return tex;

    const auto& cfg = cfg_mgr.config();
    if (cfg.api_key.empty() || cfg.character_name.empty()) return tex;

    const std::string cache_key = "char_" + cfg.character_name + "_stand.png";
    maple::cache::DiskCache cache;
    if (!cache.init()) return tex;

    std::optional<std::vector<uint8_t>> png;

    if (cache.exists(cache_key)) {
        png = cache.load(cache_key);
    } else {
        png = fetch_character_png(cfg.api_key, cfg.character_name);
        if (png) cache.save(cache_key, png->data(), png->size());
    }

    if (!png || png->empty()) return tex;

    Image img = LoadImageFromMemory(".png", png->data(), static_cast<int>(png->size()));
    if (img.data == nullptr) return tex;

    tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

static void run_main_loop(
    int screen_w, int screen_h,
    const Texture2D& bg, const Texture2D& char_tex,
    maple::platform::Tray& tray,
    maple::platform::SessionMonitor& session_monitor)
{
    const Rectangle src  = (bg.id != 0)
        ? compute_source_rect(bg, screen_w, screen_h) : Rectangle{};
    const Rectangle dest = {
        0, 0, static_cast<float>(screen_w), static_cast<float>(screen_h) };

    const int char_x = (char_tex.id != 0)
        ? screen_w / 2 - char_tex.width / 2 : 0;
    const int char_y = (char_tex.id != 0)
        ? static_cast<int>(screen_h * 0.78f) - char_tex.height : 0;

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

        if (char_tex.id != 0)
            DrawTexture(char_tex, char_x, char_y, WHITE);
        EndDrawing();
    }
}

int main() {
    // First run: if no config exists, launch the config app and exit
    maple::core::ConfigManager cfg_check;
    if (!cfg_check.load() || cfg_check.config().api_key.empty()) {
        maple::platform::launch_config_app();
        return 0;
    }

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

    Texture2D bg       = load_background_texture();
    Texture2D char_tex = load_character_texture();

    run_main_loop(screen_w, screen_h, bg, char_tex, tray, session_monitor);

    if (char_tex.id != 0) UnloadTexture(char_tex);
    if (bg.id != 0) UnloadTexture(bg);
    session_monitor.destroy();
    tray.destroy();
    if (embedded) maple::platform::detach_window(hwnd);
    CloseWindow();
    return 0;
}
