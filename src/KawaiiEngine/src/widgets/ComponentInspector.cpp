#include <imgui.h>

#include <spdlog/spdlog.h>

#include <ImGuiFileDialog.h>

#include "helpers/macro.hpp"
#include "component.hpp"
#include "widgets/ComponentInspector.hpp"

template<>
auto kawe::ComponentInspector::drawComponentTweaker(entt::registry &world, entt::entity e, const Position3f &position) const
    -> void
{
    float temp[3] = {
        static_cast<float>(position.component.x),
        static_cast<float>(position.component.y),
        static_cast<float>(position.component.z)};
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
    float temp[3] = {
        static_cast<float>(rotation.component.x),
        static_cast<float>(rotation.component.y),
        static_cast<float>(rotation.component.z)};
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
    float temp[3] = {
        static_cast<float>(scale.component.x),
        static_cast<float>(scale.component.y),
        static_cast<float>(scale.component.z)};
    if (ImGui::InputFloat3("scale", temp, "%.3f")) {
        world.patch<Scale3f>(e, [&temp](auto &scl) {
            scl.component.x = temp[0];
            scl.component.y = temp[1];
            scl.component.z = temp[2];
        });
    }
}

template<>
auto kawe::ComponentInspector::drawComponentTweaker(entt::registry &world, entt::entity e, const Velocity3f &vel) const
    -> void
{
    float temp[3] = {
        static_cast<float>(vel.component.x),
        static_cast<float>(vel.component.y),
        static_cast<float>(vel.component.z)};
    if (ImGui::InputFloat3("velocity", temp, "%.3f")) {
        world.patch<Velocity3f>(e, [&temp](auto &v) {
            v.component.x = temp[0];
            v.component.y = temp[1];
            v.component.z = temp[2];
        });
    }
}

