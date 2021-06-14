#pragma once

#include <vector>
#include <chrono>

#include "graphics/Window.hpp"
#include "Event.hpp"

namespace kawe {

class Window;

class EventProvider {
public:
    EventProvider();

    auto assign(const Window &window) -> void;
    auto getNextEvent() -> event::Event;
    auto getLastEventWhere(const std::function<bool(const event::Event &)> &predicate) const noexcept
        -> const event::Event &;
    auto getEventsProcessed() const noexcept -> const std::vector<event::Event> &;
    auto setCurrentTimepoint(const std::chrono::steady_clock::time_point &t) -> void;
    auto getTimeScaler() const noexcept -> const double &;
    auto setTimeScaler(double value) noexcept -> void;

    auto exportEvents();

private:
    static EventProvider *s_instance;

    std::chrono::steady_clock::time_point m_lastTimePoint;

    std::vector<event::Event> m_buffer_events; // input buffer

    std::vector<event::Event> m_events_processed; // previous event

    double m_time_scaler{1.0};

    auto fetchEvent() -> event::Event;
    auto getElapsedTime() noexcept -> std::chrono::nanoseconds;
    static auto callback_eventClose(::GLFWwindow *window) -> void;
    static auto callback_eventResized(GLFWwindow *, int w, int h) -> void;
    static auto callback_eventMoved(GLFWwindow *, int x, int y) -> void;
    static auto callback_eventKeyBoard(GLFWwindow *, int key, int scancode, int action, int mods) -> void;
    static auto callback_eventMousePressed(GLFWwindow *window, int button, int action, int mods) -> void;
    static auto callback_eventMouseMoved(GLFWwindow *, double x, double y) -> void;
    static auto callback_char(GLFWwindow *, unsigned int codepoint) -> void;
};

} // namespace kawe
