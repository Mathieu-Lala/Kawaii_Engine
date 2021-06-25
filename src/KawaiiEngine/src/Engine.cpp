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

    spdlog::get("console")->debug(
        "[Engine] OpenGL version supported by this platform ({})", glGetString(GL_VERSION));

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

    IMGUI_CHECKVERSION();

    auto imgui_ctx = ImGui::CreateContext();
    if (!imgui_ctx) { return; }

    if (!::ImGui_ImplGlfw_InitForOpenGL(window->get(), false)) { return; }
    if (!::ImGui_ImplOpenGL3_Init()) { return; }

    glfwSwapInterval(0);

    // rendering backend

    world.on_destroy<Render::VAO>().connect<Render::VAO::on_destroy>();
    world.on_destroy<Render::VBO<Render::VAO::Attribute::POSITION>>()
        .connect<Render::VBO<Render::VAO::Attribute::POSITION>::on_destroy>();
    world.on_destroy<Render::VBO<Render::VAO::Attribute::COLOR>>()
        .connect<Render::VBO<Render::VAO::Attribute::COLOR>::on_destroy>();
    world.on_destroy<Render::EBO>().connect<Render::EBO::on_destroy>();

    // collision update

    world.on_construct<AABB>().connect<&Engine::check_collision>(*this);
    world.on_update<AABB>().connect<&Engine::check_collision>(*this);

    world.on_construct<Position3f>().connect<&Engine::update_aabb>(*this);
    world.on_construct<Position3f>().connect<&Engine::update_aabb>(*this);
    world.on_construct<Position3f>().connect<&Engine::update_aabb>(*this);
    world.on_update<Position3f>().connect<&Engine::update_aabb>(*this);
    world.on_update<Rotation3f>().connect<&Engine::update_aabb>(*this);
    world.on_update<Scale3f>().connect<&Engine::update_aabb>(*this);

    world.on_construct<Render::VBO<Render::VAO::Attribute::POSITION>>().connect<&Engine::update_aabb>(*this);
    world.on_update<Render::VBO<Render::VAO::Attribute::POSITION>>().connect<&Engine::update_aabb>(*this);

    // create sub entity

    world.on_construct<Collider>().connect<&Engine::update_aabb>(*this);

    world.on_construct<CameraData>().connect<&Engine::create_camera>(*this);

    // update color

    world.on_construct<FillColor>().connect<&Engine::update_vbo_color>(*this);
    world.on_update<FillColor>().connect<&Engine::update_vbo_color>(*this);

    world.on_update<Render::VBO<Render::VAO::Attribute::COLOR>>()
        .connect<[](entt::registry &reg, entt::entity e) -> void { reg.remove_if_exists<FillColor>(e); }>();


    world.on_construct<CameraData>().connect<&Engine::update_camera>(*this);
    world.on_update<CameraData>().connect<&Engine::update_camera>(*this);

    dispatcher.sink<event::TimeElapsed>().connect<&Engine::update_camera_event>(*this);
    world.on_update<Position3f>().connect<&Engine::update_camera>(*this);
    world.on_update<Position3f>().connect<&Engine::try_update_camera_target>(*this);

    world.set<entt::dispatcher *>(&dispatcher);
    world.set<ResourceLoader *>(&loader);
    ctx = std::make_unique<Context>(world);
    world.set<Context *>(ctx.get());

    events = std::make_unique<EventProvider>(*window);
    event_monitor = std::make_unique<EventMonitor>(*events, world);
    recorder = std::make_unique<Recorder>(*window);

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
}

kawe::Engine::~Engine()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    // ImGui::DestroyContext(imgui_ctx);

    glfwSetErrorCallback(nullptr);
    glfwTerminate();
}

auto kawe::Engine::start() -> void
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
                [&](const event::TimeElapsed &e) { on_time_elapsed(e); },
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

auto kawe::Engine::update_aabb(entt::registry &reg, entt::entity e) -> void
{
    if (const auto collider = reg.try_get<Collider>(e); collider != nullptr) {
        if (const auto vbo = reg.try_get<Render::VBO<Render::VAO::Attribute::POSITION>>(e); vbo != nullptr) {
            AABB::emplace(reg, e, vbo->vertices);
        }
    }
}

