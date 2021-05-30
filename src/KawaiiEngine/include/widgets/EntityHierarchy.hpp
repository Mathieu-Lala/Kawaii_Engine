#pragma once

#include <entt/entt.hpp>

#include "component.hpp"

namespace kawe {

class EntityHierarchy {
public:
    std::optional<entt::entity> &selected;

    auto draw(entt::registry &world)
    {
        ImGui::Begin("KAWE: Entity Hierarchy");

        ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));
        if (ImGui::Selectable("none", !selected.has_value())) { selected = {}; }

        world.each([this, &world](const auto &entity) {
            const auto &name = world.get_or_emplace<Name>(entity, fmt::format("<anonymous#{}>", entity));
            if (ImGui::Selectable(name.component.data(), selected.has_value() && selected.value() == entity))
                selected = entity;
        });
        ImGui::EndChild();
        if (ImGui::Button("Create")) {
            const auto created = world.create();
            spdlog::warn("Creating entity {}", created);
            selected = created;
        }

        ImGui::End();
    }
};

} // namespace kawe
