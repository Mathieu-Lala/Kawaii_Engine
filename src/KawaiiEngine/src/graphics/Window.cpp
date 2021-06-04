#include "graphics/Window.hpp"
#include "EventProvider.hpp"

kawe::Window::Window(
    EventProvider &events, const std::string_view window_title, glm::ivec2 &&size, glm::ivec2 &&position, bool isFullscreen)
{
    if (isFullscreen) {
        GLFWmonitor *primary = glfwGetPrimaryMonitor();
        const GLFWvidmode *mode = glfwGetVideoMode(primary);

        window = glfwCreateWindow(mode->width, mode->height, window_title.data(), primary, nullptr);
        spdlog::trace("Monitor size: {{ .width: {}, .height }}", mode->width, mode->height);
    } else {
        window = glfwCreateWindow(size.x, size.y, window_title.data(), nullptr, nullptr);
        glfwSetWindowPos(window, position.x, position.y);
        spdlog::trace("Window size: {{ .width: {}, .height }}", size.x, size.y);
        spdlog::trace("Window position: {{ .x: {}, .y }}", position.x, position.y);
    }

    if (!window) {
        spdlog::error("Window creation failed");
    } else {
        spdlog::trace("Window created");
    }

    events.assign(*this);

    isWindowIconofied = glfwGetWindowAttrib(window, GLFW_ICONIFIED);
    isWindowFocused = glfwGetWindowAttrib(window, GLFW_FOCUSED);

    // current_window_size = get_window_size();
}

template<>
auto kawe::Window::useEvent(const Pressed<MouseButton> &m) -> void
{
    ::ImGui_ImplGlfw_MouseButtonCallback(
        window, magic_enum::enum_integer(m.source.button), GLFW_PRESS, 0 /* todo */);
}

template<>
auto kawe::Window::useEvent(const Released<MouseButton> &m) -> void
{
    ::ImGui_ImplGlfw_MouseButtonCallback(
        window, magic_enum::enum_integer(m.source.button), GLFW_RELEASE, 0 /* todo */);
}

template<>
auto kawe::Window::useEvent(const Pressed<Key> &k) -> void
{
    ::ImGui_ImplGlfw_KeyCallback(
        window, static_cast<int>(k.source.keycode), k.source.scancode, GLFW_PRESS, 0 /* todo */);
}

template<>
auto kawe::Window::useEvent(const Released<Key> &k) -> void
{
    ::ImGui_ImplGlfw_KeyCallback(
        window, static_cast<int>(k.source.keycode), k.source.scancode, GLFW_RELEASE, 0 /* todo */);
}

template<>
auto kawe::Window::useEvent(const Character &character) -> void
{
    ::ImGui_ImplGlfw_CharCallback(window, character.codepoint);
}
