#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <CommCtrl.h>

#include "core/config.hpp"
#include "platform/process.hpp"

#include <string>

#pragma comment(lib, "comctl32.lib")

// Control IDs
constexpr int ID_EDIT_API_KEY       = 101;
constexpr int ID_EDIT_CHAR_NAME     = 102;
constexpr int ID_CHECK_AUTO_REFRESH = 103;
constexpr int ID_EDIT_REFRESH_MIN   = 104;
constexpr int ID_CHECK_AUTO_START   = 105;
constexpr int ID_BTN_START          = 201;
constexpr int ID_BTN_SAVE           = 202;
constexpr int ID_BTN_CLOSE          = 203;

static HWND g_hwnd_api_key      = nullptr;
static HWND g_hwnd_char_name    = nullptr;
static HWND g_hwnd_auto_refresh = nullptr;
static HWND g_hwnd_refresh_min  = nullptr;
static HWND g_hwnd_auto_start   = nullptr;

static maple::core::ConfigManager g_cfg;

static std::wstring utf8_to_wide(const std::string& s) {
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (len <= 0) return {};
    std::wstring ws(len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, ws.data(), len);
    return ws;
}

static std::string wide_to_utf8(const std::wstring& ws) {
    if (ws.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1,
                                  nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};
    std::string s(len - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1,
                        s.data(), len, nullptr, nullptr);
    return s;
}

static std::wstring get_window_text(HWND hwnd) {
    int len = GetWindowTextLengthW(hwnd);
    if (len <= 0) return {};
    std::wstring buf(len + 1, L'\0');
    GetWindowTextW(hwnd, buf.data(), len + 1);
    buf.resize(len);
    return buf;
}

static void populate_ui_from_config(const maple::core::AppConfig& cfg) {
    SetWindowTextW(g_hwnd_api_key, utf8_to_wide(cfg.api_key).c_str());
    SetWindowTextW(g_hwnd_char_name, utf8_to_wide(cfg.character_name).c_str());

    SendMessageW(g_hwnd_auto_refresh, BM_SETCHECK,
                 cfg.auto_refresh ? BST_CHECKED : BST_UNCHECKED, 0);

    SetWindowTextW(g_hwnd_refresh_min,
                   std::to_wstring(cfg.refresh_interval_min).c_str());

    SendMessageW(g_hwnd_auto_start, BM_SETCHECK,
                 cfg.auto_start ? BST_CHECKED : BST_UNCHECKED, 0);
}

static void collect_ui_to_config(maple::core::AppConfig& cfg) {
    cfg.api_key = wide_to_utf8(get_window_text(g_hwnd_api_key));
    cfg.character_name = wide_to_utf8(get_window_text(g_hwnd_char_name));

    cfg.auto_refresh =
        (SendMessageW(g_hwnd_auto_refresh, BM_GETCHECK, 0, 0) == BST_CHECKED);

    auto min_str = get_window_text(g_hwnd_refresh_min);
    try { cfg.refresh_interval_min = std::stoi(wide_to_utf8(min_str)); }
    catch (...) { cfg.refresh_interval_min = 60; }

    cfg.auto_start =
        (SendMessageW(g_hwnd_auto_start, BM_GETCHECK, 0, 0) == BST_CHECKED);
}

static void on_start(HWND hwnd) {
    collect_ui_to_config(g_cfg.config());
    g_cfg.save();
    maple::platform::launch_wallpaper_app();
    DestroyWindow(hwnd);
}

static void on_save() {
    collect_ui_to_config(g_cfg.config());
    g_cfg.save();
}

static HWND create_label(HWND parent, const wchar_t* text,
                          int x, int y, int w, int h) {
    return CreateWindowExW(
        0, L"STATIC", text, WS_CHILD | WS_VISIBLE | SS_LEFT,
        x, y, w, h, parent, nullptr, nullptr, nullptr);
}

static HWND create_edit(HWND parent, int id,
                         int x, int y, int w, int h) {
    return CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        x, y, w, h, parent, reinterpret_cast<HMENU>(static_cast<intptr_t>(id)),
        nullptr, nullptr);
}

