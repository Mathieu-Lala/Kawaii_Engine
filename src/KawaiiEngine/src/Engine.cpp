#include "Engine.hpp"

using namespace std::chrono_literals;

kawe::Engine::Engine() : entity_hierarchy{component_inspector.selected}
{
    constexpr auto KAWE_GLFW_MAJOR = 4;
    constexpr auto KAWE_GLFW_MINOR = 5;

    glfwSetErrorCallback([](int code, const char *message) {
        spdlog::get("console")->error("[GLFW] An error occurred '{}' 'code={}'", message, code);
    });

    if (glfwInit() == GLFW_FALSE) {
        spdlog::get("console")->critical("[GLFW] Initialisation failed");
        return;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, KAWE_GLFW_MAJOR);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, KAWE_GLFW_MINOR);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    spdlog::get("console")->debug("[GLFW] version: '{}'", glfwGetVersionString());

    // todo : only in debug mode
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

    // todo : allow app to change that
    window = std::make_unique<Window>("Kawe: Engine", glm::ivec2{1600, 900});
    glfwMakeContextCurrent(window->get());

    if (const auto err = glewInit(); err != GLEW_OK) {
        spdlog::get("console")->critical("[GLEW] An error occurred '{}' 'code={}'", glewGetErrorString(err), err);
        return;
    }

    spdlog::get("console")->debug("[Engine] OpenGL version supported by this platform ({})", glGetString(GL_VERSION));

    // todo : only in debug mode
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(
        []([[maybe_unused]] GLenum source,
           GLenum type,
           [[maybe_unused]] GLuint id,
           [[maybe_unused]] GLenum severity,
           [[maybe_unused]] GLsizei length,
           const GLchar *message,
           const void *) {
            if (type == GL_DEBUG_TYPE_ERROR) {
                spdlog::get("console")->error("[Engine] GL CALLBACK: message = {}", message);
            } else {
                spdlog::get("console")->warn("[Engine] GL CALLBACK: message = {}", message);
            }
        },
        nullptr);

    {
        IMGUI_CHECKVERSION();

        auto imgui_ctx = ImGui::CreateContext();
        if (!imgui_ctx) { return; }

        if (!::ImGui_ImplGlfw_InitForOpenGL(window->get(), false)) { return; }
        if (!::ImGui_ImplOpenGL3_Init()) { return; }

        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    }

    glfwSwapInterval(0);

    world.set<entt::dispatcher *>(&dispatcher);
    world.set<ResourceLoader *>(&loader);
    ctx = std::make_unique<Context>(world);
    world.set<Context *>(ctx.get());

    events = std::make_unique<EventProvider>(*window);
    event_monitor = std::make_unique<EventMonitor>(*events, world);
    recorder = std::make_unique<Recorder>(*window);

    system = std::make_unique<System>(world, dispatcher, *ctx, *window);

    dispatcher.sink<kawe::action::Render<kawe::Render::Layout::UI>>().connect<&Engine::on_imgui>(*this);
}

kawe::Engine::~Engine()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    // ImGui::DestroyContext(imgui_ctx);

    glfwSetErrorCallback(nullptr);
    glfwTerminate();
}