auto kawe::Engine::update_vbo_color(entt::registry &reg, entt::entity e) -> void
{
    const auto vbo_color = reg.try_get<Render::VBO<Render::VAO::Attribute::COLOR>>(e);
    const auto vbo_pos = reg.try_get<Render::VBO<Render::VAO::Attribute::POSITION>>(e);
    if (!vbo_color && !vbo_pos) return;

    const auto size = vbo_color != nullptr ? vbo_color->vertices.size() : vbo_pos->vertices.size();
    const auto stride_size = vbo_color != nullptr ? vbo_color->stride_size : vbo_pos->stride_size;

    const auto fill_color = reg.get<FillColor>(e);
    std::vector<float> vert{};
    for (auto i = 0ul; i != size; i += stride_size) {
        vert.emplace_back(fill_color.component.r);
        vert.emplace_back(fill_color.component.g);
        vert.emplace_back(fill_color.component.b);
        vert.emplace_back(fill_color.component.a);
    }

    Render::VBO<Render::VAO::Attribute::COLOR>::emplace(reg, e, vert, 4);
}

// AABB algorithm = really simple and fast collision detection
auto kawe::Engine::check_collision(entt::registry &reg, entt::entity e) -> void
{
    const auto aabb = reg.get<AABB>(e);

    bool has_aabb_collision = false;

    for (const auto &other : reg.view<Collider, AABB>()) {
        if (e == other) continue;
        const auto other_aabb = reg.get<AABB>(other);

        const auto collide = (aabb.min.x <= other_aabb.max.x && aabb.max.x >= other_aabb.min.x)
                             && (aabb.min.y <= other_aabb.max.y && aabb.max.y >= other_aabb.min.y)
                             && (aabb.min.z <= other_aabb.max.z && aabb.max.z >= other_aabb.min.z);
        has_aabb_collision |= collide;

        if (collide) {
            reg.patch<Collider>(e, [](auto &c) { c.step = Collider::CollisionStep::AABB; });
            reg.patch<Collider>(other, [](auto &c) { c.step = Collider::CollisionStep::AABB; });
            reg.emplace_or_replace<FillColor>(aabb.guizmo, glm::vec4{1.0f, 0.0f, 0.0f, 1.0f});
            reg.emplace_or_replace<FillColor>(other_aabb.guizmo, glm::vec4{1.0f, 0.0f, 0.0f, 1.0f});
        }
    }

    // todo : this logic should be done on another signal named on_collision_resolved ...
    const auto collider = reg.get<Collider>(e);
    if (!has_aabb_collision) {
        if (collider.step != Collider::CollisionStep::NONE) {
            reg.patch<Collider>(e, [](auto &c) { c.step = Collider::CollisionStep::NONE; });
            reg.emplace_or_replace<FillColor>(aabb.guizmo, glm::vec4{0.0f, 0.0f, 0.0f, 1.0f});
        }
    }
}

auto kawe::Engine::draw_docking_window() -> void
{
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin(
        "DockSpace Demo",
        nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus
            | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking);
    ImGui::PopStyleVar();

    ImGui::PopStyleVar(2);

    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(
        dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None | ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::End();
}

auto kawe::Engine::on_time_elapsed(const event::TimeElapsed &e) -> void
{
    const auto dt_nano = e.world_time;
    const auto dt_secs =
        static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(dt_nano).count()) / 1'000'000.0;

    for (const auto &entity : world.view<Position3f, Velocity3f>()) {
        const auto &vel = world.get<Velocity3f>(entity);
        world.patch<Position3f>(entity, [&vel, &dt_secs](auto &pos) {
            pos.component += vel.component * static_cast<double>(dt_secs);
        });
    }

    // updating clocks.
    world.view<Clock>().each([&e](Clock &clock) { clock.on_update(e); });

    for (const auto &entity : world.view<Gravitable3f, Velocity3f>()) {
        const auto &gravity = world.get<Gravitable3f>(entity);
        world.patch<Velocity3f>(entity, [&gravity, &dt_secs](auto &vel) {
            vel.component += gravity.component * static_cast<double>(dt_secs);
        });
    }

    // todo : trigger a time elapsed only if the simulation is running
    dispatcher.trigger<event::TimeElapsed>(e);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    draw_docking_window();
    {
        on_imgui();

        if (render_internal_gui) {
            ImGui::ShowDemoWindow();
            entity_hierarchy.draw(world);
            component_inspector.draw<Component>(world);
            event_monitor->draw();
            recorder->draw();
            console.draw();
        }
    }

    ImGui::Render();

    system_rendering();

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window->get());
}

