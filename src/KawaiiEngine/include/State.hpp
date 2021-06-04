#pragma once

#include <string_view>
#include <unordered_map>

#include "graphics/Window.hpp"
#include "graphics/Shader.hpp"
#include "Camera.hpp"

#include "Event.hpp"

namespace kawe {

struct State {
    static constexpr std::string_view VERT_SH = R"(#version 450
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

    static constexpr std::string_view FRAG_SH = R"(#version 450
in vec4 fragColors;
out vec4 FragColor;

void main()
{
    FragColor = fragColors;
}
)";

    State(const Window &window) : shader{VERT_SH, FRAG_SH}
    {
        camera.emplace_back(window, glm::vec3{5.0f, 5.0f, 5.0f}, Rect4<float>{0.0f, 0.0f, 0.5f, 1.0f});
        camera.emplace_back(window, glm::vec3{5.0f, 5.0f, 5.0f}, Rect4<float>{0.5f, 0.0f, 0.5f, 1.0f});

        shader.use();

        for (const auto &i : magic_enum::enum_values<MouseButton::Button>()) {
            state_mouse_button[i] = false;
        }
        for (const auto &i : magic_enum::enum_values<Key::Code>()) { keyboard_state[i] = false; }
    }

    Shader shader;
    std::vector<Camera> camera;

    bool is_running = true;

    glm::dvec2 mouse_pos{};
    glm::dvec2 mouse_pos_when_pressed{};

    std::unordered_map<MouseButton::Button, bool> state_mouse_button;
    std::unordered_map<Key::Code, bool> keyboard_state;
};

} // namespace kawe
