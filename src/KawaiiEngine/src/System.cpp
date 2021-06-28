#include "component.hpp"
#include "Event.hpp"
#include "System.hpp"

auto kawe::System::on_time_elapsed_render(const action::Render<Render::Layout::SCENE> &) -> void
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
                auto light_count = static_cast<unsigned int>(my_world.size<PointLight>());
                vao.shader_program->setUniform("lightCount", light_count);

                if (light_count) {
                    // ! using fmt, find another way of pushing uniform arrays.
                    auto lights = my_world.view<PointLight>();
                    for (auto it = lights.begin(); it != lights.end(); ++it) {
                        auto index = it - lights.begin();

                        auto &light_pos = my_world.get<kawe::Position3f>(*it);
                        auto &light = my_world.get<PointLight>(*it);

                        vao.shader_program->setUniform(fmt::format("pointLights[{}].position", index), light_pos.component);
                        vao.shader_program->setUniform(fmt::format("pointLights[{}].intensity", index), light.intensity);
                        vao.shader_program->setUniform(
                            fmt::format("pointLights[{}].DiffuseColor", index), light.diffuse_color);
                        vao.shader_program->setUniform(
                            fmt::format("pointLights[{}].SpecularColor", index), light.specular_color);
                    }
                }
            }
        } else {
            const auto found = std::find_if(
                ctx.shaders.begin(), ctx.shaders.end(), [](auto &shader) { return shader->getName() == "picking"; });

            assert(found != ctx.shaders.end());

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

        for (const entt::entity &e :
             my_world.view<Render::VAO, With...>(entt::exclude<Render::EBO, Position3f, Rotation3f, Scale3f, Texture2D>)) {
            const auto &vao = my_world.get<Render::VAO>(e);
            render.operator()<false, false, is_pickable>(
                cam, e, vao, Position3f{}, Rotation3f{}, Scale3f{}, Texture2D::empty);
        }

        for (const entt::entity &e :
             my_world.view<Render::VAO, Position3f, With...>(entt::exclude<Render::EBO, Rotation3f, Scale3f, Texture2D>)) {
            const auto &vao = my_world.get<Render::VAO>(e);
            const auto &pos = my_world.get<Position3f>(e);
            render.operator()<false, false, is_pickable>(cam, e, vao, pos, Rotation3f{}, Scale3f{}, Texture2D::empty);
        }

        for (const entt::entity &e :
             my_world.view<Render::VAO, Rotation3f, With...>(entt::exclude<Render::EBO, Position3f, Scale3f, Texture2D>)) {
            const auto &vao = my_world.get<Render::VAO>(e);
            const auto &rot = my_world.get<Rotation3f>(e);
            render.operator()<false, false, is_pickable>(cam, e, vao, Position3f{}, rot, Scale3f{}, Texture2D::empty);
        }

        for (const entt::entity &e :
             my_world.view<Render::VAO, Scale3f, With...>(entt::exclude<Render::EBO, Position3f, Rotation3f, Texture2D>)) {
            const auto &vao = my_world.get<Render::VAO>(e);
            const auto &scale = my_world.get<Scale3f>(e);
            render.operator()<false, false, is_pickable>(cam, e, vao, Position3f{}, Rotation3f{}, scale, Texture2D::empty);
        }

        for (const entt::entity &e :
             my_world.view<Render::VAO, Position3f, Scale3f, With...>(entt::exclude<Render::EBO, Rotation3f, Texture2D>)) {
            const auto &vao = my_world.get<Render::VAO>(e);
            const auto &pos = my_world.get<Position3f>(e);
            const auto &scale = my_world.get<Scale3f>(e);
            render.operator()<false, false, is_pickable>(cam, e, vao, pos, Rotation3f{}, scale, Texture2D::empty);
        }

        for (const entt::entity &e :
             my_world.view<Render::VAO, Rotation3f, Scale3f, With...>(entt::exclude<Render::EBO, Position3f, Texture2D>)) {
            const auto &vao = my_world.get<Render::VAO>(e);
            const auto &scale = my_world.get<Scale3f>(e);
            const auto &rot = my_world.get<Rotation3f>(e);
            render.operator()<false, false, is_pickable>(cam, e, vao, Position3f{}, rot, scale, Texture2D::empty);
        }

        for (const entt::entity &e :
             my_world.view<Render::VAO, Position3f, Rotation3f, With...>(entt::exclude<Render::EBO, Scale3f, Texture2D>)) {
            const auto &vao = my_world.get<Render::VAO>(e);
            const auto &pos = my_world.get<Position3f>(e);
            const auto &rot = my_world.get<Rotation3f>(e);
            render.operator()<false, false, is_pickable>(cam, e, vao, pos, rot, Scale3f{}, Texture2D::empty);
        }

        for (const entt::entity &e :
             my_world.view<Render::VAO, Position3f, Rotation3f, Scale3f, With...>(entt::exclude<Render::EBO, Texture2D>)) {
            const auto &vao = my_world.get<Render::VAO>(e);
            const auto &pos = my_world.get<Position3f>(e);
            const auto &scale = my_world.get<Scale3f>(e);
            const auto &rot = my_world.get<Rotation3f>(e);
            render.operator()<false, false, is_pickable>(cam, e, vao, pos, rot, scale, Texture2D::empty);
        }

        // with ebo

        for (const entt::entity &e :
             my_world.view<Render::EBO, Render::VAO, With...>(entt::exclude<Position3f, Rotation3f, Scale3f, Texture2D>)) {
            const auto &vao = my_world.get<Render::VAO>(e);
            render.operator()<true, false, is_pickable>(
                cam, e, vao, Position3f{}, Rotation3f{}, Scale3f{}, Texture2D::empty);
        }

        for (const entt::entity &e :
             my_world.view<Render::EBO, Render::VAO, Position3f, With...>(entt::exclude<Rotation3f, Scale3f, Texture2D>)) {
            const auto &vao = my_world.get<Render::VAO>(e);
            const auto &pos = my_world.get<Position3f>(e);
            render.operator()<true, false, is_pickable>(cam, e, vao, pos, Rotation3f{}, Scale3f{}, Texture2D::empty);
        }

        for (const entt::entity &e :
             my_world.view<Render::EBO, Render::VAO, Rotation3f, With...>(entt::exclude<Position3f, Scale3f, Texture2D>)) {
            const auto &vao = my_world.get<Render::VAO>(e);
            const auto &rot = my_world.get<Rotation3f>(e);
            render.operator()<true, false, is_pickable>(cam, e, vao, Position3f{}, rot, Scale3f{}, Texture2D::empty);
        }

        for (const entt::entity &e :
             my_world.view<Render::EBO, Render::VAO, Scale3f, With...>(entt::exclude<Position3f, Rotation3f, Texture2D>)) {
            const auto &vao = my_world.get<Render::VAO>(e);
            const auto &scale = my_world.get<Scale3f>(e);
            render.operator()<true, false, is_pickable>(cam, e, vao, Position3f{}, Rotation3f{}, scale, Texture2D::empty);
        }

        for (const entt::entity &e :
             my_world.view<Render::EBO, Render::VAO, Position3f, Scale3f, With...>(entt::exclude<Rotation3f, Texture2D>)) {
            const auto &vao = my_world.get<Render::VAO>(e);
            const auto &pos = my_world.get<Position3f>(e);
            const auto &scale = my_world.get<Scale3f>(e);
            render.operator()<true, false, is_pickable>(cam, e, vao, pos, Rotation3f{}, scale, Texture2D::empty);
        }

        for (const entt::entity &e :
             my_world.view<Render::EBO, Render::VAO, Rotation3f, Scale3f, With...>(entt::exclude<Position3f, Texture2D>)) {
            const auto &vao = my_world.get<Render::VAO>(e);
            const auto &scale = my_world.get<Scale3f>(e);
            const auto &rot = my_world.get<Rotation3f>(e);
            render.operator()<true, false, is_pickable>(cam, e, vao, Position3f{}, rot, scale, Texture2D::empty);
        }

        for (const entt::entity &e :
             my_world.view<Render::EBO, Render::VAO, Position3f, Rotation3f, With...>(entt::exclude<Scale3f, Texture2D>)) {
            const auto &vao = my_world.get<Render::VAO>(e);
            const auto &pos = my_world.get<Position3f>(e);
            const auto &rot = my_world.get<Rotation3f>(e);
            render.operator()<true, false, is_pickable>(cam, e, vao, pos, rot, Scale3f{}, Texture2D::empty);
        }

        for (const entt::entity &e :
             my_world.view<Render::EBO, Render::VAO, Position3f, Rotation3f, Scale3f, With...>(entt::exclude<Texture2D>)) {
            const auto &vao = my_world.get<Render::VAO>(e);
            const auto &pos = my_world.get<Position3f>(e);
            const auto &scale = my_world.get<Scale3f>(e);
            const auto &rot = my_world.get<Rotation3f>(e);
            render.operator()<true, false, is_pickable>(cam, e, vao, pos, rot, scale, Texture2D::empty);
        }

        // with texture
        // without ebo

        for (const auto &e :
             my_world.view<With..., Render::VAO, Texture2D>(entt::exclude<Render::EBO, Position3f, Rotation3f, Scale3f>)) {
            const auto &vao = my_world.get<Render::VAO>(e);
            const auto &texture = my_world.get<Texture2D>(e);
            render.operator()<false, true, is_pickable>(cam, e, vao, Position3f{}, Rotation3f{}, Scale3f{}, texture);
        }

        for (const auto &e :
             my_world.view<With..., Render::VAO, Position3f, Texture2D>(entt::exclude<Render::EBO, Rotation3f, Scale3f>)) {
            const auto &vao = my_world.get<Render::VAO>(e);
            const auto &pos = my_world.get<Position3f>(e);
            const auto &texture = my_world.get<Texture2D>(e);
            render.operator()<false, true, is_pickable>(cam, e, vao, pos, Rotation3f{}, Scale3f{}, texture);
        }

        for (const auto &e :
             my_world.view<With..., Render::VAO, Rotation3f, Texture2D>(entt::exclude<Render::EBO, Position3f, Scale3f>)) {
            const auto &vao = my_world.get<Render::VAO>(e);
            const auto &rot = my_world.get<Rotation3f>(e);
            const auto &texture = my_world.get<Texture2D>(e);
            render.operator()<false, true, is_pickable>(cam, e, vao, Position3f{}, rot, Scale3f{}, texture);
        }

        for (const auto &e :
             my_world.view<With..., Render::VAO, Scale3f, Texture2D>(entt::exclude<Render::EBO, Position3f, Rotation3f>)) {
            const auto &vao = my_world.get<Render::VAO>(e);
            const auto &scale = my_world.get<Scale3f>(e);
            const auto &texture = my_world.get<Texture2D>(e);
            render.operator()<false, true, is_pickable>(cam, e, vao, Position3f{}, Rotation3f{}, scale, texture);
        }

        for (const auto &e :
             my_world.view<With..., Render::VAO, Position3f, Scale3f, Texture2D>(entt::exclude<Render::EBO, Rotation3f>)) {
            const auto &vao = my_world.get<Render::VAO>(e);
            const auto &pos = my_world.get<Position3f>(e);
            const auto &scale = my_world.get<Scale3f>(e);
            const auto &texture = my_world.get<Texture2D>(e);
            render.operator()<false, true, is_pickable>(cam, e, vao, pos, Rotation3f{}, scale, texture);
        }

        for (const auto &e :
             my_world.view<With..., Render::VAO, Rotation3f, Scale3f, Texture2D>(entt::exclude<Render::EBO, Position3f>)) {
            const auto &vao = my_world.get<Render::VAO>(e);
            const auto &scale = my_world.get<Scale3f>(e);
            const auto &rot = my_world.get<Rotation3f>(e);
            const auto &texture = my_world.get<Texture2D>(e);
            render.operator()<false, true, is_pickable>(cam, e, vao, Position3f{}, rot, scale, texture);
        }

        for (const auto &e :
             my_world.view<With..., Render::VAO, Position3f, Rotation3f, Texture2D>(entt::exclude<Render::EBO, Scale3f>)) {
            const auto &vao = my_world.get<Render::VAO>(e);
            const auto &pos = my_world.get<Position3f>(e);
            const auto &rot = my_world.get<Rotation3f>(e);
            const auto &texture = my_world.get<Texture2D>(e);
            render.operator()<false, true, is_pickable>(cam, e, vao, pos, rot, Scale3f{}, texture);
        }

        for (const auto &e :
             my_world.view<With..., Render::VAO, Position3f, Rotation3f, Scale3f, Texture2D>(entt::exclude<Render::EBO>)) {
            const auto &vao = my_world.get<Render::VAO>(e);
            const auto &pos = my_world.get<Position3f>(e);
            const auto &scale = my_world.get<Scale3f>(e);
            const auto &rot = my_world.get<Rotation3f>(e);
            const auto &texture = my_world.get<Texture2D>(e);
            render.operator()<false, true, is_pickable>(cam, e, vao, pos, rot, scale, texture);
        }

        // with ebo

        for (const auto &e :
             my_world.view<With..., Render::EBO, Render::VAO, Texture2D>(entt::exclude<Position3f, Rotation3f, Scale3f>)) {
            const auto &vao = my_world.get<Render::VAO>(e);
            const auto &texture = my_world.get<Texture2D>(e);
            render.operator()<true, true, is_pickable>(cam, e, vao, Position3f{}, Rotation3f{}, Scale3f{}, texture);
        }

        for (const auto &e :
             my_world.view<With..., Render::EBO, Render::VAO, Position3f, Texture2D>(entt::exclude<Rotation3f, Scale3f>)) {
            const auto &vao = my_world.get<Render::VAO>(e);
            const auto &pos = my_world.get<Position3f>(e);
            const auto &texture = my_world.get<Texture2D>(e);
            render.operator()<true, true, is_pickable>(cam, e, vao, pos, Rotation3f{}, Scale3f{}, texture);
        }

        for (const auto &e :
             my_world.view<With..., Render::EBO, Render::VAO, Rotation3f, Texture2D>(entt::exclude<Position3f, Scale3f>)) {
            const auto &vao = my_world.get<Render::VAO>(e);
            const auto &rot = my_world.get<Rotation3f>(e);
            const auto &texture = my_world.get<Texture2D>(e);
            render.operator()<true, true, is_pickable>(cam, e, vao, Position3f{}, rot, Scale3f{}, texture);
        }

        for (const auto &e :
             my_world.view<With..., Render::EBO, Render::VAO, Scale3f, Texture2D>(entt::exclude<Position3f, Rotation3f>)) {
            const auto &vao = my_world.get<Render::VAO>(e);
            const auto &scale = my_world.get<Scale3f>(e);
            const auto &texture = my_world.get<Texture2D>(e);
            render.operator()<true, true, is_pickable>(cam, e, vao, Position3f{}, Rotation3f{}, scale, texture);
        }

        for (const auto &e :
             my_world.view<With..., Render::EBO, Render::VAO, Position3f, Scale3f, Texture2D>(entt::exclude<Rotation3f>)) {
            const auto &vao = my_world.get<Render::VAO>(e);
            const auto &pos = my_world.get<Position3f>(e);
            const auto &scale = my_world.get<Scale3f>(e);
            const auto &texture = my_world.get<Texture2D>(e);
            render.operator()<true, true, is_pickable>(cam, e, vao, pos, Rotation3f{}, scale, texture);
        }

        for (const auto &e :
             my_world.view<With..., Render::EBO, Render::VAO, Rotation3f, Scale3f, Texture2D>(entt::exclude<Position3f>)) {
            const auto &vao = my_world.get<Render::VAO>(e);
            const auto &scale = my_world.get<Scale3f>(e);
            const auto &rot = my_world.get<Rotation3f>(e);
            const auto &texture = my_world.get<Texture2D>(e);
            render.operator()<true, true, is_pickable>(cam, e, vao, Position3f{}, rot, scale, texture);
        }

        for (const auto &e :
             my_world.view<With..., Render::EBO, Render::VAO, Position3f, Rotation3f, Texture2D>(entt::exclude<Scale3f>)) {
            const auto &vao = my_world.get<Render::VAO>(e);
            const auto &pos = my_world.get<Position3f>(e);
            const auto &rot = my_world.get<Rotation3f>(e);
            const auto &texture = my_world.get<Texture2D>(e);
            render.operator()<true, true, is_pickable>(cam, e, vao, pos, rot, Scale3f{}, texture);
        }

        for (const auto &e :
             my_world.view<With..., Render::EBO, Render::VAO, Position3f, Rotation3f, Scale3f, Texture2D>(entt::exclude<>)) {
            const auto &vao = my_world.get<Render::VAO>(e);
            const auto &pos = my_world.get<Position3f>(e);
            const auto &scale = my_world.get<Scale3f>(e);
            const auto &rot = my_world.get<Rotation3f>(e);
            const auto &texture = my_world.get<Texture2D>(e);
            render.operator()<true, true, is_pickable>(cam, e, vao, pos, rot, scale, texture);
        }
    };

    if (ctx.state_mouse_button[event::MouseButton::Button::BUTTON_LEFT]
        && !ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow)) {
        const auto &list_pickable = my_world.view<Pickable, Render::VAO>();
        if (list_pickable.size_hint() != 0) {
            glClearColor(1, 1, 1, 1);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            for (auto &i : my_world.view<CameraData>()) {
                const auto &camera = my_world.get<CameraData>(i);
                const auto &cam_viewport = camera.viewport;
                const auto window_size = window.getSize<float>();
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
                static_cast<GLint>(ctx.mouse_pos_when_pressed.x),
                static_cast<GLint>(ctx.mouse_pos_when_pressed.y),
                1,
                1,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                data.data());

            const int pickedID = data[0] + data[1] * 256 + data[2] * 256 * 256;
            spdlog::debug("pick = {}", pickedID);
            if (pickedID == 0x00ffffff || !my_world.valid(static_cast<entt::entity>(pickedID))) {
                // todo : trigger action is picked or something
                // component_inspector.selected = {};
            } else {
                // todo : trigger action is picked or something
                // component_inspector.selected = static_cast<entt::entity>(pickedID);
                my_world.patch<Pickable>(
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
        glClearColor(ctx.clear_color.r, ctx.clear_color.g, ctx.clear_color.b, ctx.clear_color.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for (auto &i : my_world.view<CameraData>()) {
            const auto &camera = my_world.get<CameraData>(i);
            // todo : when resizing the window, the object deform
            // this doesn t sound kind right ...
            const auto cam_viewport = camera.viewport;
            const auto window_size = window.getSize<float>();
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