template<>
auto kawe::ComponentInspector::drawComponentTweaker(entt::registry &world, entt::entity e, const Gravitable3f &gravity) const
    -> void
{
    float temp[3] = {
        static_cast<float>(gravity.component.x),
        static_cast<float>(gravity.component.y),
        static_cast<float>(gravity.component.z)};
    if (ImGui::InputFloat3("velocity", temp, "%.3f")) {
        world.patch<Gravitable3f>(e, [&temp](auto &grav) {
            grav.component.x = temp[0];
            grav.component.y = temp[1];
            grav.component.z = temp[2];
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

template<>
auto kawe::ComponentInspector::drawComponentTweaker(entt::registry &world, entt::entity e, const Render::VAO &vao) const
    -> void
{
    constexpr auto enum_name = magic_enum::enum_type_name<Render::VAO::DisplayMode>();
    auto display_mode = vao.mode;
    if (ImGui::BeginCombo(
            "##combo", fmt::format("{} = {}", enum_name.data(), magic_enum::enum_name(vao.mode)).data())) {
        for (const auto &i : Render::VAO::DISPLAY_MODES) {
            const auto is_selected = vao.mode == i;
            if (ImGui::Selectable(magic_enum::enum_name(i).data(), is_selected)) {
                display_mode = i;
                world.patch<Render::VAO>(
                    e, [&display_mode](Render::VAO &component) { component.mode = display_mode; });
            }
            if (is_selected) { ImGui::SetItemDefaultFocus(); }
        }
        ImGui::EndCombo();
    }
}

template<std::size_t S, kawe::Render::VAO::Attribute A>
static auto stride_editor(
    entt::registry &world,
    entt::entity e,
    const kawe::Render::VBO<A> &vbo,
    std::function<bool(std::size_t, std::array<float, S> &)> widget)
{
    std::size_t index = 0;
    for (auto it = vbo.vertices.begin(); it != vbo.vertices.end(); it += static_cast<long>(vbo.stride_size)) {
        std::array<float, S> stride{};
        std::copy(it, it + static_cast<long>(vbo.stride_size), stride.begin());
        if (widget(index, stride)) {
            auto temp = vbo.vertices;
            for (std::size_t i = 0; i != vbo.stride_size; i++) {
                temp[index * vbo.stride_size + i] = stride[i];
            }
            kawe::Render::VBO<A>::emplace(world, e, temp, vbo.stride_size);
            return;
        }
        index++;
    }
}

template<>
auto kawe::ComponentInspector::drawComponentTweaker(
    entt::registry &world, entt::entity e, const Render::VBO<Render::VAO::Attribute::POSITION> &vbo) const -> void
{
    stride_editor<3>(world, e, vbo, [&](auto index, auto &stride) {
        return ImGui::InputFloat3(fmt::format("{}", index).data(), stride.data());
    });
}

template<>
auto kawe::ComponentInspector::drawComponentTweaker(
    entt::registry &world, entt::entity e, const Render::VBO<Render::VAO::Attribute::COLOR> &vbo) const -> void
{
    stride_editor<4>(world, e, vbo, [&](auto index, auto &stride) {
        return ImGui::ColorEdit4(fmt::format("{}", index).data(), stride.data());
    });
}

template<>
auto kawe::ComponentInspector::drawComponentTweaker(entt::registry &world, entt::entity e, const Mesh &mesh) const
    -> void
{
    ImGui::Text("path: %s", mesh.filepath.c_str());
    ImGui::Text("model: %s", mesh.model_name.c_str());
    ImGui::Text("loaded successfully: %s", mesh.loaded_successfully ? "Yes" : "No");

    ImGuiFileDialog::Instance()->SetExtentionInfos(".obj", ImVec4(1.0f, 1.0f, 0.0f, 0.9f));

    if (ImGui::Button("From File"))
        ImGuiFileDialog::Instance()->OpenDialog("kawe::Inspect::Mesh", "Choose File", ".obj", ".");

    if (ImGuiFileDialog::Instance()->Display("kawe::Inspect::Mesh")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            const auto path = ImGuiFileDialog::Instance()->GetFilePathName();

            world.remove_if_exists<Render::VBO<Render::VAO::Attribute::POSITION>>(e);
            world.remove_if_exists<Render::VBO<Render::VAO::Attribute::COLOR>>(e);
            world.remove_if_exists<Render::VBO<Render::VAO::Attribute::NORMALS>>(e);
            world.remove_if_exists<Render::EBO>(e);
            world.remove_if_exists<Render::VAO>(e);

            Mesh::emplace(world, e, path);
        }

        ImGuiFileDialog::Instance()->Close();
    }
}

template<>
auto kawe::ComponentInspector::drawComponentTweaker(entt::registry &, entt::entity, const AABB &aabb) const -> void
{
    ImGui::Text(fmt::format("min: {{.x: {}, .y: {}, .z: {}}}", aabb.min.x, aabb.min.y, aabb.min.z).data());
    ImGui::Text(fmt::format("max: {{.x: {}, .y: {}, .z: {}}}", aabb.max.x, aabb.max.y, aabb.max.z).data());
}

template<>
auto kawe::ComponentInspector::drawComponentTweaker(entt::registry &, entt::entity, const Collider &collider) const
    -> void
{
    constexpr auto enum_name = magic_enum::enum_type_name<Collider::CollisionStep>();
    ImGui::Text(fmt::format("{} = {}", enum_name.data(), magic_enum::enum_name(collider.step)).data());
}

template<>
auto kawe::ComponentInspector::drawComponentTweaker(entt::registry &world, entt::entity, const Parent &parent) const
    -> void
{
    ImGui::Text(
        "parent = '%s'",
        world.valid(parent.component) ? world.get<Name>(parent.component).component.data() : "null");
}

template<>
auto kawe::ComponentInspector::drawComponentTweaker(entt::registry &world, entt::entity, const Children &children) const
    -> void
{
    if (children.component.empty()) {
        ImGui::Text("No children yet");
    } else {
        int it = 0;
        for (const auto &i : children.component) {
            ImGui::Text("children[%d] = '%s'", it++, world.get<Name>(i).component.data());
        }
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
            (selected.value(), Component{});

            if (ImGui::BeginTabItem("+")) {
                [try_variant = [&world]<typename Variant>(entt::entity e) {
                    if (const auto component = world.try_get<Variant>(e); component == nullptr) {
                        if (ImGui::Selectable(Variant::name.data(), false)) { world.emplace<Variant>(e); }
                    }
                }]<typename... T>(entt::entity entity, const std::variant<std::monostate, T...> &)
                {
                    (try_variant.template operator()<T>(entity), ...);
                }
                (selected.value(), Component{});

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
