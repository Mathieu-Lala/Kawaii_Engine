#include "graphics/Window.hpp"

kawe::Window::Window(const std::string_view window_title, glm::ivec2 &&size, glm::ivec2 &&position, bool isFullscreen)
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
    /*
            // Prepare window
            glfwSetWindowUserPointer(window, this);
            glfwSetFramebufferSizeCallback(window, framebuffer_size_handler);
            glfwSetWindowSizeCallback(window, window_resize_handler);
            glfwSetWindowIconifyCallback(window, window_iconify_handler);
            glfwSetWindowFocusCallback(window, window_focus_handler);
            glfwSetWindowCloseCallback(window, window_close_handler);
            glfwSetErrorCallback(error_handler);

            // Set input callbacks
            glfwSetKeyCallback(window, keyboard_handler);
            glfwSetCharCallback(window, char_input_handler);
            glfwSetMouseButtonCallback(window, mouse_button_handler);
            glfwSetCursorPosCallback(window, mouse_move_handler);
            glfwSetScrollCallback(window, mouse_scroll_handler);
            glfwSetJoystickCallback(joystick_configuration_change_handler);
    */

    isWindowIconofied = glfwGetWindowAttrib(window, GLFW_ICONIFIED);
    isWindowFocused = glfwGetWindowAttrib(window, GLFW_FOCUSED);

    // current_window_size = get_window_size();
}
