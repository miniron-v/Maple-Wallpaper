#pragma once

#include <filesystem>
#include <string>

namespace maple::core {

struct AppConfig {
    std::string api_key;
    std::string character_name;
    bool auto_refresh = false;
    int refresh_interval_min = 60;
    bool auto_start = false;
};

class ConfigManager {
public:
    bool load();
    bool save() const;
    AppConfig& config();
    const AppConfig& config() const;

private:
    AppConfig config_;
    std::filesystem::path config_path_;
};

} // namespace maple::core
