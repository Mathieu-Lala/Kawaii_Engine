#pragma once

#include <optional>

#include <entt/entt.hpp>

namespace kawe {

class ComponentInspector {
public:
    ComponentInspector() : count{s_count++} {}

    std::optional<entt::entity> selected;
    int count = 0;

    static int s_count;

    template<typename MyVariant>
    auto draw(entt::registry &world) -> void
    {
        if (!ImGui::Begin(fmt::format("KAWE: Component Inspector ({})", count).data())) return ImGui::End();

        ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));
        ImGuiHelper::Text(
            "Entity: {}",
            selected.has_value()
                ? world
                      .get_or_emplace<Name>(selected.value(), fmt::format("<kawe:anonymous#{}>", selected.value()))
                      .component.data()
                : "No entity selected");

        if (selected.has_value()) {
            ImGui::Separator();

            if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None)) {
                [try_variant = [&world, this]<typename Variant>(entt::entity e) {
                    if (const auto component = world.try_get<Variant>(e); component != nullptr) {
                        if (ImGui::BeginTabItem(Variant::name.data())) {
                            if (ImGui::Button(fmt::format("Delete Component###{}", Variant::name).data())) {
                                spdlog::warn("Deleting component {} of {}", Variant::name, e);
                                world.remove<Variant>(e);
                            }
                            ImGui::Separator();
                            drawComponentTweaker(world, selected.value(), *component);
                            ImGui::EndTabItem();
                        }
                    }
                }]<typename... T>(entt::entity entity, const std::variant<std::monostate, T...> &)
                {
                    (try_variant.template operator()<T>(entity), ...);
                }
                (selected.value(), MyVariant{});

                if (ImGui::BeginTabItem("+")) {
                    [try_variant = [&world]<typename Variant>(entt::entity e) {
                        if (const auto component = world.try_get<Variant>(e); component == nullptr) {
                            if (ImGui::Selectable(Variant::name.data(), false)) { world.emplace<Variant>(e); }
                        }
                    }]<typename... T>(entt::entity entity, const std::variant<std::monostate, T...> &)
                    {
                        (try_variant.template operator()<T>(entity), ...);
                    }
                    (selected.value(), MyVariant{});

                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        }
        ImGui::EndChild();
        if (selected.has_value()) {
            ImGui::Separator();
            if (ImGui::Button("Delete Entity")) {
                spdlog::warn("Deleting entity {}", selected.value());
                world.destroy(selected.value());
                selected = {};
            }
        }

        ImGui::End();
    }

private:
    template<typename T>
    auto drawComponentTweaker(entt::registry &, entt::entity, const T &) const -> void
    {
        ImGuiHelper::Text("<not implemented> {}", std::decay_t<T>::name.data());
    }
};

} // namespace kawe
