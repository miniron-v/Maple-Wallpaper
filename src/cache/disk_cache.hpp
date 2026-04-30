#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace maple::cache {

class DiskCache {
public:
    bool init();
    std::optional<std::vector<uint8_t>> load(const std::string& key) const;
    bool save(const std::string& key, const uint8_t* data, size_t size);
    bool exists(const std::string& key) const;
    bool remove(const std::string& key);
    void clear();

private:
    std::filesystem::path cache_dir_;
};

} // namespace maple::cache
