#pragma once

#include <ctime>
#include <string>

namespace kawe {

inline auto time_to_string(std::time_t now = std::time(nullptr)) -> std::string
{
    const auto tp = std::localtime(&now);
    char buffer[32];
    return std::strftime(buffer, sizeof(buffer), "%Y-%m-%d_%H-%M-%S", tp) ? buffer : "1970-01-01_00:00:00";
}

} // namespace kawe
