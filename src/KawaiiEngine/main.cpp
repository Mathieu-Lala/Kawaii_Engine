#include <spdlog/spdlog.h>

#include "graphics/Window.hpp"

int main()
{
    constexpr auto KAWE_GLFW_MAJOR = 3;
    constexpr auto KAWE_GLFW_MINOR = 3;

    spdlog::warn("Hello world");

    glfwSetErrorCallback([](int code, const char *message) {
        spdlog::error("[GLFW] An error occured '{}' 'code={}'\n", message, code);
    });

    if (glfwInit() == GLFW_FALSE) { return false; }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, KAWE_GLFW_MAJOR);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, KAWE_GLFW_MINOR);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    spdlog::trace("[GLFW] Version: '{}'\n", glfwGetVersionString());

    kawe::Window window{"test", {1080, 800}};
    glfwMakeContextCurrent(window.get());

    if (const auto err = glewInit(); err != GLEW_OK) {
        spdlog::error("[GLEW] An error occured '{}' 'code={}'", glewGetErrorString(err), err);
        return 1;
    }
}
