#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <shellapi.h>

#include "platform/process.hpp"

#include <array>
#include <filesystem>

namespace maple::platform {

static std::filesystem::path get_exe_directory() {
    std::array<wchar_t, MAX_PATH> buf{};
    DWORD len = GetModuleFileNameW(nullptr, buf.data(),
                                   static_cast<DWORD>(buf.size()));
    if (len == 0 || len >= buf.size()) return {};
    return std::filesystem::path(buf.data()).parent_path();
}

static bool launch_exe(const wchar_t* exe_name) {
    auto dir = get_exe_directory();
    if (dir.empty()) return false;

    auto exe_path = dir / exe_name;
    if (!std::filesystem::exists(exe_path)) return false;

    auto result = ShellExecuteW(
        nullptr, L"open",
        exe_path.wstring().c_str(),
        nullptr,
        dir.wstring().c_str(),
        SW_SHOWNORMAL);

    // ShellExecuteW returns > 32 on success
    return reinterpret_cast<intptr_t>(result) > 32;
}

bool launch_config_app() {
    return launch_exe(L"MapleWallpaperConfig.exe");
}

bool launch_wallpaper_app() {
    return launch_exe(L"MapleWallpaper.exe");
}

} // namespace maple::platform
