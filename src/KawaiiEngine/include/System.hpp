#pragma once

#include "Action.hpp"

namespace kawe {

struct System {
    entt::registry &my_world;
    Context &ctx;
    Window &window;

    System(entt::registry &world, entt::dispatcher &dispatcher, Context &context, Window &w) :
        my_world{world}, ctx{context}, window{w}
    {
        {
            // rendering backend memory cleanup
            my_world.on_destroy<Render::VAO>().connect<Render::VAO::on_destroy>();

            my_world.on_destroy<Render::VBO<Render::VAO::Attribute::POSITION>>()
                .connect<Render::VBO<Render::VAO::Attribute::POSITION>::on_destroy>();

            my_world.on_destroy<Render::VBO<Render::VAO::Attribute::COLOR>>()
                .connect<Render::VBO<Render::VAO::Attribute::COLOR>::on_destroy>();

            my_world.on_destroy<Render::VBO<Render::VAO::Attribute::NORMALS>>()
                .connect<Render::VBO<Render::VAO::Attribute::NORMALS>::on_destroy>();

            my_world.on_destroy<Render::VBO<Render::VAO::Attribute::TEXTURE_2D>>()
                .connect<Render::VBO<Render::VAO::Attribute::TEXTURE_2D>::on_destroy>();

            my_world.on_destroy<Render::EBO>().connect<Render::EBO::on_destroy>();
        }

        {
            // if a transform component is updated, try to update the AABB
            my_world.on_construct<Position3f>().connect<&System::on_update_aabb>(*this);
            my_world.on_construct<Position3f>().connect<&System::on_update_aabb>(*this);
            my_world.on_construct<Position3f>().connect<&System::on_update_aabb>(*this);
            my_world.on_update<Position3f>().connect<&System::on_update_aabb>(*this);
            my_world.on_update<Rotation3f>().connect<&System::on_update_aabb>(*this);
            my_world.on_update<Scale3f>().connect<&System::on_update_aabb>(*this);

            // if the position vertices updated, try to update the AABB
            my_world.on_construct<Render::VBO<Render::VAO::Attribute::POSITION>>().connect<&System::on_update_aabb>(*this);
            my_world.on_update<Render::VBO<Render::VAO::Attribute::POSITION>>().connect<&System::on_update_aabb>(*this);

            // if a collider is created, emplace a AABB
            my_world.on_construct<Collider>().connect<&System::on_update_aabb>(*this);
        }

        {
            // run the collision pipeline
            my_world.on_construct<AABB>().connect<&System::run_collision_pipeline>(*this);
            my_world.on_update<AABB>().connect<&System::run_collision_pipeline>(*this);
        }

        {
            // update color
            my_world.on_construct<FillColor>().connect<&System::on_fill_color_update>(*this);
            my_world.on_update<FillColor>().connect<&System::on_fill_color_update>(*this);

            my_world.on_update<Render::VBO<Render::VAO::Attribute::COLOR>>()
                .connect<[](entt::registry &reg, entt::entity e) -> void { reg.remove_if_exists<FillColor>(e); }>();
        }

        {
            // initialize camera child entity
            my_world.on_construct<CameraData>().connect<&System::on_create_camera>(*this);
        }

        {
            // update the view and projection matrix
            // my_world.on_construct<CameraData>().connect<&System::on_update_camera>(*this);
            my_world.on_update<CameraData>().connect<&System::on_update_camera>(*this);
            my_world.on_update<Position3f>().connect<&System::on_update_camera>(*this);
            my_world.on_update<Position3f>().connect<&System::try_update_camera_target>(*this);

            dispatcher.sink<event::TimeElapsed>().connect<&System::on_time_elapsed_camera>(*this);
        }

        {
            // updating clocks.
            dispatcher.sink<event::TimeElapsed>().connect<&System::on_time_elapsed_clock>(*this);
        }

        {
            dispatcher.sink<event::TimeElapsed>().connect<&System::on_time_elapsed_physics>(*this);
        }

        {
            dispatcher.sink<action::Render<Render::Layout::SCENE>>().connect<&System::on_time_elapsed_render>(*this);
        }
    }

    // static auto system_rendering() -> void;

    auto on_update_aabb(entt::registry &reg, entt::entity e) -> void
    {
        if (const auto collider = reg.try_get<Collider>(e); collider != nullptr) {
            if (const auto vbo = reg.try_get<Render::VBO<Render::VAO::Attribute::POSITION>>(e); vbo != nullptr) {
                AABB::emplace(reg, e, vbo->vertices);
            }
        }
    }