auto kawe::Engine::start(const std::function<void(entt::registry &)> on_create) -> void
{
    CALL_OPEN_GL(::glEnable(GL_DEPTH_TEST));
    CALL_OPEN_GL(::glEnable(GL_BLEND));
    CALL_OPEN_GL(::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    const auto camera_one = world.create();
    world.emplace<CameraData>(camera_one);
    world.emplace_or_replace<Name>(camera_one, fmt::format("<kawe:camera#{}>", camera_one));

    on_create(world);

    while (ctx->is_running) {
        const auto event = events->getNextEvent();

        if (events->getState() == EventProvider::State::PLAYBACK) {
            std::visit(
                overloaded{
                    [&](const event::TimeElapsed &e) { std::this_thread::sleep_for(e.elapsed); },
                    [&](const event::Moved<event::Mouse> &e) {
                        window->setCursorPosition({e.x, e.y});
                    },
                    [&](const event::Moved<event::Window> &e) {
                        window->setPosition({e.x, e.y});
                    },
                    [&](const event::ResizeWindow &e) {
                        window->setSize({e.width, e.height});
                    },
                    [&](const event::MaximazeWindow &e) { window->maximaze(e.maximazed); },
                    [&](const event::MinimazeWindow &e) { window->minimaze(e.minimazed); },
                    [&](const event::FocusWindow &e) { window->focus(e.focused); },
                    [](const auto &) {}},
                event);
        }

        std::visit(
            overloaded{
                [&](const event::Connected<event::Window> &) {
                    events->setCurrentTimepoint(std::chrono::steady_clock::now());
                },
                [&](const event::Disconnected<event::Window> &) { ctx->is_running = false; },
                [&](const event::Moved<event::Mouse> &mouse) {
                    ctx->mouse_pos = {mouse.x, mouse.y};
                    dispatcher.trigger<event::Moved<event::Mouse>>(mouse);
                },
                [&](const event::Pressed<event::MouseButton> &e) {
                    window->sendEventToImGui(e);
                    ctx->state_mouse_button[e.source.button] = true;
                    ctx->mouse_pos_when_pressed = ctx->mouse_pos;
                    dispatcher.trigger<event::Pressed<event::MouseButton>>(e);
                },
                [&](const event::Released<event::MouseButton> &e) {
                    window->sendEventToImGui(e);
                    ctx->state_mouse_button[e.source.button] = false;
                    dispatcher.trigger<event::Released<event::MouseButton>>(e);
                },
                [&](const event::Pressed<event::Key> &e) {
                    window->sendEventToImGui(e);
                    ctx->keyboard_state[e.source.keycode] = true;
                    // todo : should be a signal instead
                    if (e.source.keycode == event::Key::Code::KEY_F10) {
                        std::filesystem::create_directories("screenshot");
                        window->screenshot(fmt::format("screenshot/screenshot_{}.png", time_to_string()));
                    }
                    dispatcher.trigger<event::Pressed<event::Key>>(e);
                },
                [&](const event::Released<event::Key> &e) {
                    window->sendEventToImGui(e);
                    ctx->keyboard_state[e.source.keycode] = false;
                    dispatcher.trigger<event::Released<event::Key>>(e);
                },
                [&](const event::Character &e) {
                    window->sendEventToImGui(e);
                    dispatcher.trigger<event::Character>(e);
                },
                [&](const event::MouseScroll &e) {
                    window->sendEventToImGui(e);
                    dispatcher.trigger<event::MouseScroll>(e);
                },
                [&](const event::TimeElapsed &e) {
                    // todo : trigger a time elapsed only if the simulation is running
                    ImGui_ImplOpenGL3_NewFrame();
                    ImGui_ImplGlfw_NewFrame();
                    ImGui::NewFrame();

                    dispatcher.trigger<action::Render<Render::Layout::UI>>({});

                    ImGui::Render();

                    dispatcher.trigger<action::Render<Render::Layout::SCENE>>({});

                    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
                    glfwSwapBuffers(window->get());

                    dispatcher.trigger<event::TimeElapsed>(e);
                },
                [](const auto &) {}},
            event);
    }

    if (event_monitor->export_on_close) {
        nlohmann::json serialized(events->getEventsProcessed());
        std::filesystem::create_directories("logs");
        std::ofstream f{fmt::format("logs/recorded_events_{}.json", time_to_string())};
        f << serialized;
    }

    world.clear();
}

auto kawe::Engine::on_imgui(const kawe::action::Render<kawe::Render::Layout::UI>) -> void
{
    const auto viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin(
        "DockSpace Demo",
        nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground
            | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking);
    ImGui::PopStyleVar();

    ImGui::PopStyleVar(2);

    const auto dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None | ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::End();

    if (render_internal_gui) {
        ImGui::ShowDemoWindow();

        entity_hierarchy.draw(world);
        component_inspector.draw<Component>(world);
        event_monitor->draw();
        recorder->draw();
        console.draw();
    }
}
