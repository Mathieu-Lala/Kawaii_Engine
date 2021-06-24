#pragma once

#include <vector>
#include <chrono>

#include "graphics/Window.hpp"
#include "Event.hpp"

namespace kawe {

class Window;

class EventProvider {
public:
    EventProvider(const Window &window);

    auto getNextEvent() -> event::Event;
    auto getLastEventWhere(const std::function<bool(const event::Event &)> &predicate) const noexcept
        -> std::optional<const event::Event *>;

    auto getEventsProcessed() const noexcept -> const std::vector<event::Event> &
    {
        return m_events_processed;
    }

    auto getEventsPending() const noexcept -> const std::vector<event::Event> & { return m_buffer_events; }

    auto setPendingEvents(std::vector<event::Event> &&in) noexcept -> void
    {
        m_buffer_events = std::move(in);
    }

    auto setCurrentTimepoint(const std::chrono::steady_clock::time_point &t) -> void { m_lastTimePoint = t; }

    auto getTimeScaler() const noexcept -> const double & { return m_time_scaler; }
    auto setTimeScaler(double value) noexcept -> void { m_time_scaler = value; }

    auto clear() -> void
    {
        m_buffer_events.clear();
        m_events_processed.clear();
    }

    enum class State {
        RECORD,
        PLAYBACK,
    };

    auto getState() const noexcept { return m_state; }

    auto setState(State s)
    {
        m_state = s;

        const bool is_record = m_state == State::RECORD;
        ::glfwSetWindowCloseCallback(m_window.get(), is_record ? callback_eventClose : nullptr);
        ::glfwSetWindowSizeCallback(m_window.get(), is_record ? callback_eventResized : nullptr);
        ::glfwSetWindowPosCallback(m_window.get(), is_record ? callback_eventMoved : nullptr);
        ::glfwSetKeyCallback(m_window.get(), is_record ? callback_eventKeyBoard : nullptr);
        ::glfwSetMouseButtonCallback(m_window.get(), is_record ? callback_eventMousePressed : nullptr);
        ::glfwSetCursorPosCallback(m_window.get(), is_record ? callback_eventMouseMoved : nullptr);
        ::glfwSetCharCallback(m_window.get(), is_record ? callback_char : nullptr);
        ::glfwSetScrollCallback(m_window.get(), is_record ? callback_scroll : nullptr);
        ::glfwSetWindowIconifyCallback(m_window.get(), is_record ? callback_minimaze : nullptr);
        ::glfwSetWindowMaximizeCallback(m_window.get(), is_record ? callback_maximaze : nullptr);
        ::glfwSetWindowFocusCallback(m_window.get(), is_record ? callback_focus : nullptr);
        // todo :
        // glfwSetJoystickCallback(joystick_configuration_change_handler);

        /// nothing to do because viewport is 0..1 ?
        // glfwSetFramebufferSizeCallback(window, framebuffer_size_handler);

        // glfwSetWindowContentScaleCallback(window, window_content_scale_callback);
        // ...
    }

private:
    static EventProvider *s_instance;
    const Window &m_window;

    std::chrono::steady_clock::time_point m_lastTimePoint;

    std::vector<event::Event> m_buffer_events; // input buffer

    std::vector<event::Event> m_events_processed; // previous event

    double m_time_scaler{1.0};

    State m_state{State::RECORD};

    auto fetchEvent() -> event::Event;
    auto getElapsedTime() noexcept -> std::chrono::nanoseconds;

    static auto callback_eventClose(::GLFWwindow *window) -> void;
    static auto callback_eventResized(GLFWwindow *, int w, int h) -> void;
    static auto callback_eventMoved(GLFWwindow *, int x, int y) -> void;
    static auto callback_eventKeyBoard(GLFWwindow *, int key, int scancode, int action, int mods) -> void;
    static auto callback_eventMousePressed(GLFWwindow *window, int button, int action, int mods) -> void;
    static auto callback_eventMouseMoved(GLFWwindow *, double x, double y) -> void;
    static auto callback_char(GLFWwindow *, unsigned int codepoint) -> void;
    static auto callback_scroll(GLFWwindow *, double xoffset, double yoffset) -> void;
    static auto callback_maximaze(GLFWwindow *, int value) -> void;
    static auto callback_minimaze(GLFWwindow *, int value) -> void;
    static auto callback_focus(GLFWwindow *, int value) -> void;
};

} // namespace kawe
