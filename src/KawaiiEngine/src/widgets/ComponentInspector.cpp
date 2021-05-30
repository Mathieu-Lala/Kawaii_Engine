#include <imgui.h>

#include <spdlog/spdlog.h>

#include "helpers/macro.hpp"
#include "component.hpp"
#include "widgets/ComponentInspector.hpp"

template<>
auto kawe::ComponentInspector::drawComponentTweaker(entt::registry &world, entt::entity e, const Position3f &position) const
    -> void
{
    float temp[3] = {position.component.x, position.component.y, position.component.z};
    if (ImGui::InputFloat3("position", temp, "%.3f")) {
        world.patch<Position3f>(e, [&temp](auto &pos) {
            pos.component.x = temp[0];
            pos.component.y = temp[1];
            pos.component.z = temp[2];
        });
    }
}

template<>
auto kawe::ComponentInspector::drawComponentTweaker(entt::registry &world, entt::entity e, const Rotation3f &rotation) const
    -> void
{
    float temp[3] = {rotation.component.x, rotation.component.y, rotation.component.z};
    if (ImGui::InputFloat3("rotation", temp, "%.3f")) {
        world.patch<Rotation3f>(e, [&temp](auto &rot) {
            rot.component.x = temp[0];
            rot.component.y = temp[1];
            rot.component.z = temp[2];
        });
    }
}

template<>
auto kawe::ComponentInspector::drawComponentTweaker(entt::registry &world, entt::entity e, const Scale3f &scale) const
    -> void
{
    float temp[3] = {scale.component.x, scale.component.y, scale.component.z};
    if (ImGui::InputFloat3("scale", temp, "%.3f")) {
        world.patch<Scale3f>(e, [&temp](auto &scl) {
            scl.component.x = temp[0];
            scl.component.y = temp[1];
            scl.component.z = temp[2];
        });
    }
}

template<>
auto kawe::ComponentInspector::drawComponentTweaker(entt::registry &world, entt::entity e, const Name &name) const
    -> void
{
    char temp[255] = {0};
    std::strcpy(temp, name.component.data());
    if (ImGui::InputText("name", temp, sizeof(temp))) {
        world.patch<Name>(e, [&temp](auto &n) { n.component = temp; });
    }
}

auto kawe::ComponentInspector::draw(entt::registry &world) -> void
{
    ImGui::Begin("KAWE: Component Inspector");

    ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));
    ImGui::Text(
        "Entity: %s",
        selected.has_value()
            ? world.get_or_emplace<Name>(selected.value(), fmt::format("<anonymous#{}>", selected.value()))
                  .component.data()
            : "No entity selected");

    if (selected.has_value()) {
        ImGui::Separator();

        if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None)) {
            const auto try_variant = [&world, this]<typename Variant>(entt::entity e) {
                try {
                    if (const auto component = world.try_get<Variant>(e); component != nullptr) {
                        if (ImGui::BeginTabItem(Variant::name.data())) {
                            drawComponentTweaker(world, selected.value(), *component);
                            ImGui::SameLine();
                            if (ImGui::Button(fmt::format("Delete Component###{}", Variant::name).data())) {
                                spdlog::warn("Deleting component {} of {}", Variant::name, e);
                                world.remove<Variant>(e);
                            }
                            ImGui::EndTabItem();
                        }
                    }
                } catch (const std::exception &) {
                }
            };

            const auto draw_component =
                [&try_variant]<typename... T>(entt::entity entity, const std::variant<std::monostate, T...> &)
            {
                (try_variant.template operator()<T>(entity), ...);
            };

            draw_component(selected.value(), Component{});

            ImGui::EndTabBar();
        }
    }
    ImGui::EndChild();
    if (selected.has_value()) {
        if (ImGui::Button("Delete Entity")) {
            spdlog::warn("Deleting entity {}", selected.value());
            world.destroy(selected.value());
            selected = {};
        }
    }

    ImGui::End();
}
