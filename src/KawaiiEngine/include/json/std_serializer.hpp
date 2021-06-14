#pragma once

#include <cstdint>
#include <chrono>

#include <nlohmann/json.hpp>

namespace nlohmann {

template<>
struct adl_serializer<std::chrono::steady_clock::duration> {
    static void to_json(nlohmann::json &j, const std::chrono::steady_clock::duration &duration)
    {
        j = nlohmann::json{{"nanoseconds", std::chrono::nanoseconds{duration}.count()}};
    }

    static void from_json(const nlohmann::json &j, std::chrono::steady_clock::duration &duration)
    {
        std::uint64_t value = j.at("nanoseconds");
        duration = std::chrono::nanoseconds{value};
    }
};

} // namespace nlohmann