static HWND create_checkbox(HWND parent, int id, const wchar_t* text,
                              int x, int y, int w, int h) {
    return CreateWindowExW(
        0, L"BUTTON", text, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        x, y, w, h, parent,
        reinterpret_cast<HMENU>(static_cast<intptr_t>(id)),
        nullptr, nullptr);
}

static HWND create_button(HWND parent, int id, const wchar_t* text,
                            int x, int y, int w, int h) {
    return CreateWindowExW(
        0, L"BUTTON", text, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        x, y, w, h, parent,
        reinterpret_cast<HMENU>(static_cast<intptr_t>(id)),
        nullptr, nullptr);
}

static void create_controls(HWND hwnd) {
    constexpr int LX = 20;   // label x
    constexpr int EX = 20;   // edit x
    constexpr int EW = 370;  // edit width
    constexpr int EH = 24;   // edit height

    int y = 15;
    create_label(hwnd, L"API \xD0A4:", LX, y, 100, 20);
    y += 22;
    g_hwnd_api_key = create_edit(hwnd, ID_EDIT_API_KEY, EX, y, EW, EH);

    y += 38;
    create_label(hwnd, L"\xCE90\xB9AD\xD130\xBA85:", LX, y, 100, 20);
    y += 22;
    g_hwnd_char_name = create_edit(hwnd, ID_EDIT_CHAR_NAME, EX, y, EW, EH);

    y += 40;
    g_hwnd_auto_refresh = create_checkbox(
        hwnd, ID_CHECK_AUTO_REFRESH,
        L"\xC790\xB3D9 \xAC31\xC2E0 (\xBD84 \xB2E8\xC704):", LX, y, 190, 22);
    g_hwnd_refresh_min = create_edit(
        hwnd, ID_EDIT_REFRESH_MIN, LX + 195, y, 60, EH);

    y += 32;
    g_hwnd_auto_start = create_checkbox(
        hwnd, ID_CHECK_AUTO_START,
        L"Windows \xC2DC\xC791 \xC2DC \xC790\xB3D9 \xC2E4\xD589", LX, y, 250, 22);

    y += 45;
    constexpr int BW = 90;
    constexpr int BH = 32;
    int bx = 55;
    create_button(hwnd, ID_BTN_START, L"\xC2DC\xC791", bx, y, BW, BH);
    bx += BW + 30;
    create_button(hwnd, ID_BTN_SAVE, L"\xC800\xC7A5", bx, y, BW, BH);
    bx += BW + 30;
    create_button(hwnd, ID_BTN_CLOSE, L"\xB2EB\xAE30", bx, y, BW, BH);
}

static void set_default_font(HWND hwnd) {
    HFONT font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    EnumChildWindows(hwnd, [](HWND child, LPARAM lp) -> BOOL {
        SendMessageW(child, WM_SETFONT, static_cast<WPARAM>(lp), TRUE);
        return TRUE;
    }, reinterpret_cast<LPARAM>(font));
}

static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg,
                                  WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE:
        create_controls(hwnd);
        set_default_font(hwnd);
        g_cfg.load();
        populate_ui_from_config(g_cfg.config());
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case ID_BTN_START: on_start(hwnd); return 0;
        case ID_BTN_SAVE:  on_save();      return 0;
        case ID_BTN_CLOSE: DestroyWindow(hwnd); return 0;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int nShow) {
    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = wnd_proc;
    wc.hInstance      = hInst;
    wc.hCursor       = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = L"MapleWallpaperConfig";

    if (!RegisterClassExW(&wc)) return 1;

    constexpr int W = 420;
    constexpr int H = 320;

    RECT rc{ 0, 0, W, H };
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRect(&rc, style, FALSE);

    HWND hwnd = CreateWindowExW(
        0, wc.lpszClassName, L"MapleWallpaper \xC124\xC815",
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        nullptr, nullptr, hInst, nullptr);

    if (!hwnd) return 1;

    ShowWindow(hwnd, nShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
}
