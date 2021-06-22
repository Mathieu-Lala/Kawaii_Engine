#pragma once

#include <memory>
#include <thread>

#include <spdlog/spdlog.h>
#include <glm/vec4.hpp>

#include <entt/entt.hpp>

#include "helpers/overloaded.hpp"
#include "graphics/Window.hpp"
#include "EventProvider.hpp"
#include "Event.hpp"
#include "graphics/Shader.hpp"
#include "component.hpp"
#include "State.hpp"

#include "resources/ResourceLoader.hpp"

#include "widgets/ComponentInspector.hpp"
#include "widgets/EntityHierarchy.hpp"
#include "widgets/EventMonitor.hpp"
#include "widgets/Recorder.hpp"


using namespace std::chrono_literals;

namespace kawe {

class Engine {
public:
    Engine();
    ~Engine();

    std::function<void()> on_imgui;
    std::function<void(entt::registry &)> on_create;
    bool render_internal_gui = true;

    auto start() -> void;

private:
    std::unique_ptr<EventProvider> events;
    std::unique_ptr<Window> window;

    ResourceLoader loader;
    entt::dispatcher dispatcher;
    entt::registry world;

    ComponentInspector component_inspector;
    EntityHierarchy entity_hierarchy;
    std::unique_ptr<EventMonitor> event_monitor;
    std::unique_ptr<Recorder> recorder;

    std::unique_ptr<State> state;

    auto update_aabb(entt::registry &reg, entt::entity e) -> void;
    auto update_vbo_color(entt::registry &reg, entt::entity e) -> void;

    // AABB algorithm = really simple and fast collision detection
    auto check_collision(entt::registry &reg, entt::entity e) -> void;

    static auto draw_docking_window() -> void;

    auto on_time_elapsed(const event::TimeElapsed &e) -> void;

    auto update_camera(entt::registry &reg, entt::entity e) -> void;
    auto create_camera(entt::registry &reg, entt::entity e) -> void;

    auto try_update_camera_target(entt::registry &reg, entt::entity e) -> void;

    auto update_camera_event(const event::TimeElapsed &e) -> void;

    auto system_rendering() -> void;
};

} // namespace kawe