    auto run_collision_pipeline(entt::registry &reg, entt::entity e) -> void
    {
        // AABB algorithm = really simple and fast collision detection
        const auto aabb = reg.get<AABB>(e);
        bool has_aabb_collision = false;
        for (const auto &other : reg.view<Collider, AABB>()) {
            if (e == other) continue;
            const auto other_aabb = reg.get<AABB>(other);

            const auto collide = (aabb.min.x <= other_aabb.max.x && aabb.max.x >= other_aabb.min.x)
                                 && (aabb.min.y <= other_aabb.max.y && aabb.max.y >= other_aabb.min.y)
                                 && (aabb.min.z <= other_aabb.max.z && aabb.max.z >= other_aabb.min.z);
            has_aabb_collision |= collide;

            if (collide) {
                reg.patch<Collider>(e, [](auto &c) { c.step = Collider::CollisionStep::AABB; });
                reg.patch<Collider>(other, [](auto &c) { c.step = Collider::CollisionStep::AABB; });
                reg.emplace_or_replace<FillColor>(aabb.guizmo, glm::vec4{1.0f, 0.0f, 0.0f, 1.0f});
                reg.emplace_or_replace<FillColor>(other_aabb.guizmo, glm::vec4{1.0f, 0.0f, 0.0f, 1.0f});
            }
        }

        // todo : this logic should be done on another signal named on_collision_resolved ...
        const auto collider = reg.get<Collider>(e);
        if (!has_aabb_collision) {
            if (collider.step != Collider::CollisionStep::NONE) {
                reg.patch<Collider>(e, [](auto &c) { c.step = Collider::CollisionStep::NONE; });
                reg.emplace_or_replace<FillColor>(aabb.guizmo, glm::vec4{0.0f, 0.0f, 0.0f, 1.0f});
            }
        }
    }

    auto on_fill_color_update(entt::registry &reg, entt::entity e) -> void
    {
        const auto vbo_color = reg.try_get<Render::VBO<Render::VAO::Attribute::COLOR>>(e);
        const auto vbo_pos = reg.try_get<Render::VBO<Render::VAO::Attribute::POSITION>>(e);
        if (!vbo_color && !vbo_pos) return;

        const auto size = vbo_color != nullptr ? vbo_color->vertices.size() : vbo_pos->vertices.size();
        const auto stride_size = vbo_color != nullptr ? vbo_color->stride_size : vbo_pos->stride_size;

        const auto &fill_color = reg.get<FillColor>(e);
        std::vector<float> vert{};
        for (auto i = 0ul; i != size; i += stride_size) {
            vert.emplace_back(fill_color.component.r);
            vert.emplace_back(fill_color.component.g);
            vert.emplace_back(fill_color.component.b);
            vert.emplace_back(fill_color.component.a);
        }

        Render::VBO<Render::VAO::Attribute::COLOR>::emplace(reg, e, vert, 4);
    }

    auto on_create_camera(entt::registry &reg, entt::entity e) -> void
    {
        const auto child = reg.create();
        reg.emplace<Position3f>(child);
        reg.emplace<Parent>(child, e);
        reg.emplace<Name>(child, fmt::format("<kawe:camera_target#{}>", e));

        reg.emplace_or_replace<Children>(e).component.push_back(child);
        reg.patch<CameraData>(e, [&child](auto &cam) { cam.target = child; });
    }

    auto on_update_camera(entt::registry &reg, entt::entity e) -> void
    {
        const auto &cam = reg.try_get<CameraData>(e);
        if (cam == nullptr) { return; }
        const auto &pos = reg.get_or_emplace<Position3f>(e, Position3f{glm::dvec3{1.0, 1.0, 1.0}});

        const auto makeOrthogonalTo = [](const glm::dvec3 &vec1, const glm::dvec3 &vec2) -> glm::dvec3 {
            if (const auto length = glm::length(vec2); length != 0) {
                const auto scale = (vec1.x * vec2.x + vec1.y * vec2.y + vec1.z * vec2.z) / (length * length);
                return {
                    vec1.x - scale * vec2.x,
                    vec1.y - scale * vec2.y,
                    vec1.z - scale * vec2.z,
                };
            } else {
                return vec1;
            }
        };

        const auto &target_center = reg.get<Position3f>(cam->target).component;

        const auto viewDir = glm::normalize(target_center - pos.component);

        cam->imagePlaneVertDir = glm::normalize(makeOrthogonalTo(cam->up, viewDir));
        cam->imagePlaneHorizDir = glm::normalize(glm::cross(viewDir, cam->imagePlaneVertDir));

        const auto size = window.getSize<double>() * glm::dvec2{cam->viewport.w, cam->viewport.h};

        cam->display.y = 2.0 * glm::length(target_center - pos.component) * std::tan(0.5 * cam->fov);
        cam->display.x = cam->display.y * (size.x / size.y);

        cam->projection = glm::perspective(glm::radians(cam->fov), size.x / size.y, cam->near, cam->far);
        cam->view = glm::lookAt(pos.component, target_center, cam->up);
    }

