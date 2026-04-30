#include "cache/disk_cache.hpp"

#include <cstdlib>
#include <fstream>

namespace maple::cache {

bool DiskCache::init() {
    const char* local_app_data = std::getenv("LOCALAPPDATA");
    if (!local_app_data) {
        return false;
    }

    cache_dir_ = std::filesystem::path(local_app_data) / "MapleWallpaper" / "cache";

    std::error_code ec;
    std::filesystem::create_directories(cache_dir_, ec);
    if (ec) {
        return false;
    }

    return true;
}

std::optional<std::vector<uint8_t>> DiskCache::load(const std::string& key) const {
    auto path = cache_dir_ / key;

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return std::nullopt;
    }

    auto file_size = file.tellg();
    if (file_size <= 0) {
        return std::nullopt;
    }

    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(static_cast<size_t>(file_size));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), file_size)) {
        return std::nullopt;
    }

    return buffer;
}

bool DiskCache::save(const std::string& key, const uint8_t* data, size_t size) {
    auto path = cache_dir_ / key;

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        return false;
    }

    file.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
    return file.good();
}

bool DiskCache::exists(const std::string& key) const {
    std::error_code ec;
    return std::filesystem::exists(cache_dir_ / key, ec);
}

bool DiskCache::remove(const std::string& key) {
    std::error_code ec;
    return std::filesystem::remove(cache_dir_ / key, ec);
}

void DiskCache::clear() {
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(cache_dir_, ec)) {
        std::filesystem::remove(entry.path(), ec);
    }
}

} // namespace maple::cache
