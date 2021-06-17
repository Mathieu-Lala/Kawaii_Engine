#include <spdlog/spdlog.h>

#include "graphics/deps.hpp"
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
auto kawe::ComponentInspector::drawComponentTweaker(entt::registry &world, entt::entity e, const FillColor &color) const
    -> void
{
    float temp[4] = {
        color.component.r,
        color.component.g,
        color.component.b,
        color.component.a,
    };
    if (ImGui::ColorEdit4("color", temp)) {
        world.patch<FillColor>(e, [&temp](auto &c) {
            c.component.r = temp[0];
            c.component.g = temp[1];
            c.component.b = temp[2];
            c.component.a = temp[3];
        });
    }
}

template<>
auto kawe::ComponentInspector::drawComponentTweaker(entt::registry &world, entt::entity e, const Texture2D &texture) const
    -> void
{
    ImGuiHelper::Text("path: {}", texture.filepath.data());

    ImGuiFileDialog::Instance()->SetExtentionInfos(".png", ImVec4(1.0f, 1.0f, 0.0f, 0.9f));
    ImGuiFileDialog::Instance()->SetExtentionInfos(".jpg", ImVec4(1.0f, 1.0f, 0.0f, 0.9f));
    ImGuiFileDialog::Instance()->SetExtentionInfos(".jpeg", ImVec4(1.0f, 1.0f, 0.0f, 0.9f));

    if (ImGui::Button("From File"))
        ImGuiFileDialog::Instance()->OpenDialog("kawe::Inspect::Mesh", "Choose File", ".png,.jpg,.jpeg", ".");

    if (ImGuiFileDialog::Instance()->Display("kawe::Inspect::Mesh")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            const auto path = ImGuiFileDialog::Instance()->GetFilePathName();

            Texture2D::emplace(world, e, path);
        }

        ImGuiFileDialog::Instance()->Close();
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
            "##combo_vao_mode", fmt::format("{} = {}", enum_name.data(), magic_enum::enum_name(vao.mode)).data())) {
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

    ImGui::Separator();

    kawe::ShaderProgram *shader_program = vao.shader_program;
    if (ImGui::BeginCombo("##combo_vao_shader", fmt::format("shader = {}", vao.shader_program->getName()).data())) {
        for (const auto &i : world.ctx<State *>()->shaders) {
            const auto is_selected = vao.shader_program->getName() == i->getName();
            if (ImGui::Selectable(i->getName().data(), is_selected)) {
                shader_program = i.get();
                world.patch<Render::VAO>(e, [&shader_program](Render::VAO &component) {
                    component.shader_program = shader_program;
                });
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

template<std::size_t S>
static auto stride_editor_ebo(
    entt::registry &world,
    entt::entity e,
    const kawe::Render::EBO &ebo,
    std::function<bool(std::size_t, std::array<std::uint32_t, S> &)> widget)
{
    std::size_t index = 0;
    for (auto it = ebo.indices.begin(); it != ebo.indices.end(); it += 3) {
        std::array<std::uint32_t, S> stride{};
        std::copy(it, it + 3, stride.begin());
        if (widget(index, stride)) {
            auto temp = ebo.indices;
            for (std::size_t i = 0; i != 3; i++) { temp[index * 3 + i] = stride[i]; }
            kawe::Render::EBO::emplace(world, e, temp);
            return;
        }
        index++;
    }
}

template<>
auto kawe::ComponentInspector::drawComponentTweaker(entt::registry &world, entt::entity e, const Render::EBO &ebo) const
    -> void
{
    stride_editor_ebo<3>(world, e, ebo, [&](auto index, auto &stride) {
        int temp[] = {static_cast<int>(stride[0]), static_cast<int>(stride[1]), static_cast<int>(stride[2])};
        return ImGui::InputInt3(fmt::format("{}", index).data(), temp);
    });
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
auto kawe::ComponentInspector::drawComponentTweaker(
    entt::registry &world, entt::entity e, const Render::VBO<Render::VAO::Attribute::TEXTURE_2D> &vbo) const
    -> void
{
    stride_editor<2>(world, e, vbo, [&](auto index, auto &stride) {
        return ImGui::InputFloat2(fmt::format("{}", index).data(), stride.data());
    });
}

constexpr static auto compute_normal(const glm::vec3 p1, const glm::vec3 p2, const glm::vec3 p3) -> glm::vec3
{
    const auto v1 = p2 - p1;
    const auto v2 = p3 - p1;
    return v1 * v2;
}

template<>
auto kawe::ComponentInspector::drawComponentTweaker(
    entt::registry &world, entt::entity e, const Render::VBO<Render::VAO::Attribute::NORMALS> &vbo) const -> void
{
    if (ImGui::Button("Compute the Normal from Position")) {
        std::vector<float> normal;

        const auto vbo_pos = world.get<Render::VBO<Render::VAO::Attribute::POSITION>>(e);
        if (const auto ebo = world.try_get<Render::EBO>(e); ebo == nullptr) {
            const auto offset = vbo_pos.stride_size * 3;
            for (std::size_t i = 0; i != vbo_pos.vertices.size() / vbo_pos.stride_size / 3; i++) {
                const auto vec_normal = compute_normal(
                    {vbo_pos.vertices[i * offset + 0],
                     vbo_pos.vertices[i * offset + 1],
                     vbo_pos.vertices[i * offset + 2]},
                    {vbo_pos.vertices[i * offset + 3],
                     vbo_pos.vertices[i * offset + 4],
                     vbo_pos.vertices[i * offset + 5]},
                    {vbo_pos.vertices[i * offset + 6],
                     vbo_pos.vertices[i * offset + 7],
                     vbo_pos.vertices[i * offset + 8]});
                for (int ii = 0; ii != 3; ii++) {
                    normal.push_back(vec_normal.x);
                    normal.push_back(vec_normal.y);
                    normal.push_back(vec_normal.z);
                }
            }
        } else {
            for (std::size_t i = 0; i < vbo_pos.vertices.size(); i += 3) {
                const auto vec_normal = compute_normal(
                    {vbo_pos.vertices[ebo->indices[i + 0] + 0],
                     vbo_pos.vertices[ebo->indices[i + 0] + 1],
                     vbo_pos.vertices[ebo->indices[i + 0] + 2]},
                    {vbo_pos.vertices[ebo->indices[i + 1] + 0],
                     vbo_pos.vertices[ebo->indices[i + 1] + 1],
                     vbo_pos.vertices[ebo->indices[i + 1] + 2]},
                    {vbo_pos.vertices[ebo->indices[i + 2] + 0],
                     vbo_pos.vertices[ebo->indices[i + 2] + 1],
                     vbo_pos.vertices[ebo->indices[i + 2] + 2]});
                normal.push_back(vec_normal.x);
                normal.push_back(vec_normal.y);
                normal.push_back(vec_normal.z);
            }
        }
        Render::VBO<Render::VAO::Attribute::NORMALS>::emplace(world, e, normal, 3);
    }

    stride_editor<3>(world, e, vbo, [&](auto index, auto &stride) {
        return ImGui::InputFloat3(fmt::format("{}", index).data(), stride.data());
    });
}


template<>
auto kawe::ComponentInspector::drawComponentTweaker(entt::registry &world, entt::entity e, const Mesh &mesh) const
    -> void
{
    ImGuiHelper::Text("path: {}", mesh.filepath.c_str());
    ImGuiHelper::Text("model: {}", mesh.model_name.c_str());
    ImGuiHelper::Text("loaded successfully: {}", mesh.loaded_successfully ? "Yes" : "No");

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
    ImGuiHelper::Text("min: {{.x: {}, .y: {}, .z: {}}}", aabb.min.x, aabb.min.y, aabb.min.z);
    ImGuiHelper::Text("max: {{.x: {}, .y: {}, .z: {}}}", aabb.max.x, aabb.max.y, aabb.max.z);
}

template<>
auto kawe::ComponentInspector::drawComponentTweaker(entt::registry &, entt::entity, const Collider &collider) const
    -> void
{
    constexpr auto enum_name = magic_enum::enum_type_name<Collider::CollisionStep>();
    ImGuiHelper::Text("{} = {}", enum_name.data(), magic_enum::enum_name(collider.step));
}

template<>
auto kawe::ComponentInspector::drawComponentTweaker(entt::registry &world, entt::entity, const Parent &parent) const
    -> void
{
    ImGuiHelper::Text(
        "parent = '{}'",
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
            ImGuiHelper::Text("children[{}] = '{}'", it++, world.get<Name>(i).component.data());
        }
    }
}

template<>
auto kawe::ComponentInspector::drawComponentTweaker(entt::registry &world, entt::entity e, const CameraData &camera) const
    -> void
{
    static constexpr auto epsilon = 0.0001f;

    // todo : improve me
    {
        bool viewport_updated = false;

        Rect4<float> temp = {
            .x = camera.viewport.x,
            .y = 1.0f - camera.viewport.y,
            .w = camera.viewport.w,
            .h = 1.0f - camera.viewport.h};

        ImGui::Dummy(ImVec2(18 * 2 + 4, 18));
        ImGui::SameLine();
        ImGui::PushItemWidth(160);
        viewport_updated |= ImGui::SliderFloat("##viewport.x", &temp.x, 0.0f, 1.0f, "");
        ImGui::PopItemWidth();

        ImGui::Dummy(ImVec2(18 * 2 + 4, 18));
        ImGui::SameLine();
        ImGui::PushItemWidth(160);
        viewport_updated |= ImGui::SliderFloat("##viewport.w", &temp.w, 0.0f, 1.0f, "");
        ImGui::PopItemWidth();

        viewport_updated |= ImGui::VSliderFloat("##viewport.y", ImVec2(18, 160), &temp.y, epsilon, 1.0f, "");
        temp.y = 1.0f - temp.y;
        ImGui::SameLine();
        viewport_updated |= ImGui::VSliderFloat("##viewport.h", ImVec2(18, 160), &temp.h, epsilon, 1.0f, "");
        temp.h = 1.0f - temp.h;

        ImGui::SameLine();
        const auto old_cursor = ImGui::GetCursorPos();

        ImGui::SetCursorPos(ImVec2(old_cursor.x + temp.x * 160, old_cursor.y + temp.y * 160));
        ImGui::Button(
            "A", ImVec2(std::max(epsilon, temp.w - temp.x) * 160, std::max(epsilon, temp.h - temp.y) * 160));
        ImGui::SetCursorPosY(old_cursor.y + 160 + 4);

        if (viewport_updated) {
            world.patch<CameraData>(e, [&temp](auto &c) {
                c.viewport = Rect4<float>{
                    .x = temp.x,
                    .y = temp.y,
                    .w = temp.w,
                    .h = temp.h,
                };
            });
        }
    }
    ImGui::Separator();

    ImGui::PushItemWidth(150);
    {
        auto fov_temp = static_cast<float>(camera.fov);
        if (ImGui::DragFloat(
                "fov", &fov_temp, 1.0f, epsilon, 180.0f - epsilon, "%.3f", ImGuiSliderFlags_Logarithmic)) {
            world.patch<CameraData>(e, [&fov_temp](auto &c) { c.fov = fov_temp; });
        }
    }
    ImGui::SameLine();
    {
        auto near_temp = static_cast<float>(camera.near);
        if (ImGui::DragFloat("near", &near_temp, 1.0f, epsilon, static_cast<float>(camera.far) - epsilon)) {
            world.patch<CameraData>(e, [&near_temp](auto &c) { c.near = near_temp; });
        }
    }
    ImGui::SameLine();
    {
        auto far_temp = static_cast<float>(camera.far);
        if (ImGui::DragFloat(
                "far",
                &far_temp,
                1.0f,
                static_cast<float>(camera.near) + epsilon,
                1'000'000.0f,
                "%.3f",
                ImGuiSliderFlags_Logarithmic)) {
            world.patch<CameraData>(e, [&far_temp](auto &c) { c.far = far_temp; });
        }
    }
    ImGui::PopItemWidth();

    ImGui::Separator();

    ImGuiHelper::Text(
        "target position: {{.x: {}, .y: {}, .z: {}}}",
        camera.target_center.x,
        camera.target_center.y,
        camera.target_center.z);

    // read only data

    ImGuiHelper::Text("display: {{.x: {}, .y: {}}}", camera.display.x, camera.display.y);

    ImGuiHelper::Text(
        "plan vert: {{.x: {}, .y: {}, .z: {}}}",
        camera.imagePlaneVertDir.x,
        camera.imagePlaneVertDir.y,
        camera.imagePlaneVertDir.z);

    ImGuiHelper::Text(
        "plan horz: {{.x: {}, .y: {}, .z: {}}}",
        camera.imagePlaneHorizDir.x,
        camera.imagePlaneHorizDir.y,
        camera.imagePlaneHorizDir.z);
}

auto kawe::ComponentInspector::draw(entt::registry &world) -> void
{
    if (!ImGui::Begin("KAWE: Component Inspector")) return ImGui::End();

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
