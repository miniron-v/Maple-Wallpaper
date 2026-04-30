#include "api/nexon_api.hpp"

#include <cpr/cpr.h>
#include <regex>

namespace maple::api {

namespace {

const std::string BASE_URL = "https://open.api.nexon.com/maplestory/v1";

// Extracts numeric item ID from nexon icon URLs like:
// https://open.api.nexon.com/static/maplestory/ItemIcon/ABCDEFGH.png
// The encoded segment may not always be a plain number, so we attempt
// multiple extraction strategies.
int extract_item_id_from_url(const std::string& url) {
    std::regex numeric_segment(R"(/(\d+)\.png)");
    std::smatch match;
    if (std::regex_search(url, match, numeric_segment)) {
        try {
            return std::stoi(match[1].str());
        } catch (...) {
            return 0;
        }
    }
    return 0;
}

} // namespace

NexonApi::NexonApi(const std::string& api_key) : api_key_(api_key) {}

std::optional<nlohmann::json> NexonApi::api_get(const std::string& endpoint,
                                                 const std::string& query) {
    auto url = BASE_URL + endpoint;
    if (!query.empty()) {
        url += "?" + query;
    }

    auto response = cpr::Get(
        cpr::Url{url},
        cpr::Header{{"x-nxopen-api-key", api_key_}},
        cpr::Timeout{10000}
    );

    if (response.status_code != 200) {
        return std::nullopt;
    }

    try {
        return nlohmann::json::parse(response.text);
    } catch (const nlohmann::json::parse_error&) {
        return std::nullopt;
    }
}

std::optional<std::string> NexonApi::fetch_ocid(const std::string& character_name) {
    // Use cpr::util::urlEncode and convert SecureString to std::string
    auto encoded = cpr::util::urlEncode(character_name);
    std::string encoded_str(encoded.data(), encoded.size());
    auto j = api_get("/id", "character_name=" + encoded_str);

    if (!j || !j->contains("ocid")) {
        return std::nullopt;
    }
    return (*j)["ocid"].get<std::string>();
}

std::optional<CharacterBasic> NexonApi::fetch_basic(const std::string& ocid) {
    auto j = api_get("/character/basic", "ocid=" + ocid);
    if (!j) {
        return std::nullopt;
    }

    CharacterBasic basic;
    basic.ocid = ocid;

    auto& data = *j;
    if (data.contains("character_name")) {
        basic.name = data["character_name"].get<std::string>();
    }
    if (data.contains("character_level")) {
        basic.level = data["character_level"].get<int>();
    }
    if (data.contains("character_class")) {
        basic.job = data["character_class"].get<std::string>();
    }
    if (data.contains("world_name")) {
        basic.world = data["world_name"].get<std::string>();
    }
    if (data.contains("character_image")) {
        basic.image_url = data["character_image"].get<std::string>();
    }

    return basic;
}

std::optional<CharacterAppearance> NexonApi::fetch_appearance(const std::string& ocid) {
    CharacterAppearance appearance;

    collect_equipment_ids(appearance, ocid);
    collect_cash_equipment_ids(appearance, ocid);
    collect_beauty_ids(appearance, ocid);

    return appearance;
}

void NexonApi::collect_equipment_ids(CharacterAppearance& out,
                                      const std::string& ocid) {
    auto j = api_get("/character/item-equipment", "ocid=" + ocid);
    if (!j || !j->contains("item_equipment")) {
        return;
    }

    for (auto& item : (*j)["item_equipment"]) {
        int id = 0;
        // item_shape_icon shows actual visual appearance
        if (item.contains("item_shape_icon") && item["item_shape_icon"].is_string()) {
            id = extract_item_id_from_url(item["item_shape_icon"].get<std::string>());
        }
        if (id == 0 && item.contains("item_icon") && item["item_icon"].is_string()) {
            id = extract_item_id_from_url(item["item_icon"].get<std::string>());
        }
        if (id != 0) {
            out.equip_ids.push_back(id);
        }
    }
}

void NexonApi::collect_cash_equipment_ids(CharacterAppearance& out,
                                           const std::string& ocid) {
    auto j = api_get("/character/cashitem-equipment", "ocid=" + ocid);
    if (!j || !j->contains("cash_item_equipment_base")) {
        return;
    }

    for (auto& item : (*j)["cash_item_equipment_base"]) {
        int id = 0;
        if (item.contains("cash_item_icon") && item["cash_item_icon"].is_string()) {
            id = extract_item_id_from_url(item["cash_item_icon"].get<std::string>());
        }
        if (id != 0) {
            out.equip_ids.push_back(id);
        }
    }
}

void NexonApi::collect_beauty_ids(CharacterAppearance& out,
                                   const std::string& ocid) {
    auto j = api_get("/character/beauty-equipment", "ocid=" + ocid);
    if (!j) {
        return;
    }

    auto& bdata = *j;
    if (bdata.contains("character_hair") && bdata["character_hair"].is_object()) {
        auto& hair = bdata["character_hair"];
        if (hair.contains("hair_icon") && hair["hair_icon"].is_string()) {
            out.hair_id = extract_item_id_from_url(hair["hair_icon"].get<std::string>());
        }
    }

    if (bdata.contains("character_face") && bdata["character_face"].is_object()) {
        auto& face = bdata["character_face"];
        if (face.contains("face_icon") && face["face_icon"].is_string()) {
            out.face_id = extract_item_id_from_url(face["face_icon"].get<std::string>());
        }
    }

    // Skin is identified by name; default to 2000 (light skin)
    if (bdata.contains("character_skin_name") && bdata["character_skin_name"].is_string()) {
        out.skin_id = 2000;
    }
}

} // namespace maple::api
