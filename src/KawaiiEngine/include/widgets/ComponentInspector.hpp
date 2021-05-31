#pragma once

#include <optional>

#include <entt/entt.hpp>

namespace kawe {

class ComponentInspector {
public:
    std::optional<entt::entity> selected;

    auto draw(entt::registry &world) -> void;

private:
    template<typename T>
    auto drawComponentTweaker(entt::registry &, entt::entity, const T &) const -> void
    {
        ImGui::Text("<not implemented> %s", std::decay_t<T>::name.data());
    }
};

} // namespace kawe
