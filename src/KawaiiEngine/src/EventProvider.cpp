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
    ::glfwSetScrollCallback(window.get(), callback_scroll);

    // glfwSetJoystickCallback(joystick_configuration_change_handler);
    // glfwSetFramebufferSizeCallback(window, framebuffer_size_handler);
    // glfwSetWindowIconifyCallback(window, window_iconify_handler);
    // glfwSetWindowFocusCallback(window, window_focus_handler);
    // glfwSetErrorCallback(error_handler);

    m_buffer_events.emplace_back(event::Connected<event::Window>{});
}

auto kawe::EventProvider::getNextEvent() -> event::Event
{
    const auto event = fetchEvent();

    if (!m_events_processed.empty()) {
        // post processing to merge the event for example
        std::visit(
            overloaded{
                [](event::TimeElapsed &prev, const event::TimeElapsed &next) { prev += next; },
                [&](const auto &, const auto &next) { m_events_processed.push_back(next); }},
            m_events_processed.back(),
            event);
    } else {
        m_events_processed.push_back(event);
    }

    return event;
}

auto kawe::EventProvider::getLastEventWhere(const std::function<bool(const event::Event &)> &predicate) const noexcept
    -> std::optional<const event::Event *>
{
    if (const auto found = std::find_if(m_events_processed.rbegin(), m_events_processed.rend(), predicate);
        found != m_events_processed.rend()) {
        return &(*found);
    } else {
        return {};
    }
}

auto kawe::EventProvider::fetchEvent() -> event::Event
{
    ::glfwPollEvents();

    if (m_buffer_events.empty()) {
        const auto elapsed = getElapsedTime();
        return event::TimeElapsed{
            std::chrono::nanoseconds{static_cast<std::int64_t>(static_cast<double>(elapsed.count()))},
            std::chrono::nanoseconds{
                static_cast<std::int64_t>(static_cast<double>(elapsed.count()) * m_time_scaler)}};
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
    s_instance->m_buffer_events.emplace_back(event::Disconnected<event::Window>{});
}

auto kawe::EventProvider::callback_eventResized(GLFWwindow *, int w, int h) -> void
{
    s_instance->m_buffer_events.emplace_back(event::ResizeWindow{w, h});
}

auto kawe::EventProvider::callback_eventMoved(GLFWwindow *, int x, int y) -> void
{
    s_instance->m_buffer_events.emplace_back(
        event::Moved<event::Window>{event::Window{}, static_cast<double>(x), static_cast<double>(y)});
}

auto kawe::EventProvider::callback_eventKeyBoard(GLFWwindow *, int key, int scancode, int action, int mods) -> void
{
    // clang-format off
    const event::Key k{
        .alt        = !!(mods & GLFW_MOD_ALT),
        .control    = !!(mods & GLFW_MOD_CONTROL),
        .system     = !!(mods & GLFW_MOD_SUPER),
        .shift      = !!(mods & GLFW_MOD_SHIFT),
        .scancode   = scancode,
        .keycode    = magic_enum::enum_cast<event::Key::Code>(key).value_or(event::Key::Code::KEY_UNKNOWN)
    };
    // clang-format on
    switch (action) {
    case GLFW_PRESS: s_instance->m_buffer_events.emplace_back(event::Pressed<event::Key>{k}); break;
    case GLFW_RELEASE:
        s_instance->m_buffer_events.emplace_back(event::Released<event::Key>{k});
        break;
        // case GLFW_REPEAT: s_instance->m_buffer_events.emplace_back(???{ key }); break; // todo
        // default: std::abort(); break;
    };
}

auto kawe::EventProvider::callback_eventMousePressed(
    [[maybe_unused]] GLFWwindow *window, int button, int action, [[maybe_unused]] int mods) -> void
{
    // double x = 0;
    // double y = 0;
    // ::glfwGetCursorPos(window, &x, &y);
    switch (action) {
    case GLFW_PRESS:
        s_instance->m_buffer_events.emplace_back(
            event::Pressed<event::MouseButton>{event::MouseButton::toButton(button), {}});
        break;
    case GLFW_RELEASE:
        s_instance->m_buffer_events.emplace_back(
            event::Released<event::MouseButton>{event::MouseButton::toButton(button), {}});
        break;
        // default: std::abort(); break;
    };
}

auto kawe::EventProvider::callback_eventMouseMoved(GLFWwindow *, double x, double y) -> void
{
    s_instance->m_buffer_events.emplace_back(event::Moved<event::Mouse>{event::Mouse{}, x, y});
}

auto kawe::EventProvider::callback_char(GLFWwindow *, unsigned int codepoint) -> void
{
    s_instance->m_buffer_events.emplace_back(event::Character{codepoint});
}

auto kawe::EventProvider::callback_scroll(GLFWwindow *, double xoffset, double yoffset) -> void
{
    s_instance->m_buffer_events.emplace_back(event::MouseScroll{xoffset, yoffset});
}
