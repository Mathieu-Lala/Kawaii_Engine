#pragma once

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <spdlog/spdlog.h>
#include <string_view>
#include <glm/vec2.hpp>

namespace kawe {

class Window {
public:
    Window(const std::string_view window_title, glm::ivec2 &&size, glm::ivec2 &&position = {0, 0}, bool isFullscreen = false);

    auto get() -> GLFWwindow * { return window; }

private:
    GLFWwindow *window;

    bool isWindowIconofied;
    bool isWindowFocused;
};

} // namespace kawe
