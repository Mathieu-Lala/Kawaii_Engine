#pragma once

#include <spdlog/spdlog.h>
#include <string_view>
#include <glm/vec2.hpp>
#include "graphics/deps.hpp"

namespace kawe {

class EventProvider;

class Window {
public:
    Window(const std::string_view window_title, glm::ivec2 &&size, glm::ivec2 &&position = {0, 0}, bool isFullscreen = false);

    ~Window() { glfwDestroyWindow(window); }

    auto get() const noexcept -> GLFWwindow * { return window; }

    template<typename T>
    auto sendEventToImGui(const T &) -> void;

    template<typename T = double>
    [[nodiscard]] auto getSize() const noexcept -> glm::vec<2, T>
    {
        int width{};
        int height{};
        ::glfwGetWindowSize(window, &width, &height);
        return {static_cast<T>(width), static_cast<T>(height)};
    }

    template<typename T = double>
    [[nodiscard]] auto getAspectRatio() const noexcept -> T
    {
        const auto size = getSize<T>();
        return static_cast<T>(size.x) / static_cast<T>(size.y);
    }

    auto setCursorPosition(glm::dvec2 &&pos) -> void { ::glfwSetCursorPos(window, pos.x, pos.y); }

    auto setPosition(glm::ivec2 &&pos) -> void { ::glfwSetWindowPos(window, pos.x, pos.y); }

    auto setSize(glm::ivec2 &&size) -> void { ::glfwSetWindowSize(window, size.x, size.y); }

    auto maximaze(bool value) -> void { value ? ::glfwMaximizeWindow(window) : ::glfwRestoreWindow(window); }
    auto minimaze(bool value) -> void { value ? ::glfwIconifyWindow(window) : ::glfwRestoreWindow(window); }

    auto focus(bool value) -> void
    {
        if (value) ::glfwFocusWindow(window);
    }

    auto screenshot(const std::string_view filename) -> bool;

private:
    GLFWwindow *window;
};

} // namespace kawe
