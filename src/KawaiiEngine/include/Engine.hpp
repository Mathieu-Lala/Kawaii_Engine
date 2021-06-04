#pragma once

#define TINYOBJLOADER_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION

#include <memory>
#include <thread>

#include <spdlog/spdlog.h>
#include <glm/vec4.hpp>

#include <entt/entt.hpp>

#include "helpers/overloaded.hpp"
#include "graphics/Window.hpp"
#include "EventProvider.hpp"
#include "Event.hpp"
#include "graphics/Shader.hpp"
#include "component.hpp"
#include "Camera.hpp"
#include "State.hpp"

#include "resources/ResourceLoader.hpp"

#include "widgets/ComponentInspector.hpp"
#include "widgets/EntityHierarchy.hpp"


using namespace std::chrono_literals;

namespace kawe {

class Engine {
public:
    Engine() : entity_hierarchy{component_inspector.selected}
    {
        spdlog::set_level(spdlog::level::trace);

        constexpr auto KAWE_GLFW_MAJOR = 4;
        constexpr auto KAWE_GLFW_MINOR = 5;

        glfwSetErrorCallback([](int code, const char *message) {
            spdlog::error("[GLFW] An error occured '{}' 'code={}'\n", message, code);
        });

        if (glfwInit() == GLFW_FALSE) { return; }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, KAWE_GLFW_MAJOR);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, KAWE_GLFW_MINOR);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

        spdlog::trace("[GLFW] Version: '{}'\n", glfwGetVersionString());

        window = std::make_unique<Window>(events, "test", glm::ivec2{1080, 800});
        glfwMakeContextCurrent(window->get());

        if (const auto err = glewInit(); err != GLEW_OK) {
            spdlog::error("[GLEW] An error occured '{}' 'code={}'", glewGetErrorString(err), err);
            return;
        }

        IMGUI_CHECKVERSION();

        auto imgui_ctx = ImGui::CreateContext();
        if (!imgui_ctx) { return; }

        if (!::ImGui_ImplGlfw_InitForOpenGL(window->get(), false)) { return; }

        if (!::ImGui_ImplOpenGL3_Init()) { return; }

        glfwSwapInterval(0);

        world.on_destroy<Render::VAO>().connect<Render::VAO::on_destroy>();
        world.on_destroy<Render::VBO<Render::VAO::Attribute::POSITION>>()
            .connect<Render::VBO<Render::VAO::Attribute::POSITION>::on_destroy>();
        world.on_destroy<Render::VBO<Render::VAO::Attribute::COLOR>>()
            .connect<Render::VBO<Render::VAO::Attribute::COLOR>::on_destroy>();
        world.on_destroy<Render::EBO>().connect<Render::EBO::on_destroy>();

        const auto update_aabb = [](entt::registry &reg, entt::entity e) -> void {
            if (const auto collider = reg.try_get<Collider>(e); collider != nullptr) {
                if (const auto vbo = reg.try_get<Render::VBO<Render::VAO::Attribute::POSITION>>(e);
                    vbo != nullptr) {
                    AABB::emplace(reg, e, vbo->vertices);
                }
            }
        };

        world.on_construct<Position3f>().connect<update_aabb>();
        world.on_construct<Position3f>().connect<update_aabb>();
        world.on_construct<Position3f>().connect<update_aabb>();

        world.on_update<Position3f>().connect<update_aabb>();
        world.on_update<Rotation3f>().connect<update_aabb>();
        world.on_update<Scale3f>().connect<update_aabb>();

        world.on_construct<Collider>().connect<update_aabb>();

        world.on_construct<Render::VBO<Render::VAO::Attribute::POSITION>>().connect<update_aabb>();
        world.on_update<Render::VBO<Render::VAO::Attribute::POSITION>>().connect<update_aabb>();

