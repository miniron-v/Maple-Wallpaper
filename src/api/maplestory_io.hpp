#pragma once

#include <cstdint>
#include <optional>
#include <vector>

namespace maple::api {

class MapleStoryIO {
public:
    std::optional<std::vector<uint8_t>> fetch_map_render(int map_id);
};

} // namespace maple::api
