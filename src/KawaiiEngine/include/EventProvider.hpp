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

    // auto assign(const Window &window) -> void;
    auto getNextEvent() -> event::Event;
    auto getLastEventWhere(const std::function<bool(const event::Event &)> &predicate) const noexcept
        -> std::optional<const event::Event *>;

    auto exportEvents();

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
        RECORD,   // the new event from the event callback are appended to m_buffer_events
        PLAYBACK, // the callback are ignored
    };

    auto getState() const noexcept { return m_state; }

    auto setState(State s)
    {
        // todo :
        // glfwSetJoystickCallback(joystick_configuration_change_handler);
        // glfwSetFramebufferSizeCallback(window, framebuffer_size_handler);
        // glfwSetWindowIconifyCallback(window, window_iconify_handler);
        // glfwSetWindowFocusCallback(window, window_focus_handler);

        spdlog::warn("set the right callbakc {}", s);

        m_state = s;

        switch (s) {
        case State::RECORD: {
            spdlog::warn("");
            ::glfwSetWindowCloseCallback(m_window.get(), callback_eventClose);
            ::glfwSetWindowSizeCallback(m_window.get(), callback_eventResized);
            ::glfwSetWindowPosCallback(m_window.get(), callback_eventMoved);
            ::glfwSetKeyCallback(m_window.get(), callback_eventKeyBoard);
            ::glfwSetMouseButtonCallback(m_window.get(), callback_eventMousePressed);
            ::glfwSetCursorPosCallback(m_window.get(), callback_eventMouseMoved);
            ::glfwSetCharCallback(m_window.get(), callback_char);
            ::glfwSetScrollCallback(m_window.get(), callback_scroll);
        } break;
        case State::PLAYBACK: {
            ::glfwSetWindowCloseCallback(m_window.get(), nullptr);
            ::glfwSetWindowSizeCallback(m_window.get(), nullptr);
            ::glfwSetWindowPosCallback(m_window.get(), nullptr);
            ::glfwSetKeyCallback(m_window.get(), nullptr);
            ::glfwSetMouseButtonCallback(m_window.get(), nullptr);
            ::glfwSetCursorPosCallback(m_window.get(), nullptr);
            ::glfwSetCharCallback(m_window.get(), nullptr);
            ::glfwSetScrollCallback(m_window.get(), nullptr);
        } break;
        }
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
};

} // namespace kawe