    auto try_update_camera_target(entt::registry &reg, entt::entity e) -> void
    {
        if (const auto has_parent = reg.try_get<Parent>(e); has_parent != nullptr) {
            if (const auto is_camera = reg.try_get<CameraData>(has_parent->component); is_camera != nullptr) {
                on_update_camera(reg, has_parent->component);
            }
        }
    }

    auto on_time_elapsed_camera(const event::TimeElapsed &e) -> void
    {
        const auto dt_nano = e.elapsed;
        const auto dt_secs =
            static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(dt_nano).count()) / 1'000'000.0;

        if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow)) {
            for (auto &camera : my_world.view<CameraData>()) {
                const auto &data = my_world.get<CameraData>(camera);
                const auto viewport = data.viewport;
                const auto window_size = window.getSize<double>();

                if (!Rect4<double>{
                        static_cast<double>(viewport.x) * window_size.x,
                        static_cast<double>(viewport.y) * window_size.y,
                        static_cast<double>(viewport.w) * window_size.x,
                        static_cast<double>(viewport.h) * window_size.y}
                         .contains(ctx.mouse_pos)) {
                    continue;
                }

                for (const auto &[button, pressed] : ctx.state_mouse_button) {
                    if (!pressed) { continue; }

                    const auto ms = dt_secs * 1'000.0;
                    const auto size = window.getSize<double>() * glm::dvec2{data.viewport.w, data.viewport.h};

                    switch (button) {
                    case event::MouseButton::Button::BUTTON_LEFT: {
                        const auto amount_x = (ctx.mouse_pos.x - ctx.mouse_pos_when_pressed.x) / size.x;
                        const auto amount_y = (ctx.mouse_pos_when_pressed.y - ctx.mouse_pos.y) / size.y;
                        CameraData::rotate(my_world, camera, data, {amount_x * ms, amount_y * ms});
                    } break;
                    case event::MouseButton::Button::BUTTON_MIDDLE: {
                        const auto amount = (ctx.mouse_pos_when_pressed.y - ctx.mouse_pos.y) / size.y;
                        CameraData::zoom(my_world, camera, data, amount * ms);
                    } break;
                    case event::MouseButton::Button::BUTTON_RIGHT: {
                        const auto amount_x = (ctx.mouse_pos.x - ctx.mouse_pos_when_pressed.x) / size.x;
                        const auto amount_y = (ctx.mouse_pos_when_pressed.y - ctx.mouse_pos.y) / size.y;
                        CameraData::translate(
                            my_world,
                            camera,
                            data,
                            {-amount_x * ms, -amount_y * ms},
                            !ctx.keyboard_state[event::Key::Code::KEY_LEFT_CONTROL]);
                    } break;
                    default: break;
                    }
                }
            }
        }
    }

    auto on_time_elapsed_clock(const event::TimeElapsed &e) -> void
    {
        my_world.view<Clock>().each([&e](Clock &clock) { clock.on_update(e); });
    }

    auto on_time_elapsed_physics(const event::TimeElapsed &e) -> void
    {
        const auto dt_secs =
            static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(e.world_time).count()) / 1'000'000.0;

        for (const auto &entity : my_world.view<Position3f, Velocity3f>()) {
            const auto &vel = my_world.get<Velocity3f>(entity);
            my_world.patch<Position3f>(
                entity, [&vel, &dt_secs](auto &pos) { pos.component += vel.component * static_cast<double>(dt_secs); });
        }

        for (const auto &entity : my_world.view<Gravitable3f, Velocity3f>()) {
            const auto &gravity = my_world.get<Gravitable3f>(entity);
            my_world.patch<Velocity3f>(entity, [&gravity, &dt_secs](auto &vel) {
                vel.component += gravity.component * static_cast<double>(dt_secs);
            });
        }
    }

    auto on_time_elapsed_render(const action::Render<Render::Layout::SCENE> &e) -> void;
};

} // namespace kawe
