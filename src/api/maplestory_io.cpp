#include "api/maplestory_io.hpp"

#include <cpr/cpr.h>
#include <string>

namespace maple::api {

namespace {

// Builds the maplestory.io item string from appearance data.
// Format: {"itemId":id1},{"itemId":id2},...
std::string build_item_string(const CharacterAppearance& appearance) {
    std::string result;

    auto append_id = [&](int id) {
        if (id == 0) return;
        if (!result.empty()) {
            result += ",";
        }
        result += "{\"itemId\":" + std::to_string(id) + "}";
    };

    append_id(appearance.skin_id);
    append_id(appearance.face_id);
    append_id(appearance.hair_id);

    for (int id : appearance.equip_ids) {
        append_id(id);
    }

    return result;
}

} // namespace

std::optional<std::vector<uint8_t>> MapleStoryIO::fetch_map_render(int map_id) {
    auto url = "https://maplestory.io/api/GMS/latest/map/"
               + std::to_string(map_id) + "/render";

    auto response = cpr::Get(
        cpr::Url{url},
        cpr::Timeout{30000}
    );

    if (response.status_code != 200) {
        return std::nullopt;
    }

    if (response.text.empty()) {
        return std::nullopt;
    }

    auto* begin = reinterpret_cast<const uint8_t*>(response.text.data());
    auto* end = begin + response.text.size();

    return std::vector<uint8_t>(begin, end);
}

std::optional<std::vector<uint8_t>> MapleStoryIO::fetch_character_render(
    const CharacterAppearance& appearance,
    const std::string& pose) {

    auto items = build_item_string(appearance);
    if (items.empty()) {
        return std::nullopt;
    }

    auto url = "https://maplestory.io/api/character/" + items
               + "/" + pose + "/animated?resize=2";

    auto response = cpr::Get(
        cpr::Url{url},
        cpr::Timeout{30000}
    );

    if (response.status_code != 200) {
        return std::nullopt;
    }

    if (response.text.empty()) {
        return std::nullopt;
    }

    auto* begin = reinterpret_cast<const uint8_t*>(response.text.data());
    auto* end = begin + response.text.size();

    return std::vector<uint8_t>(begin, end);
}

} // namespace maple::api