        // AABB alogrithm = really simple and fast collision detection
        const auto check_collision = [](entt::registry &reg, entt::entity e) -> void {
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

                    {
                        // todo : this should be wrapped in a helper function ?
                        const auto &vbo_color = reg.get<Render::VBO<Render::VAO::Attribute::COLOR>>(aabb.guizmo);
                        std::vector<float> color_red{};
                        for (auto i = 0ul; i != vbo_color.vertices.size(); i += vbo_color.stride_size) {
                            color_red.emplace_back(1.0f); // r
                            color_red.emplace_back(0.0f); // g
                            color_red.emplace_back(0.0f); // b
                            color_red.emplace_back(1.0f); // a
                        }
                        Render::VBO<Render::VAO::Attribute::COLOR>::emplace(reg, aabb.guizmo, color_red, 4);
                    }

                    {
                        // todo : this should be wrapped in a helper function ?
                        const auto &vbo_color =
                            reg.get<Render::VBO<Render::VAO::Attribute::COLOR>>(other_aabb.guizmo);
                        std::vector<float> color_red{};
                        for (auto i = 0ul; i != vbo_color.vertices.size(); i += vbo_color.stride_size) {
                            color_red.emplace_back(1.0f); // r
                            color_red.emplace_back(0.0f); // g
                            color_red.emplace_back(0.0f); // b
                            color_red.emplace_back(1.0f); // a
                        }
                        Render::VBO<Render::VAO::Attribute::COLOR>::emplace(reg, other_aabb.guizmo, color_red, 4);
                    }
                }
            }

            // todo : this logic should be done on another signal named on_collision_resolved ...
            const auto collider = reg.get<Collider>(e);
            if (!has_aabb_collision) {
                if (collider.step != Collider::CollisionStep::NONE) {
                    reg.patch<Collider>(e, [](auto &c) { c.step = Collider::CollisionStep::NONE; });

                    // todo : this should be wrapped in a helper function ?
                    const auto &vbo_color = reg.get<Render::VBO<Render::VAO::Attribute::COLOR>>(aabb.guizmo);
                    std::vector<float> color_black{};
                    for (auto i = 0ul; i != vbo_color.vertices.size(); i += vbo_color.stride_size) {
                        color_black.emplace_back(0.0f); // r
                        color_black.emplace_back(0.0f); // g
                        color_black.emplace_back(0.0f); // b
                        color_black.emplace_back(1.0f); // a
                    }
                    Render::VBO<Render::VAO::Attribute::COLOR>::emplace(reg, aabb.guizmo, color_black, 4);
                }
            }
        };

        world.on_construct<AABB>().connect<check_collision>();
        world.on_update<AABB>().connect<check_collision>();

        world.set<entt::dispatcher *>(&dispatcher);
        world.set<ResourceLoader *>(&loader);
        state = std::make_unique<State>(*window);
        world.set<State *>(state.get());
    }

    ~Engine()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();

        // ImGui::DestroyContext(imgui_ctx);

        glfwSetErrorCallback(nullptr);
        glfwTerminate();
    }

    std::function<void()> on_imgui;
    std::function<void(entt::registry &)> on_create;

    auto start()
    {
        CALL_OPEN_GL(::glEnable(GL_DEPTH_TEST));
        CALL_OPEN_GL(::glEnable(GL_BLEND));
        CALL_OPEN_GL(::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

        on_create(world);

        while (state->is_running && !window->should_close()) {
            const auto event = events.getNextEvent();

            std::visit(
                kawe::overloaded{
                    [&](const kawe::OpenWindow &) {
                        events.setCurrentTimepoint(std::chrono::steady_clock::now());
                    },
                    [&](const kawe::CloseWindow &) { state->is_running = false; },
                    [&](const kawe::Moved<kawe::Mouse> &mouse) {
                        state->mouse_pos = {mouse.source.x, mouse.source.y};
                        dispatcher.trigger<kawe::Moved<kawe::Mouse>>(mouse);
                    },
                    [&](const kawe::Pressed<kawe::MouseButton> &e) {
                        window->useEvent(e);
                        state->state_mouse_button[e.source.button] = true;
                        state->mouse_pos_when_pressed = {e.source.mouse.x, e.source.mouse.y};
                        dispatcher.trigger<kawe::Pressed<kawe::MouseButton>>(e);
                    },
                    [&](const kawe::Released<kawe::MouseButton> &e) {
                        window->useEvent(e);
                        state->state_mouse_button[e.source.button] = false;
                        dispatcher.trigger<kawe::Released<kawe::MouseButton>>(e);
                    },
                    [&](const kawe::Pressed<kawe::Key> &e) {
                        window->useEvent(e);
                        state->keyboard_state[e.source.keycode] = true;
                        dispatcher.trigger<kawe::Pressed<kawe::Key>>(e);
                    },
                    [&](const kawe::Released<kawe::Key> &e) {
                        window->useEvent(e);
                        state->keyboard_state[e.source.keycode] = false;
                        dispatcher.trigger<kawe::Released<kawe::Key>>(e);
                    },
                    [&](const kawe::Character &e) {
                        window->useEvent(e);
                        dispatcher.trigger<kawe::Character>(e);
                    },
                    [&](const kawe::TimeElapsed &e) { on_time_elapsed(e); },
                    [](const auto &) {}},
                event);
        }

        world.clear();
    }

private:
    EventProvider events;
    std::unique_ptr<Window> window;

    ResourceLoader loader;
    entt::dispatcher dispatcher;
    entt::registry world;

    ComponentInspector component_inspector;
    EntityHierarchy entity_hierarchy;

    std::unique_ptr<State> state;

    auto on_time_elapsed(const kawe::TimeElapsed &e) -> void
    {
        const auto dt_nano = e.elapsed;
        const auto dt_secs =
            static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(dt_nano).count())
            / 1'000'000.0;

        { // note : camera logics should be a system // should be trigger by a signal
            if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow)) {
                for (auto &camera : state->camera) {
                    const auto viewport = camera.getViewport();
                    const auto window_size = window->getSize<double>();

                    if (!Rect4<double>{
                            static_cast<double>(viewport.x) * window_size.x,
                            static_cast<double>(viewport.y) * window_size.y,
                            static_cast<double>(viewport.w) * window_size.x,
                            static_cast<double>(viewport.h) * window_size.y}
                             .contains(state->mouse_pos)) {
                        continue;
                    }

                    for (const auto &[button, pressed] : state->state_mouse_button) {
                        if (!pressed) { continue; }
                        camera.handleMouseInput(button, state->mouse_pos, state->mouse_pos_when_pressed, dt_secs);
                    }
                }
            }
        }

        for (const auto &entity : world.view<Position3f, Velocity3f>()) {
            const auto &vel = world.get<Velocity3f>(entity);
            world.patch<Position3f>(entity, [&vel, &dt_secs](auto &pos) {
                pos.component += vel.component * static_cast<double>(dt_secs);
            });
        }

        for (const auto &entity : world.view<Gravitable3f, Velocity3f>()) {
            const auto &gravity = world.get<Gravitable3f>(entity);
            world.patch<Velocity3f>(entity, [&gravity, &dt_secs](auto &vel) {
                vel.component += gravity.component * static_cast<double>(dt_secs);
            });
        }

        dispatcher.trigger<kawe::TimeElapsed>(e);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::ShowDemoWindow();

        on_imgui();

        entity_hierarchy.draw(world);
        component_inspector.draw(world);

        ImGui::Render();

        glClearColor(state->clear_color.r, state->clear_color.g, state->clear_color.b, state->clear_color.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for (auto &i : state->camera) { system_rendering(i, state->shader); }

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window->get());
    }

    // todo : this should not take a shader as argument
    auto system_rendering(Camera &camera, Shader &shader) -> void
    {
        // todo : when resizing the window, the object deform
        // this doesn t sound kind right ...
        const auto viewport = camera.getViewport();
        const auto window_size = window->getSize<float>();
        ::glViewport(
            static_cast<GLint>(viewport.x * window_size.x),
            static_cast<GLint>(viewport.y * window_size.y),
            static_cast<GLsizei>(viewport.w * window_size.x),
            static_cast<GLsizei>(viewport.h * window_size.y));

        shader.setUniform("view", camera.getView());
        shader.setUniform("projection", camera.getProjection());

        const auto render =
            [&shader]<bool has_ebo>(
                const Render::VAO &vao, const Position3f &pos, const Rotation3f &rot, const Scale3f &scale) {
                auto model = glm::dmat4(1.0);
                model = glm::translate(model, pos.component);
                model = glm::rotate(model, glm::radians(rot.component.x), glm::dvec3(1.0, 0.0, 0.0));
                model = glm::rotate(model, glm::radians(rot.component.y), glm::dvec3(0.0, 1.0, 0.0));
                model = glm::rotate(model, glm::radians(rot.component.z), glm::dvec3(0.0, 0.0, 1.0));
                model = glm::scale(model, scale.component);
                shader.setUniform("model", model);

                CALL_OPEN_GL(::glBindVertexArray(vao.object));
                if constexpr (has_ebo) {
                    CALL_OPEN_GL(::glDrawElements(static_cast<GLenum>(vao.mode), vao.count, GL_UNSIGNED_INT, 0));
                } else {
                    CALL_OPEN_GL(::glDrawArrays(static_cast<GLenum>(vao.mode), 0, vao.count));
                }
            };

        // note : it should be a better way..
        // todo create a matrix of callback templated somthing something

        // without ebo

        world.view<Render::VAO>(entt::exclude<Render::EBO, Position3f, Rotation3f, Scale3f>)
            .each([&render](const auto &vao) {
                render.operator()<false>(
                    vao, {Position3f::default_value}, {Rotation3f::default_value}, {Scale3f::default_value});
            });

        world.view<Render::VAO, Position3f>(entt::exclude<Render::EBO, Rotation3f, Scale3f>)
            .each([&render](const auto &vao, const auto &pos) {
                render.operator()<false>(vao, pos, {Rotation3f::default_value}, {Scale3f::default_value});
            });

        world.view<Render::VAO, Rotation3f>(entt::exclude<Render::EBO, Position3f, Scale3f>)
            .each([&render](const auto &vao, const auto &rot) {
                render.operator()<false>(vao, {Position3f::default_value}, rot, {Scale3f::default_value});
            });

        world.view<Render::VAO, Scale3f>(entt::exclude<Render::EBO, Position3f, Rotation3f>)
            .each([&render](const auto &vao, const auto &scale) {
                render.operator()<false>(vao, {Position3f::default_value}, {Rotation3f::default_value}, scale);
            });

        world.view<Render::VAO, Position3f, Scale3f>(entt::exclude<Render::EBO, Rotation3f>)
            .each([&render](const auto &vao, const auto &pos, const auto &scale) {
                render.operator()<false>(vao, pos, {Rotation3f::default_value}, scale);
            });

        world.view<Render::VAO, Rotation3f, Scale3f>(entt::exclude<Render::EBO, Position3f>)
            .each([&render](const auto &vao, const auto &rot, const auto &scale) {
                render.operator()<false>(vao, {Position3f::default_value}, rot, scale);
            });

        world.view<Render::VAO, Position3f, Rotation3f>(entt::exclude<Render::EBO, Scale3f>)
            .each([&render](const auto &vao, const auto &pos, const auto &rot) {
                render.operator()<false>(vao, pos, rot, {Scale3f::default_value});
            });

        world.view<Render::VAO, Position3f, Rotation3f, Scale3f>(entt::exclude<Render::EBO>)
            .each([&render](const auto &vao, const auto &pos, const auto &rot, const auto &scale) {
                render.operator()<false>(vao, pos, rot, scale);
            });

        // with ebo

        world.view<Render::EBO, Render::VAO>(entt::exclude<Position3f, Rotation3f, Scale3f>)
            .each([&render](const auto &, const auto &vao) {
                render.operator()<true>(
                    vao, {Position3f::default_value}, {Rotation3f::default_value}, {Scale3f::default_value});
            });

        world.view<Render::EBO, Render::VAO, Position3f>(entt::exclude<Rotation3f, Scale3f>)
            .each([&render](const auto &, const auto &vao, const auto &pos) {
                render.operator()<true>(vao, pos, {Rotation3f::default_value}, {Scale3f::default_value});
            });

        world.view<Render::EBO, Render::VAO, Rotation3f>(entt::exclude<Position3f, Scale3f>)
            .each([&render](const auto &, const auto &vao, const auto &rot) {
                render.operator()<true>(vao, {Position3f::default_value}, rot, {Scale3f::default_value});
            });

        world.view<Render::EBO, Render::VAO, Scale3f>(entt::exclude<Position3f, Rotation3f>)
            .each([&render](const auto &, const auto &vao, const auto &scale) {
                render.operator()<true>(vao, {Position3f::default_value}, {Rotation3f::default_value}, scale);
            });

        world.view<Render::EBO, Render::VAO, Position3f, Scale3f>(entt::exclude<Rotation3f>)
            .each([&render](const auto &, const auto &vao, const auto &pos, const auto &scale) {
                render.operator()<true>(vao, pos, {Rotation3f::default_value}, scale);
            });

        world.view<Render::EBO, Render::VAO, Rotation3f, Scale3f>(entt::exclude<Position3f>)
            .each([&render](const auto &, const auto &vao, const auto &rot, const auto &scale) {
                render.operator()<true>(vao, {Position3f::default_value}, rot, scale);
            });

        world.view<Render::EBO, Render::VAO, Position3f, Rotation3f>(entt::exclude<Scale3f>)
            .each([&render](const auto &, const auto &vao, const auto &pos, const auto &rot) {
                render.operator()<true>(vao, pos, rot, {Scale3f::default_value});
            });

        world.view<Render::EBO, Render::VAO, Position3f, Rotation3f, Scale3f>().each(
            [&render](const auto &, const auto &vao, const auto &pos, const auto &rot, const auto &scale) {
                render.operator()<true>(vao, pos, rot, scale);
            });
    }
}; // namespace kawe

} // namespace kawe
