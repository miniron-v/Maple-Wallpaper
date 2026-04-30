#include "core/config.hpp"

#include <cstdlib>
#include <fstream>
#include <nlohmann/json.hpp>

namespace maple::core {

bool ConfigManager::load() {
    const char* local_app_data = std::getenv("LOCALAPPDATA");
    if (!local_app_data) {
        return false;
    }

    config_path_ = std::filesystem::path(local_app_data) / "MapleWallpaper" / "config.json";

    std::ifstream file(config_path_);
    if (!file.is_open()) {
        return false;
    }

    nlohmann::json j;
    try {
        file >> j;
    } catch (const nlohmann::json::parse_error&) {
        return false;
    }

    if (j.contains("api_key") && j["api_key"].is_string()) {
        config_.api_key = j["api_key"].get<std::string>();
    }
    if (j.contains("character_name") && j["character_name"].is_string()) {
        config_.character_name = j["character_name"].get<std::string>();
    }
    if (j.contains("auto_refresh") && j["auto_refresh"].is_boolean()) {
        config_.auto_refresh = j["auto_refresh"].get<bool>();
    }
    if (j.contains("refresh_interval_min") && j["refresh_interval_min"].is_number_integer()) {
        config_.refresh_interval_min = j["refresh_interval_min"].get<int>();
    }
    if (j.contains("auto_start") && j["auto_start"].is_boolean()) {
        config_.auto_start = j["auto_start"].get<bool>();
    }

    return true;
}

bool ConfigManager::save() const {
    auto path = config_path_;
    if (path.empty()) {
        const char* local_app_data = std::getenv("LOCALAPPDATA");
        if (!local_app_data) {
            return false;
        }
        path = std::filesystem::path(local_app_data) / "MapleWallpaper" / "config.json";
    }

    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    if (ec) {
        return false;
    }

    nlohmann::json j;
    j["api_key"] = config_.api_key;
    j["character_name"] = config_.character_name;
    j["auto_refresh"] = config_.auto_refresh;
    j["refresh_interval_min"] = config_.refresh_interval_min;
    j["auto_start"] = config_.auto_start;

    std::ofstream file(path, std::ios::trunc);
    if (!file.is_open()) {
        return false;
    }

    file << j.dump(4);
    return file.good();
}

AppConfig& ConfigManager::config() {
    return config_;
}

const AppConfig& ConfigManager::config() const {
    return config_;
}

} // namespace maple::core
