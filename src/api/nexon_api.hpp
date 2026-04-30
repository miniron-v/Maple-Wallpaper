#pragma once

#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace maple::api {

struct CharacterBasic {
    std::string ocid;
    std::string name;
    int level = 0;
    std::string job;
    std::string world;
    std::string image_url;
};

struct EquipmentItem {
    std::string slot;
    std::string name;
    std::string icon;
    int item_id = 0;
};

struct CharacterAppearance {
    std::vector<int> equip_ids;
    int skin_id = 0;
    int face_id = 0;
    int hair_id = 0;
};

class NexonApi {
public:
    explicit NexonApi(const std::string& api_key);

    std::optional<std::string> fetch_ocid(const std::string& character_name);
    std::optional<CharacterBasic> fetch_basic(const std::string& ocid);
    std::optional<CharacterAppearance> fetch_appearance(const std::string& ocid);

private:
    std::string api_key_;

    std::optional<nlohmann::json> api_get(const std::string& endpoint,
                                          const std::string& query = "");

    void collect_equipment_ids(CharacterAppearance& out, const std::string& ocid);
    void collect_cash_equipment_ids(CharacterAppearance& out, const std::string& ocid);
    void collect_beauty_ids(CharacterAppearance& out, const std::string& ocid);
};

} // namespace maple::api
