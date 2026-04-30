#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "api/nexon_api.hpp"

namespace maple::api {

class MapleStoryIO {
public:
    std::optional<std::vector<uint8_t>> fetch_map_render(int map_id);

    std::optional<std::vector<uint8_t>> fetch_character_render(
        const CharacterAppearance& appearance,
        const std::string& pose = "stand1");
};

} // namespace maple::api
