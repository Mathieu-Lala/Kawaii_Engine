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
#include "Context.hpp"

#include "resources/ResourceLoader.hpp"

#include "widgets/ComponentInspector.hpp"
#include "widgets/EntityHierarchy.hpp"
#include "widgets/EventMonitor.hpp"
#include "widgets/Recorder.hpp"
#include "widgets/Console.hpp"

#include "System.hpp"

using namespace std::chrono_literals;

namespace kawe {

class Engine {
public:
    Engine();
    ~Engine();

    bool render_internal_gui = true;

    auto start(const std::function<void(entt::registry &)> on_create) -> void;

private:
    // widgets
    Console console;
    ComponentInspector component_inspector;
    EntityHierarchy entity_hierarchy;
    std::unique_ptr<EventMonitor> event_monitor;
    std::unique_ptr<Recorder> recorder;

    std::unique_ptr<EventProvider> events;

    // todo : should be an entity
    std::unique_ptr<Window> window;

    ResourceLoader loader;

    entt::dispatcher dispatcher;
    entt::registry world;
    std::unique_ptr<Context> ctx;
    std::unique_ptr<System> system;

    auto on_imgui(const kawe::action::Render<kawe::Render::Layout::UI>) -> void;
};

} // namespace kawe
