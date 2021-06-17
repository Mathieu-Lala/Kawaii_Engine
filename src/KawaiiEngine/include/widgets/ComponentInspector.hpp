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
        ImGuiHelper::Text("<not implemented> {}", std::decay_t<T>::name.data());
    }
};

} // namespace kawe