auto kawe::Engine::update_camera(entt::registry &reg, entt::entity e) -> void
{
    const auto &cam = reg.try_get<CameraData>(e);
    if (cam == nullptr) { return; }
    const auto &pos = reg.get_or_emplace<Position3f>(e, Position3f{glm::dvec3{1.0, 1.0, 1.0}});

    const auto makeOrthogonalTo = [](const glm::dvec3 &vec1, const glm::dvec3 &vec2) -> glm::dvec3 {
        if (const auto length = glm::length(vec2); length != 0) {
            const auto scale = (vec1.x * vec2.x + vec1.y * vec2.y + vec1.z * vec2.z) / (length * length);
            return {
                vec1.x - scale * vec2.x,
                vec1.y - scale * vec2.y,
                vec1.z - scale * vec2.z,
            };
        } else {
            return vec1;
        }
    };

    const auto &target_center = reg.get<Position3f>(cam->target).component;

    const auto viewDir = glm::normalize(target_center - pos.component);

    cam->imagePlaneVertDir = glm::normalize(makeOrthogonalTo(cam->up, viewDir));
    cam->imagePlaneHorizDir = glm::normalize(glm::cross(viewDir, cam->imagePlaneVertDir));

    const auto size = window->getSize<double>() * glm::dvec2{cam->viewport.w, cam->viewport.h};

    cam->display.y = 2.0 * glm::length(target_center - pos.component) * std::tan(0.5 * cam->fov);
    cam->display.x = cam->display.y * (size.x / size.y);

    cam->projection = glm::perspective(glm::radians(cam->fov), size.x / size.y, cam->near, cam->far);
    cam->view = glm::lookAt(pos.component, target_center, cam->up);
}

auto kawe::Engine::create_camera(entt::registry &reg, entt::entity e) -> void
{
    const auto child = reg.create();
    reg.emplace<Position3f>(child);
    reg.emplace<Parent>(child, e);
    reg.emplace<Name>(child, fmt::format("<kawe:camera_target#{}>", e));

    reg.emplace_or_replace<Children>(e).component.push_back(child);
    reg.get<CameraData>(e).target = child;
}

auto kawe::Engine::try_update_camera_target(entt::registry &reg, entt::entity e) -> void
{
    if (const auto has_parent = reg.try_get<Parent>(e); has_parent != nullptr) {
        if (const auto is_camera = reg.try_get<CameraData>(has_parent->component); is_camera != nullptr) {
            update_camera(reg, has_parent->component);
        }
    }
}

auto kawe::Engine::update_camera_event(const event::TimeElapsed &e) -> void
{
    const auto dt_nano = e.elapsed;
    const auto dt_secs =
        static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(dt_nano).count()) / 1'000'000.0;

    if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow)) {
        for (auto &camera : world.view<CameraData>()) {
            const auto &data = world.get<CameraData>(camera);
            const auto viewport = data.viewport;
            const auto window_size = window->getSize<double>();

            if (!Rect4<double>{
                    static_cast<double>(viewport.x) * window_size.x,
                    static_cast<double>(viewport.y) * window_size.y,
                    static_cast<double>(viewport.w) * window_size.x,
                    static_cast<double>(viewport.h) * window_size.y}
                     .contains(ctx->mouse_pos)) {
                continue;
            }

            for (const auto &[button, pressed] : ctx->state_mouse_button) {
                if (!pressed) { continue; }

                const auto ms = dt_secs * 1'000.0;
                const auto size = window->getSize<double>() * glm::dvec2{data.viewport.w, data.viewport.h};

                switch (button) {
                case event::MouseButton::Button::BUTTON_LEFT: {
                    const auto amount_x = (ctx->mouse_pos.x - ctx->mouse_pos_when_pressed.x) / size.x;
                    const auto amount_y = (ctx->mouse_pos_when_pressed.y - ctx->mouse_pos.y) / size.y;
                    CameraData::rotate(world, camera, data, {amount_x * ms, amount_y * ms});
                } break;
                case event::MouseButton::Button::BUTTON_MIDDLE: {
                    const auto amount = (ctx->mouse_pos_when_pressed.y - ctx->mouse_pos.y) / size.y;
                    CameraData::zoom(world, camera, data, amount * ms);
                } break;
                case event::MouseButton::Button::BUTTON_RIGHT: {
                    const auto amount_x = (ctx->mouse_pos.x - ctx->mouse_pos_when_pressed.x) / size.x;
                    const auto amount_y = (ctx->mouse_pos_when_pressed.y - ctx->mouse_pos.y) / size.y;
                    CameraData::translate(
                        world,
                        camera,
                        data,
                        {-amount_x * ms, -amount_y * ms},
                        !ctx->keyboard_state[event::Key::Code::KEY_LEFT_CONTROL]);
                } break;
                default: break;
                }
            }
        }
    }
}

