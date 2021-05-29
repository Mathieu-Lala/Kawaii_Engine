#pragma once

#include <memory>

#include <spdlog/spdlog.h>
#include <glm/vec4.hpp>

#include <entt/entt.hpp>

#include "helpers/overloaded.hpp"
#include "graphics/Window.hpp"
#include "EventProvider.hpp"
#include "Event.hpp"
#include "graphics/Shader.hpp"

namespace kawe {

class Engine {
public:
    Engine()
    {
        constexpr auto KAWE_GLFW_MAJOR = 3;
        constexpr auto KAWE_GLFW_MINOR = 3;

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
    }

    ~Engine()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();

        // ImGui::DestroyContext(imgui_ctx);

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

        bool is_running = true;
        glm::ivec2 mouse_pos{};

        on_create(world);

        while (is_running && !window->should_close()) {
            // TODO : remove me
            bool timeElapsed = false;

            const auto event = events.getNextEvent();

            std::visit(
                kawe::overloaded{
                    [&](const kawe::OpenWindow &) {
                        events.setCurrentTimepoint(std::chrono::steady_clock::now());
                    },
                    [&](const kawe::CloseWindow &) { is_running = false; },

                    [&mouse_pos](const kawe::Moved<kawe::Mouse> &mouse) {
                        mouse_pos = {mouse.source.x, mouse.source.y};
                    },
                    [&](const kawe::Pressed<kawe::MouseButton> &e) {
                        window->useEvent(e);

                        // state_mouse_button[static_cast<std::size_t>(magic_enum::enum_integer(e.source.button))]
                        // = true; mouse_pos_when_pressed = {e.source.mouse.x, e.source.mouse.y};
                    },
                    [&](const kawe::Released<kawe::MouseButton> &e) {
                        window->useEvent(e);

                        // state_mouse_button[static_cast<std::size_t>(magic_enum::enum_integer(e.source.button))]
                        // = false;
                    },
                    [&](const kawe::Pressed<kawe::Key> &e) {
                        window->useEvent(e);
                        // keyboard_state[e.source.keycode] = true;
                    },
                    [&](const kawe::Released<kawe::Key> &e) {
                        window->useEvent(e);
                        // keyboard_state[e.source.keycode] = false;
                    },
                    [&](const kawe::Character &e) { window->useEvent(e); },
                    [&timeElapsed](const kawe::TimeElapsed &) { timeElapsed = true; },
                    [](const auto &) {}},
                event);


            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ImGui::ShowDemoWindow();

            on_imgui();

            ImGui::Render();

            constexpr auto CLEAR_COLOR = glm::vec4{0.0f, 1.0f, 0.2f, 1.0f};

            glClearColor(CLEAR_COLOR.r, CLEAR_COLOR.g, CLEAR_COLOR.b, CLEAR_COLOR.a);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            system_rendering();

            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(window->get());
        }
    }

private:
    EventProvider events;
    std::unique_ptr<Window> window;

    entt::registry world;

    auto system_rendering() -> void {}
};

} // namespace kawe
