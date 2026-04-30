#include "api/maplestory_io.hpp"

#include <cpr/cpr.h>
#include <string>

namespace maple::api {

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

    // cpr stores binary data in response.text as raw bytes
    auto* begin = reinterpret_cast<const uint8_t*>(response.text.data());
    auto* end = begin + response.text.size();

    return std::vector<uint8_t>(begin, end);
}

} // namespace maple::api