auto kawe::Engine::system_rendering() -> void
{
    const auto render = [this]<bool has_ebo, bool has_texture, bool is_pickable>(
                            const CameraData &cam,
                            [[maybe_unused]] const entt::entity &e,
                            const Render::VAO &vao,
                            const Position3f &pos,
                            const Rotation3f &rot,
                            const Scale3f &scale,
                            const Texture2D &texture) {
        auto model = glm::dmat4(1.0);
        model = glm::translate(model, pos.component);
        model = glm::rotate(model, glm::radians(rot.component.x), glm::dvec3(1.0, 0.0, 0.0));
        model = glm::rotate(model, glm::radians(rot.component.y), glm::dvec3(0.0, 1.0, 0.0));
        model = glm::rotate(model, glm::radians(rot.component.z), glm::dvec3(0.0, 0.0, 1.0));
        model = glm::scale(model, scale.component);

        if constexpr (!is_pickable) {
            // note : not optimized at all !!! bad bad bad
            // or is it ?
            vao.shader_program->use();
            vao.shader_program->setUniform("view", cam.view);
            vao.shader_program->setUniform("projection", cam.projection);
            vao.shader_program->setUniform("model", model);

            if constexpr (has_texture) {
                CALL_OPEN_GL(glBindTexture(GL_TEXTURE_2D, texture.textureID));

                // setting light properties.
                auto light_count = static_cast<unsigned int>(world.size<PointLight>());
                vao.shader_program->setUniform("lightCount", light_count);

                if (light_count) {
                    // ! using fmt, find another way of pushing uniform arrays.
                    auto lights = world.view<PointLight>();
                    for (auto it = lights.begin(); it != lights.end(); ++it) {
                        auto index = it - lights.begin();

                        auto &light_pos = world.get<kawe::Position3f>(*it);
                        auto &light = world.get<PointLight>(*it);

                        vao.shader_program->setUniform(
                            fmt::format("pointLights[{}].position", index), light_pos.component);
                        vao.shader_program->setUniform(
                            fmt::format("pointLights[{}].intensity", index), light.intensity);
                        vao.shader_program->setUniform(
                            fmt::format("pointLights[{}].DiffuseColor", index), light.diffuse_color);
                        vao.shader_program->setUniform(
                            fmt::format("pointLights[{}].SpecularColor", index), light.specular_color);
                    }
                }
            }
        } else {
            const auto found = std::find_if(ctx->shaders.begin(), ctx->shaders.end(), [](auto &shader) {
                return shader->getName() == "picking";
            });

            assert(found != ctx->shaders.end());

            const auto r = static_cast<double>((static_cast<std::uint32_t>(e) & 0x000000FFu) >> 0u);
            const auto g = static_cast<double>((static_cast<std::uint32_t>(e) & 0x0000FF00u) >> 8u);
            const auto b = static_cast<double>((static_cast<std::uint32_t>(e) & 0x00FF0000u) >> 16u);

            (*found)->use();
            (*found)->setUniform("view", cam.view);
            (*found)->setUniform("projection", cam.projection);
            (*found)->setUniform("model", model);
            (*found)->setUniform("object_color", glm::dvec4{r / 255.0, g / 255.0, b / 255.0, 1.0});
        }

        CALL_OPEN_GL(::glBindVertexArray(vao.object));
        if constexpr (has_ebo) {
            CALL_OPEN_GL(::glDrawElements(static_cast<GLenum>(vao.mode), vao.count, GL_UNSIGNED_INT, 0));
        } else {
            CALL_OPEN_GL(::glDrawArrays(static_cast<GLenum>(vao.mode), 0, vao.count));
        }

        if constexpr (!is_pickable && has_texture) { CALL_OPEN_GL(glBindTexture(GL_TEXTURE_2D, 0)); }
    };

    const auto render_all = [ this, &render ]<typename... With>(const CameraData &cam)
    {
        constexpr auto is_pickable = sizeof...(With) != 0;

        // note : it should be a better way..
        // todo create a matrix of callback templated somthing something

        // without texture
        // without ebo

        for (const entt::entity &e : world.view<Render::VAO, With...>(
                 entt::exclude<Render::EBO, Position3f, Rotation3f, Scale3f, Texture2D>)) {
            const auto &vao = world.get<Render::VAO>(e);
            render.operator()<false, false, is_pickable>(
                cam, e, vao, Position3f{}, Rotation3f{}, Scale3f{}, Texture2D::empty);
        }

        for (const entt::entity &e : world.view<Render::VAO, Position3f, With...>(
                 entt::exclude<Render::EBO, Rotation3f, Scale3f, Texture2D>)) {
            const auto &vao = world.get<Render::VAO>(e);
            const auto &pos = world.get<Position3f>(e);
            render.operator()<false, false, is_pickable>(
                cam, e, vao, pos, Rotation3f{}, Scale3f{}, Texture2D::empty);
        }

        for (const entt::entity &e : world.view<Render::VAO, Rotation3f, With...>(
                 entt::exclude<Render::EBO, Position3f, Scale3f, Texture2D>)) {
            const auto &vao = world.get<Render::VAO>(e);
            const auto &rot = world.get<Rotation3f>(e);
            render.operator()<false, false, is_pickable>(
                cam, e, vao, Position3f{}, rot, Scale3f{}, Texture2D::empty);
        }

        for (const entt::entity &e : world.view<Render::VAO, Scale3f, With...>(
                 entt::exclude<Render::EBO, Position3f, Rotation3f, Texture2D>)) {
            const auto &vao = world.get<Render::VAO>(e);
            const auto &scale = world.get<Scale3f>(e);
            render.operator()<false, false, is_pickable>(
                cam, e, vao, Position3f{}, Rotation3f{}, scale, Texture2D::empty);
        }

        for (const entt::entity &e : world.view<Render::VAO, Position3f, Scale3f, With...>(
                 entt::exclude<Render::EBO, Rotation3f, Texture2D>)) {
            const auto &vao = world.get<Render::VAO>(e);
            const auto &pos = world.get<Position3f>(e);
            const auto &scale = world.get<Scale3f>(e);
            render.operator()<false, false, is_pickable>(cam, e, vao, pos, Rotation3f{}, scale, Texture2D::empty);
        }

        for (const entt::entity &e : world.view<Render::VAO, Rotation3f, Scale3f, With...>(
                 entt::exclude<Render::EBO, Position3f, Texture2D>)) {
            const auto &vao = world.get<Render::VAO>(e);
            const auto &scale = world.get<Scale3f>(e);
            const auto &rot = world.get<Rotation3f>(e);
            render.operator()<false, false, is_pickable>(cam, e, vao, Position3f{}, rot, scale, Texture2D::empty);
        }

        for (const entt::entity &e : world.view<Render::VAO, Position3f, Rotation3f, With...>(
                 entt::exclude<Render::EBO, Scale3f, Texture2D>)) {
            const auto &vao = world.get<Render::VAO>(e);
            const auto &pos = world.get<Position3f>(e);
            const auto &rot = world.get<Rotation3f>(e);
            render.operator()<false, false, is_pickable>(cam, e, vao, pos, rot, Scale3f{}, Texture2D::empty);
        }

        for (const entt::entity &e : world.view<Render::VAO, Position3f, Rotation3f, Scale3f, With...>(
                 entt::exclude<Render::EBO, Texture2D>)) {
            const auto &vao = world.get<Render::VAO>(e);
            const auto &pos = world.get<Position3f>(e);
            const auto &scale = world.get<Scale3f>(e);
            const auto &rot = world.get<Rotation3f>(e);
            render.operator()<false, false, is_pickable>(cam, e, vao, pos, rot, scale, Texture2D::empty);
        }

        // with ebo

        for (const entt::entity &e : world.view<Render::EBO, Render::VAO, With...>(
                 entt::exclude<Position3f, Rotation3f, Scale3f, Texture2D>)) {
            const auto &vao = world.get<Render::VAO>(e);
            render.operator()<true, false, is_pickable>(
                cam, e, vao, Position3f{}, Rotation3f{}, Scale3f{}, Texture2D::empty);
        }

        for (const entt::entity &e : world.view<Render::EBO, Render::VAO, Position3f, With...>(
                 entt::exclude<Rotation3f, Scale3f, Texture2D>)) {
            const auto &vao = world.get<Render::VAO>(e);
            const auto &pos = world.get<Position3f>(e);
            render.operator()<true, false, is_pickable>(
                cam, e, vao, pos, Rotation3f{}, Scale3f{}, Texture2D::empty);
        }

        for (const entt::entity &e : world.view<Render::EBO, Render::VAO, Rotation3f, With...>(
                 entt::exclude<Position3f, Scale3f, Texture2D>)) {
            const auto &vao = world.get<Render::VAO>(e);
            const auto &rot = world.get<Rotation3f>(e);
            render.operator()<true, false, is_pickable>(
                cam, e, vao, Position3f{}, rot, Scale3f{}, Texture2D::empty);
        }

        for (const entt::entity &e : world.view<Render::EBO, Render::VAO, Scale3f, With...>(
                 entt::exclude<Position3f, Rotation3f, Texture2D>)) {
            const auto &vao = world.get<Render::VAO>(e);
            const auto &scale = world.get<Scale3f>(e);
            render.operator()<true, false, is_pickable>(
                cam, e, vao, Position3f{}, Rotation3f{}, scale, Texture2D::empty);
        }

        for (const entt::entity &e : world.view<Render::EBO, Render::VAO, Position3f, Scale3f, With...>(
                 entt::exclude<Rotation3f, Texture2D>)) {
            const auto &vao = world.get<Render::VAO>(e);
            const auto &pos = world.get<Position3f>(e);
            const auto &scale = world.get<Scale3f>(e);
            render.operator()<true, false, is_pickable>(cam, e, vao, pos, Rotation3f{}, scale, Texture2D::empty);
        }

        for (const entt::entity &e : world.view<Render::EBO, Render::VAO, Rotation3f, Scale3f, With...>(
                 entt::exclude<Position3f, Texture2D>)) {
            const auto &vao = world.get<Render::VAO>(e);
            const auto &scale = world.get<Scale3f>(e);
            const auto &rot = world.get<Rotation3f>(e);
            render.operator()<true, false, is_pickable>(cam, e, vao, Position3f{}, rot, scale, Texture2D::empty);
        }

        for (const entt::entity &e : world.view<Render::EBO, Render::VAO, Position3f, Rotation3f, With...>(
                 entt::exclude<Scale3f, Texture2D>)) {
            const auto &vao = world.get<Render::VAO>(e);
            const auto &pos = world.get<Position3f>(e);
            const auto &rot = world.get<Rotation3f>(e);
            render.operator()<true, false, is_pickable>(cam, e, vao, pos, rot, Scale3f{}, Texture2D::empty);
        }

        for (const entt::entity &e :
             world.view<Render::EBO, Render::VAO, Position3f, Rotation3f, Scale3f, With...>(
                 entt::exclude<Texture2D>)) {
            const auto &vao = world.get<Render::VAO>(e);
            const auto &pos = world.get<Position3f>(e);
            const auto &scale = world.get<Scale3f>(e);
            const auto &rot = world.get<Rotation3f>(e);
            render.operator()<true, false, is_pickable>(cam, e, vao, pos, rot, scale, Texture2D::empty);
        }

        // with texture
        // without ebo

        for (const auto &e : world.view<With..., Render::VAO, Texture2D>(
                 entt::exclude<Render::EBO, Position3f, Rotation3f, Scale3f>)) {
            const auto &vao = world.get<Render::VAO>(e);
            const auto &texture = world.get<Texture2D>(e);
            render.operator()<false, true, is_pickable>(
                cam, e, vao, Position3f{}, Rotation3f{}, Scale3f{}, texture);
        }

        for (const auto &e : world.view<With..., Render::VAO, Position3f, Texture2D>(
                 entt::exclude<Render::EBO, Rotation3f, Scale3f>)) {
            const auto &vao = world.get<Render::VAO>(e);
            const auto &pos = world.get<Position3f>(e);
            const auto &texture = world.get<Texture2D>(e);
            render.operator()<false, true, is_pickable>(cam, e, vao, pos, Rotation3f{}, Scale3f{}, texture);
        }

        for (const auto &e : world.view<With..., Render::VAO, Rotation3f, Texture2D>(
                 entt::exclude<Render::EBO, Position3f, Scale3f>)) {
            const auto &vao = world.get<Render::VAO>(e);
            const auto &rot = world.get<Rotation3f>(e);
            const auto &texture = world.get<Texture2D>(e);
            render.operator()<false, true, is_pickable>(cam, e, vao, Position3f{}, rot, Scale3f{}, texture);
        }

        for (const auto &e : world.view<With..., Render::VAO, Scale3f, Texture2D>(
                 entt::exclude<Render::EBO, Position3f, Rotation3f>)) {
            const auto &vao = world.get<Render::VAO>(e);
            const auto &scale = world.get<Scale3f>(e);
            const auto &texture = world.get<Texture2D>(e);
            render.operator()<false, true, is_pickable>(cam, e, vao, Position3f{}, Rotation3f{}, scale, texture);
        }

        for (const auto &e : world.view<With..., Render::VAO, Position3f, Scale3f, Texture2D>(
                 entt::exclude<Render::EBO, Rotation3f>)) {
            const auto &vao = world.get<Render::VAO>(e);
            const auto &pos = world.get<Position3f>(e);
            const auto &scale = world.get<Scale3f>(e);
            const auto &texture = world.get<Texture2D>(e);
            render.operator()<false, true, is_pickable>(cam, e, vao, pos, Rotation3f{}, scale, texture);
        }

        for (const auto &e : world.view<With..., Render::VAO, Rotation3f, Scale3f, Texture2D>(
                 entt::exclude<Render::EBO, Position3f>)) {
            const auto &vao = world.get<Render::VAO>(e);
            const auto &scale = world.get<Scale3f>(e);
            const auto &rot = world.get<Rotation3f>(e);
            const auto &texture = world.get<Texture2D>(e);
            render.operator()<false, true, is_pickable>(cam, e, vao, Position3f{}, rot, scale, texture);
        }

        for (const auto &e : world.view<With..., Render::VAO, Position3f, Rotation3f, Texture2D>(
                 entt::exclude<Render::EBO, Scale3f>)) {
            const auto &vao = world.get<Render::VAO>(e);
            const auto &pos = world.get<Position3f>(e);
            const auto &rot = world.get<Rotation3f>(e);
            const auto &texture = world.get<Texture2D>(e);
            render.operator()<false, true, is_pickable>(cam, e, vao, pos, rot, Scale3f{}, texture);
        }

        for (const auto &e : world.view<With..., Render::VAO, Position3f, Rotation3f, Scale3f, Texture2D>(
                 entt::exclude<Render::EBO>)) {
            const auto &vao = world.get<Render::VAO>(e);
            const auto &pos = world.get<Position3f>(e);
            const auto &scale = world.get<Scale3f>(e);
            const auto &rot = world.get<Rotation3f>(e);
            const auto &texture = world.get<Texture2D>(e);
            render.operator()<false, true, is_pickable>(cam, e, vao, pos, rot, scale, texture);
        }

        // with ebo

        for (const auto &e : world.view<With..., Render::EBO, Render::VAO, Texture2D>(
                 entt::exclude<Position3f, Rotation3f, Scale3f>)) {
            const auto &vao = world.get<Render::VAO>(e);
            const auto &texture = world.get<Texture2D>(e);
            render.operator()<true, true, is_pickable>(
                cam, e, vao, Position3f{}, Rotation3f{}, Scale3f{}, texture);
        }

        for (const auto &e : world.view<With..., Render::EBO, Render::VAO, Position3f, Texture2D>(
                 entt::exclude<Rotation3f, Scale3f>)) {
            const auto &vao = world.get<Render::VAO>(e);
            const auto &pos = world.get<Position3f>(e);
            const auto &texture = world.get<Texture2D>(e);
            render.operator()<true, true, is_pickable>(cam, e, vao, pos, Rotation3f{}, Scale3f{}, texture);
        }

        for (const auto &e : world.view<With..., Render::EBO, Render::VAO, Rotation3f, Texture2D>(
                 entt::exclude<Position3f, Scale3f>)) {
            const auto &vao = world.get<Render::VAO>(e);
            const auto &rot = world.get<Rotation3f>(e);
            const auto &texture = world.get<Texture2D>(e);
            render.operator()<true, true, is_pickable>(cam, e, vao, Position3f{}, rot, Scale3f{}, texture);
        }

        for (const auto &e : world.view<With..., Render::EBO, Render::VAO, Scale3f, Texture2D>(
                 entt::exclude<Position3f, Rotation3f>)) {
            const auto &vao = world.get<Render::VAO>(e);
            const auto &scale = world.get<Scale3f>(e);
            const auto &texture = world.get<Texture2D>(e);
            render.operator()<true, true, is_pickable>(cam, e, vao, Position3f{}, Rotation3f{}, scale, texture);
        }

        for (const auto &e : world.view<With..., Render::EBO, Render::VAO, Position3f, Scale3f, Texture2D>(
                 entt::exclude<Rotation3f>)) {
            const auto &vao = world.get<Render::VAO>(e);
            const auto &pos = world.get<Position3f>(e);
            const auto &scale = world.get<Scale3f>(e);
            const auto &texture = world.get<Texture2D>(e);
            render.operator()<true, true, is_pickable>(cam, e, vao, pos, Rotation3f{}, scale, texture);
        }

        for (const auto &e : world.view<With..., Render::EBO, Render::VAO, Rotation3f, Scale3f, Texture2D>(
                 entt::exclude<Position3f>)) {
            const auto &vao = world.get<Render::VAO>(e);
            const auto &scale = world.get<Scale3f>(e);
            const auto &rot = world.get<Rotation3f>(e);
            const auto &texture = world.get<Texture2D>(e);
            render.operator()<true, true, is_pickable>(cam, e, vao, Position3f{}, rot, scale, texture);
        }

        for (const auto &e : world.view<With..., Render::EBO, Render::VAO, Position3f, Rotation3f, Texture2D>(
                 entt::exclude<Scale3f>)) {
            const auto &vao = world.get<Render::VAO>(e);
            const auto &pos = world.get<Position3f>(e);
            const auto &rot = world.get<Rotation3f>(e);
            const auto &texture = world.get<Texture2D>(e);
            render.operator()<true, true, is_pickable>(cam, e, vao, pos, rot, Scale3f{}, texture);
        }

        for (const auto &e :
             world.view<With..., Render::EBO, Render::VAO, Position3f, Rotation3f, Scale3f, Texture2D>(
                 entt::exclude<>)) {
            const auto &vao = world.get<Render::VAO>(e);
            const auto &pos = world.get<Position3f>(e);
            const auto &scale = world.get<Scale3f>(e);
            const auto &rot = world.get<Rotation3f>(e);
            const auto &texture = world.get<Texture2D>(e);
            render.operator()<true, true, is_pickable>(cam, e, vao, pos, rot, scale, texture);
        }
    };

    if (ctx->state_mouse_button[event::MouseButton::Button::BUTTON_LEFT]
        && !ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow)) {
        const auto &list_pickable = world.view<Pickable, Render::VAO>();
        if (list_pickable.size_hint() != 0) {
            glClearColor(1, 1, 1, 1);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            for (auto &i : world.view<CameraData>()) {
                const auto &camera = world.get<CameraData>(i);
                const auto &cam_viewport = camera.viewport;
                const auto window_size = window->getSize<float>();
                GLint viewport[4] = {
                    static_cast<GLint>(cam_viewport.x * window_size.x),
                    static_cast<GLint>(cam_viewport.y * window_size.y),
                    static_cast<GLsizei>(cam_viewport.w * window_size.x),
                    static_cast<GLsizei>(cam_viewport.h * window_size.y)};
                ::glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

                render_all.operator()<Pickable>(camera);
            }
            // note : is it required ?
            glFlush();
            glFinish();

            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            std::array<std::uint8_t, 4> data{0, 0, 0, 0};
            glReadPixels(
                static_cast<GLint>(ctx->mouse_pos_when_pressed.x),
                static_cast<GLint>(ctx->mouse_pos_when_pressed.y),
                1,
                1,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                data.data());

            const int pickedID = data[0] + data[1] * 256 + data[2] * 256 * 256;
            spdlog::debug("pick = {}", pickedID);
            if (pickedID == 0x00ffffff || !world.valid(static_cast<entt::entity>(pickedID))) {
                component_inspector.selected = {};
            } else {
                component_inspector.selected = static_cast<entt::entity>(pickedID);
                world.patch<Pickable>(
                    static_cast<entt::entity>(pickedID), [](auto &pickable) { pickable.is_picked = false; });
            }

            // todo : send a signal to the app ?
        }
    }
// #define SHOW_THE_PICK_IMAGE
#ifdef SHOW_THE_PICK_IMAGE
    // this will render during one frame what the engine see when u try to pick an object
    else {
#endif
        glClearColor(ctx->clear_color.r, ctx->clear_color.g, ctx->clear_color.b, ctx->clear_color.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for (auto &i : world.view<CameraData>()) {
            const auto &camera = world.get<CameraData>(i);
            // todo : when resizing the window, the object deform
            // this doesn t sound kind right ...
            const auto cam_viewport = camera.viewport;
            const auto window_size = window->getSize<float>();
            GLint viewport[4] = {
                static_cast<GLint>(cam_viewport.x * window_size.x),
                static_cast<GLint>(cam_viewport.y * window_size.y),
                static_cast<GLsizei>(cam_viewport.w * window_size.x),
                static_cast<GLsizei>(cam_viewport.h * window_size.y)};
            ::glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

            render_all(camera);
        }

#ifdef SHOW_THE_PICK_IMAGE
    }
#endif
}
