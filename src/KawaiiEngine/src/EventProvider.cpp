#include <vector>
#include <chrono>

#include "graphics/Window.hpp"
#include "EventProvider.hpp"
#include "helpers/overloaded.hpp"

kawe::EventProvider *kawe::EventProvider::s_instance = nullptr;

kawe::EventProvider::EventProvider() { s_instance = this; }

auto kawe::EventProvider::assign(const Window &window) -> void
{
    ::glfwSetWindowCloseCallback(window.get(), callback_eventClose);
    ::glfwSetWindowSizeCallback(window.get(), callback_eventResized);
    ::glfwSetWindowPosCallback(window.get(), callback_eventMoved);
    ::glfwSetKeyCallback(window.get(), callback_eventKeyBoard);
    ::glfwSetMouseButtonCallback(window.get(), callback_eventMousePressed);
    ::glfwSetCursorPosCallback(window.get(), callback_eventMouseMoved);
    ::glfwSetCharCallback(window.get(), callback_char);

    /// todo : ImGui_ImplGlfw_ScrollCallback
    /// glfwSetScrollCallback(window, scroll_callback);
    /// void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)

    m_buffer_events.emplace_back(OpenWindow{});
    m_events_processed.emplace_back(TimeElapsed{});


    // // Prepare window
    // glfwSetFramebufferSizeCallback(window, framebuffer_size_handler);
    // glfwSetWindowSizeCallback(window, window_resize_handler);
    // glfwSetWindowIconifyCallback(window, window_iconify_handler);
    // glfwSetWindowFocusCallback(window, window_focus_handler);
    // glfwSetWindowCloseCallback(window, window_close_handler);
    // glfwSetErrorCallback(error_handler);
    //
    // // Set input callbacks
    // glfwSetKeyCallback(window, keyboard_handler);
    // glfwSetCharCallback(window, char_input_handler);
    // glfwSetMouseButtonCallback(window, mouse_button_handler);
    // glfwSetCursorPosCallback(window, mouse_move_handler);
    // glfwSetScrollCallback(window, mouse_scroll_handler);
    // glfwSetJoystickCallback(joystick_configuration_change_handler);
}

auto kawe::EventProvider::getNextEvent() -> Event
{
    const auto event = fetchEvent();

    std::visit(
        overloaded{
            [](TimeElapsed &prev, const TimeElapsed &next) { prev.elapsed += next.elapsed; },
            [&](const auto &, const std::monostate &) {},
            [&](const auto &, const auto &next) { m_events_processed.push_back(next); }},
        m_events_processed.back(),
        event);

    return event;
}

auto kawe::EventProvider::getLastEvent() const noexcept -> const Event &
{
    for (auto i = m_events_processed.size(); i; i--) {
        const auto &event = m_events_processed.at(i - 1);
        if (!std::holds_alternative<TimeElapsed>(event)) { return event; }
    }
    return m_events_processed.back();
}

auto kawe::EventProvider::getEventsProcessed() const noexcept -> const std::vector<Event> &
{
    return m_events_processed;
}

auto kawe::EventProvider::setCurrentTimepoint(const std::chrono::steady_clock::time_point &t) -> void
{
    m_lastTimePoint = t;
}

auto kawe::EventProvider::getTimeScaler() const noexcept -> const double & { return m_time_scaler; }

auto kawe::EventProvider::setTimeScaler(double value) noexcept -> void { m_time_scaler = value; }

auto kawe::EventProvider::fetchEvent() -> Event
{
    ::glfwPollEvents();

    if (m_buffer_events.empty()) {
        return TimeElapsed{std::chrono::nanoseconds{
            static_cast<std::int64_t>(static_cast<double>(getElapsedTime().count()) * m_time_scaler)}};
    } else {
        const auto event = m_buffer_events.front();
        m_buffer_events.erase(m_buffer_events.begin());
        return event;
    }
}

auto kawe::EventProvider::getElapsedTime() noexcept -> std::chrono::nanoseconds
{
    const auto newTimePoint = std::chrono::steady_clock::now();
    const auto timeElapsed = newTimePoint - m_lastTimePoint;
    m_lastTimePoint = newTimePoint;
    return timeElapsed;
}

auto kawe::EventProvider::callback_eventClose(::GLFWwindow *window) -> void
{
    ::glfwSetWindowShouldClose(window, false);
    s_instance->m_buffer_events.emplace_back(CloseWindow{});
}

auto kawe::EventProvider::callback_eventResized(GLFWwindow *, int w, int h) -> void
{
    s_instance->m_buffer_events.emplace_back(ResizeWindow{w, h});
}

auto kawe::EventProvider::callback_eventMoved(GLFWwindow *, int x, int y) -> void
{
    s_instance->m_buffer_events.emplace_back(MoveWindow{x, y});
}

auto kawe::EventProvider::callback_eventKeyBoard(GLFWwindow *, int key, int scancode, int action, int mods) -> void
{
    // clang-format off
        const Key k{
            .alt        = !!(mods & GLFW_MOD_ALT),
            .control    = !!(mods & GLFW_MOD_CONTROL),
            .system     = !!(mods & GLFW_MOD_SUPER),
            .shift      = !!(mods & GLFW_MOD_SHIFT),
            .scancode   = scancode,
            .keycode    = magic_enum::enum_cast<Key::Code>(key).value_or(Key::Code::KEY_UNKNOWN)
        };
    // clang-format on
    switch (action) {
    case GLFW_PRESS: s_instance->m_buffer_events.emplace_back(Pressed<Key>{k}); break;
    case GLFW_RELEASE:
        s_instance->m_buffer_events.emplace_back(Released<Key>{k});
        break;
        // case GLFW_REPEAT: s_instance->m_buffer_events.emplace_back(???{ key }); break; // todo
        // default: std::abort(); break;
    };
}

auto kawe::EventProvider::callback_eventMousePressed(
    GLFWwindow *window, int button, int action, [[maybe_unused]] int mods) -> void
{
    double x = 0;
    double y = 0;
    ::glfwGetCursorPos(window, &x, &y);
    switch (action) {
    case GLFW_PRESS:
        s_instance->m_buffer_events.emplace_back(Pressed<MouseButton>{MouseButton::toButton(button), {x, y}});
        break;
    case GLFW_RELEASE:
        s_instance->m_buffer_events.emplace_back(Released<MouseButton>{MouseButton::toButton(button), {x, y}});
        break;
        // default: std::abort(); break;
    };
}

auto kawe::EventProvider::callback_eventMouseMoved(GLFWwindow *, double x, double y) -> void
{
    s_instance->m_buffer_events.emplace_back(Moved<Mouse>{x, y});
}

auto kawe::EventProvider::callback_char(GLFWwindow *, unsigned int codepoint) -> void
{
    s_instance->m_buffer_events.emplace_back(Character{codepoint});
}
