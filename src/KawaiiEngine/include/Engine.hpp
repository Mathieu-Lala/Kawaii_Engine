#pragma once

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
#include "resources/ResourceLoader.hpp"

using namespace std::chrono_literals;

namespace kawe {

class Engine {
public:
    Engine()
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

        world.set<entt::dispatcher *>(&dispatcher);
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
        constexpr auto VERT_SH = R"(#version 450
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec4 inColors;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec4 fragColors;

void main()
{
    gl_Position = projection * view * model * vec4(inPos, 1.0f);
    fragColors = inColors;
}
)";

        constexpr auto FRAG_SH = R"(#version 450
in vec4 fragColors;
out vec4 FragColor;

void main()
{
    FragColor = fragColors;
}
)";

        CALL_OPEN_GL(::glEnable(GL_DEPTH_TEST));
        CALL_OPEN_GL(::glEnable(GL_BLEND));
        CALL_OPEN_GL(::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

        Shader shader(VERT_SH, FRAG_SH);
        shader.use();

        Camera camera{*window, glm::vec3{5, 5, 5}};


        bool is_running = true;

        glm::dvec2 mouse_pos{};
        glm::dvec2 mouse_pos_when_pressed{};

        std::unordered_map<MouseButton::Button, bool> state_mouse_button;
        for (const auto &i : magic_enum::enum_values<MouseButton::Button>()) {
            state_mouse_button[i] = false;
        }

        std::unordered_map<Key::Code, bool> keyboard_state;
        for (const auto &i : magic_enum::enum_values<Key::Code>()) { keyboard_state[i] = false; }

        on_create(world);

        while (is_running && !window->should_close()) {
            const auto event = events.getNextEvent();

            std::visit(
                kawe::overloaded{
                    [&](const kawe::OpenWindow &) {
                        events.setCurrentTimepoint(std::chrono::steady_clock::now());
                    },
                    [&](const kawe::CloseWindow &) { is_running = false; },
                    [&](const kawe::Moved<kawe::Mouse> &mouse) {
                        mouse_pos = {mouse.source.x, mouse.source.y};
                        dispatcher.trigger<kawe::Moved<kawe::Mouse>>(mouse);
                    },
                    [&](const kawe::Pressed<kawe::MouseButton> &e) {
                        window->useEvent(e);
                        state_mouse_button[e.source.button] = true;
                        mouse_pos_when_pressed = {e.source.mouse.x, e.source.mouse.y};
                        dispatcher.trigger<kawe::Pressed<kawe::MouseButton>>(e);
                    },
                    [&](const kawe::Released<kawe::MouseButton> &e) {
                        window->useEvent(e);
                        state_mouse_button[e.source.button] = false;
                        dispatcher.trigger<kawe::Released<kawe::MouseButton>>(e);
                    },
                    [&](const kawe::Pressed<kawe::Key> &e) {
                        window->useEvent(e);
                        keyboard_state[e.source.keycode] = true;
                        dispatcher.trigger<kawe::Pressed<kawe::Key>>(e);
                    },
                    [&](const kawe::Released<kawe::Key> &e) {
                        window->useEvent(e);
                        keyboard_state[e.source.keycode] = false;
                        dispatcher.trigger<kawe::Released<kawe::Key>>(e);
                    },
                    [&](const kawe::Character &e) {
                        window->useEvent(e);
                        dispatcher.trigger<kawe::Character>(e);
                    },
                    [&](const kawe::TimeElapsed &e) {
                        const auto dt_nano = std::get<TimeElapsed>(event).elapsed;

                        spdlog::trace("dt nano: {}", dt_nano.count());

                        if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow)) {
                            for (const auto &[button, pressed] : state_mouse_button) {
                                if (pressed) {
                                    camera.handleMouseInput(button, mouse_pos, mouse_pos_when_pressed, dt_nano);
                                }
                            }
                        }

                        if (camera.hasChanged<Camera::Matrix::VIEW>()) {
                            const auto view =
                                glm::lookAt(camera.getPosition(), camera.getTargetCenter(), camera.getUp());
                            shader.setUniform("view", view);
                            camera.setChangedFlag<Camera::Matrix::VIEW>(false);
                        }
                        if (camera.hasChanged<Camera::Matrix::PROJECTION>()) {
                            const auto projection = camera.getProjection();
                            shader.setUniform("projection", projection);
                            camera.setChangedFlag<Camera::Matrix::PROJECTION>(false);
                        }

                        dispatcher.trigger<kawe::TimeElapsed>(e);

                        ImGui_ImplOpenGL3_NewFrame();
                        ImGui_ImplGlfw_NewFrame();
                        ImGui::NewFrame();

                        ImGui::ShowDemoWindow();

                        on_imgui();

                        ImGui::Render();

                        constexpr auto CLEAR_COLOR = glm::vec4{0.0f, 1.0f, 0.2f, 1.0f};

                        glClearColor(CLEAR_COLOR.r, CLEAR_COLOR.g, CLEAR_COLOR.b, CLEAR_COLOR.a);
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                        system_rendering(shader);

                        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

                        glfwSwapBuffers(window->get());
                    },
                    [](const auto &) {}},
                event);
        }
    }

