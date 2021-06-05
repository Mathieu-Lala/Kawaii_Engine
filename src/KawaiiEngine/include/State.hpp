#pragma once

#include <string_view>
#include <unordered_map>

#include "resources/ResourceLoader.hpp"
#include "graphics/Window.hpp"
#include "graphics/Shader.hpp"
#include "Camera.hpp"

#include "Event.hpp"

namespace kawe {

struct State {
    State(const Window &window)
    {
        ResourceLoader loader;

        default_vert_shader = loader.load<Shader>("./asset/shader/default.vert");
        default_frag_shader = loader.load<Shader>("./asset/shader/default.frag");

        if (!default_vert_shader || !default_frag_shader) {
            spdlog::error("Failed initializing engine: couldn't load 'default.vert' or 'default.frag' shaders.");
            return;
        }

        default_shader_program = std::make_unique<ShaderProgram>(
            std::vector<uint32_t> { default_vert_shader->shader_id, default_frag_shader->shader_id }
        );

        default_shader_program->use();

        camera.emplace_back(window, glm::vec3{5.0f, 5.0f, 5.0f}, Rect4<float>{0.0f, 0.0f, 0.5f, 1.0f});
        camera.emplace_back(window, glm::vec3{5.0f, 5.0f, 5.0f}, Rect4<float>{0.5f, 0.0f, 0.5f, 1.0f});

        for (const auto &i : magic_enum::enum_values<MouseButton::Button>()) {
            state_mouse_button[i] = false;
        }
        for (const auto &i : magic_enum::enum_values<Key::Code>()) { keyboard_state[i] = false; }
    }

    // shaders.
    std::shared_ptr<Shader> default_vert_shader;
    std::shared_ptr<Shader> default_frag_shader;
    std::unique_ptr<ShaderProgram> default_shader_program;

    // cameras.
    std::vector<Camera> camera;

    // inputs.
    glm::dvec2 mouse_pos{};
    glm::dvec2 mouse_pos_when_pressed{};

    std::unordered_map<MouseButton::Button, bool> state_mouse_button;
    std::unordered_map<Key::Code, bool> keyboard_state;

    bool is_running = true;
};

} // namespace kawe
