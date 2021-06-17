#include <stb_image_write.h>

#include "graphics/Window.hpp"
#include "EventProvider.hpp"
#include "helpers/macro.hpp"

kawe::Window::Window(const std::string_view window_title, glm::ivec2 &&size, glm::ivec2 &&position, bool isFullscreen)
{
    spdlog::debug("starting window initialiser.");

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


    isWindowIconofied = glfwGetWindowAttrib(window, GLFW_ICONIFIED);
    isWindowFocused = glfwGetWindowAttrib(window, GLFW_FOCUSED);

    spdlog::debug("window initialized.");

    // current_window_size = get_window_size();
}

auto kawe::Window::screenshot(const std::string_view filename) -> bool
{
    GLint viewport[4];

    CALL_OPEN_GL(::glGetIntegerv(GL_VIEWPORT, viewport));
    const auto &x = viewport[0];
    const auto &y = viewport[1];
    const auto &width = viewport[2];
    const auto &height = viewport[3];

    constexpr auto CHANNEL = 4ul;
    std::vector<char> pixels(static_cast<std::size_t>(width * height) * CHANNEL);

    CALL_OPEN_GL(::glPixelStorei(GL_PACK_ALIGNMENT, 1));
    CALL_OPEN_GL(::glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data()));

    std::array<char, CHANNEL> pixel;
    for (auto j = 0; j < height / 2; ++j)
        for (auto i = 0; i < width; ++i) {
            const auto top = static_cast<std::size_t>(i + j * width) * pixel.size();
            const auto bottom = static_cast<std::size_t>(i + (height - j - 1) * width) * pixel.size();

            std::memcpy(pixel.data(), pixels.data() + top, pixel.size());
            std::memcpy(pixels.data() + top, pixels.data() + bottom, pixel.size());
            std::memcpy(pixels.data() + bottom, pixel.data(), pixel.size());
        }

    return !!::stbi_write_png(filename.data(), width, height, 4, pixels.data(), 0);
}

template<>
auto kawe::Window::useEvent(const event::Pressed<event::MouseButton> &m) -> void
{
    ::ImGui_ImplGlfw_MouseButtonCallback(
        window, magic_enum::enum_integer(m.source.button), GLFW_PRESS, 0 /* todo */);
}

template<>
auto kawe::Window::useEvent(const event::Released<event::MouseButton> &m) -> void
{
    ::ImGui_ImplGlfw_MouseButtonCallback(
        window, magic_enum::enum_integer(m.source.button), GLFW_RELEASE, 0 /* todo */);
}

template<>
auto kawe::Window::useEvent(const event::Pressed<event::Key> &k) -> void
{
    ::ImGui_ImplGlfw_KeyCallback(
        window, static_cast<int>(k.source.keycode), k.source.scancode, GLFW_PRESS, 0 /* todo */);
}

template<>
auto kawe::Window::useEvent(const event::Released<event::Key> &k) -> void
{
    ::ImGui_ImplGlfw_KeyCallback(
        window, static_cast<int>(k.source.keycode), k.source.scancode, GLFW_RELEASE, 0 /* todo */);
}

template<>
auto kawe::Window::useEvent(const event::Character &character) -> void
{
    ::ImGui_ImplGlfw_CharCallback(window, character.codepoint);
}

template<>
auto kawe::Window::useEvent(const event::MouseScroll &scroll) -> void
{
    ::ImGui_ImplGlfw_ScrollCallback(window, scroll.x, scroll.y);
}