private:
    EventProvider events;
    std::unique_ptr<Window> window;

    entt::dispatcher dispatcher;
    entt::registry world;

    auto system_rendering(Shader &shader) -> void
    {
        const auto render = [&shader]<bool has_ebo>(
                                const VAO &vao, const Position3f &pos, const Rotation3f &rot, const Scale3f &scale) {
            auto model = glm::mat4(1.0f);
            model = glm::translate(model, pos.vec);
            model = glm::rotate(model, glm::radians(rot.vec.x), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(rot.vec.y), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(rot.vec.z), glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, scale.vec);
            shader.setUniform("model", model);

            CALL_OPEN_GL(::glBindVertexArray(vao.object));
            if constexpr (has_ebo) {
                CALL_OPEN_GL(::glDrawElements(static_cast<GLenum>(vao.mode), vao.count, GL_UNSIGNED_INT, 0));
            } else {
                CALL_OPEN_GL(::glDrawArrays(static_cast<GLenum>(vao.mode), 0, vao.count));
            }
        };

        [[maybe_unused]] static constexpr auto NO_POSITION = glm::vec3{0.0f, 0.0f, 0.0f};
        [[maybe_unused]] static constexpr auto NO_ROTATION = glm::vec3{0.0f, 0.0f, 0.0f};
        [[maybe_unused]] static constexpr auto NO_SCALE = glm::vec3{1.0f, 1.0f, 1.0f};

        // note : it should be a better way..
        // todo create a matrix of callback templated somthing something

        // without ebo

        world.view<VAO>(entt::exclude<EBO, Position3f, Rotation3f, Scale3f>).each([&render](const auto &vao) {
            render.operator()<false>(vao, {NO_POSITION}, {NO_ROTATION}, {NO_SCALE});
        });

        world.view<VAO, Position3f>(entt::exclude<EBO, Rotation3f, Scale3f>)
            .each([&render](const auto &vao, const auto &pos) {
                render.operator()<false>(vao, pos, {NO_ROTATION}, {NO_SCALE});
            });

        world.view<VAO, Rotation3f>(entt::exclude<EBO, Position3f, Scale3f>)
            .each([&render](const auto &vao, const auto &rot) {
                render.operator()<false>(vao, {NO_POSITION}, rot, {NO_SCALE});
            });

        world.view<VAO, Scale3f>(entt::exclude<EBO, Position3f, Rotation3f>)
            .each([&render](const auto &vao, const auto &scale) {
                render.operator()<false>(vao, {NO_POSITION}, {NO_ROTATION}, scale);
            });

        world.view<VAO, Position3f, Scale3f>(entt::exclude<EBO, Rotation3f>)
            .each([&render](const auto &vao, const auto &pos, const auto &scale) {
                render.operator()<false>(vao, pos, {NO_ROTATION}, scale);
            });

        world.view<VAO, Rotation3f, Scale3f>(entt::exclude<EBO, Position3f>)
            .each([&render](const auto &vao, const auto &rot, const auto &scale) {
                render.operator()<false>(vao, {NO_POSITION}, rot, scale);
            });

        world.view<VAO, Position3f, Rotation3f>(entt::exclude<EBO, Scale3f>)
            .each([&render](const auto &vao, const auto &pos, const auto &rot) {
                render.operator()<false>(vao, pos, rot, {NO_SCALE});
            });

        world.view<VAO, Position3f, Rotation3f, Scale3f>(entt::exclude<EBO>)
            .each([&render](const auto &vao, const auto &pos, const auto &rot, const auto &scale) {
                render.operator()<false>(vao, pos, rot, scale);
            });

        // with ebo

        world.view<EBO, VAO>(entt::exclude<Position3f, Rotation3f, Scale3f>)
            .each([&render](const auto &, const auto &vao) {
                render.operator()<true>(vao, {NO_POSITION}, {NO_ROTATION}, {NO_SCALE});
            });

        world.view<EBO, VAO, Position3f>(entt::exclude<Rotation3f, Scale3f>)
            .each([&render](const auto &, const auto &vao, const auto &pos) {
                render.operator()<true>(vao, pos, {NO_ROTATION}, {NO_SCALE});
            });

        world.view<EBO, VAO, Rotation3f>(entt::exclude<Position3f, Scale3f>)
            .each([&render](const auto &, const auto &vao, const auto &rot) {
                render.operator()<true>(vao, {NO_POSITION}, rot, {NO_SCALE});
            });

        world.view<EBO, VAO, Scale3f>(entt::exclude<Position3f, Rotation3f>)
            .each([&render](const auto &, const auto &vao, const auto &scale) {
                render.operator()<true>(vao, {NO_POSITION}, {NO_ROTATION}, scale);
            });

        world.view<EBO, VAO, Position3f, Scale3f>(entt::exclude<Rotation3f>)
            .each([&render](const auto &, const auto &vao, const auto &pos, const auto &scale) {
                render.operator()<true>(vao, pos, {NO_ROTATION}, scale);
            });

        world.view<EBO, VAO, Rotation3f, Scale3f>(entt::exclude<Position3f>)
            .each([&render](const auto &, const auto &vao, const auto &rot, const auto &scale) {
                render.operator()<true>(vao, {NO_POSITION}, rot, scale);
            });

        world.view<EBO, VAO, Position3f, Rotation3f>(entt::exclude<Scale3f>)
            .each([&render](const auto &, const auto &vao, const auto &pos, const auto &rot) {
                render.operator()<true>(vao, pos, rot, {NO_SCALE});
            });

        world.view<EBO, VAO, Position3f, Rotation3f, Scale3f>().each(
            [&render](const auto &, const auto &vao, const auto &pos, const auto &rot, const auto &scale) {
                render.operator()<true>(vao, pos, rot, scale);
            });
    }
};

} // namespace kawe
