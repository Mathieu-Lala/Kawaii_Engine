#pragma once

#include <string_view>
#include <filesystem>
#include <string>
#include <variant>
#include <limits>
#include <functional>

#include <glm/glm.hpp>
#include <magic_enum.hpp>
#include <entt/entt.hpp>

#include <GL/glew.h>

#include "helpers/Rectangle.hpp"

#include "resources/ResourceLoader.hpp"
#include "State.hpp"

namespace kawe {

struct Children {
    static constexpr std::string_view name{"Children"};

    std::vector<entt::entity> component;
};

struct Parent {
    static constexpr std::string_view name{"Parent"};

    entt::entity component{entt::null};
};

struct Name {
    static constexpr std::string_view name{"Name"};

    std::string component;
};

template<std::size_t D, typename T>
struct Position {
    static constexpr std::string_view name{"Position"};

    glm::vec<static_cast<int>(D), T> component{0.0f, 0.0f, 0.0f};
};

using Position3f = Position<3, double>;

template<std::size_t D, typename T>
struct Rotation {
    static constexpr std::string_view name{"Rotation"};

    glm::vec<static_cast<int>(D), T> component{0.0f, 0.0f, 0.0f};
};

using Rotation3f = Rotation<3, double>;

template<std::size_t D, typename T>
struct Scale {
    static constexpr std::string_view name{"Scale"};

    glm::vec<static_cast<int>(D), T> component{1.0f, 1.0f, 1.0f};
};

using Scale3f = Scale<3, double>;


// using this because the VAO/VBO/EBO are referencing each others
struct Render {
    struct VAO {
        static constexpr std::string_view name{"VAO"};

        enum class DisplayMode : std::uint32_t {
            POINTS = GL_POINTS,
            LINE_STRIP = GL_LINE_STRIP,
            LINE_LOOP = GL_LINE_LOOP,
            LINES = GL_LINES,
            LINE_STRIP_ADJACENCY = GL_LINE_STRIP_ADJACENCY,
            LINES_ADJACENCY = GL_LINES_ADJACENCY,
            TRIANGLE_STRIP = GL_TRIANGLE_STRIP,
            TRIANGLE_FAN = GL_TRIANGLE_FAN,
            TRIANGLES = GL_TRIANGLES,
            TRIANGLE_STRIP_ADJACENCY = GL_TRIANGLE_STRIP_ADJACENCY,
            TRIANGLES_ADJACENCY = GL_TRIANGLES_ADJACENCY,
            PATCHES = GL_PATCHES,
        };

        static constexpr auto DISPLAY_MODES = std::to_array(
            {VAO::DisplayMode::POINTS,
             VAO::DisplayMode::LINE_STRIP,
             VAO::DisplayMode::LINE_LOOP,
             VAO::DisplayMode::LINES,
             VAO::DisplayMode::LINE_STRIP_ADJACENCY,
             VAO::DisplayMode::LINES_ADJACENCY,
             VAO::DisplayMode::TRIANGLE_STRIP,
             VAO::DisplayMode::TRIANGLE_FAN,
             VAO::DisplayMode::TRIANGLES,
             VAO::DisplayMode::TRIANGLE_STRIP_ADJACENCY,
             VAO::DisplayMode::TRIANGLES_ADJACENCY,
             VAO::DisplayMode::PATCHES});

        unsigned int object;
        DisplayMode mode;
        GLsizei count;
        ShaderProgram *shader_program;

        static constexpr DisplayMode DEFAULT_MODE{DisplayMode::TRIANGLES};

        static auto emplace(entt::registry &world, const entt::entity &entity) -> VAO &
        {
            spdlog::trace("engine::core::VAO: emplace to {}", entity);
            VAO obj{0u, DEFAULT_MODE, 0, world.ctx<State *>()->shaders[0].get()};
            CALL_OPEN_GL(::glGenVertexArrays(1, &obj.object));
            return world.emplace<VAO>(entity, obj);
        }

        static auto on_destroy(entt::registry &world, const entt::entity &entity) -> void
        {
            spdlog::trace("engine::core::VAO: destroy of {}", entity);
            const auto &vao = world.get<VAO>(entity);
            CALL_OPEN_GL(::glDeleteVertexArrays(1, &vao.object));
        }

        enum class Attribute { POSITION, COLOR, TEXTURE_2D, NORMALS };
    };

    template<VAO::Attribute A>
    struct VBO {
        static std::string name;

        unsigned int object;
        std::vector<float> vertices;
        std::size_t stride_size;

        static auto emplace(
            entt::registry &world,
            const entt::entity &entity,
            const std::vector<float> &in_vertices,
            std::size_t in_stride_size) -> VBO<A> &
        {
            spdlog::trace("engine::core::VBO<{}>: emplace to {}", magic_enum::enum_name(A).data(), entity);

            const VAO *vao{nullptr};
            if (vao = world.try_get<VAO>(entity); !vao) { vao = &VAO::emplace(world, entity); }
            if constexpr (A == VAO::Attribute::TEXTURE_2D) {
                const auto &shaders = world.ctx<State *>()->shaders;
                const auto found = std::find_if(begin(shaders), end(shaders), [](const auto &shader) {
                    return shader->getName() == "texture_2D";
                });
                world.patch<VAO>(
                    entity, [shader = found == std::end(shaders) ? shaders[0].get() : (*found).get()](auto &obj) {
                        obj.shader_program = shader;
                    });
            }

            CALL_OPEN_GL(::glBindVertexArray(vao->object));

            VBO<A> obj{0u, std::move(in_vertices), in_stride_size};
            CALL_OPEN_GL(::glGenBuffers(1, &obj.object));

            CALL_OPEN_GL(::glBindBuffer(GL_ARRAY_BUFFER, obj.object));
            CALL_OPEN_GL(::glBufferData(
                GL_ARRAY_BUFFER,
                static_cast<GLsizeiptr>(obj.vertices.size() * sizeof(float)),
                obj.vertices.data(),
                GL_STATIC_DRAW));
            CALL_OPEN_GL(::glVertexAttribPointer(
                static_cast<GLuint>(A),
                static_cast<GLint>(obj.stride_size),
                GL_FLOAT,
                GL_FALSE,
                static_cast<GLsizei>(obj.stride_size * static_cast<int>(sizeof(float))),
                0));
            CALL_OPEN_GL(::glEnableVertexAttribArray(static_cast<GLuint>(A)));

            if (const auto ebo = world.try_get<EBO>(entity); ebo == nullptr) {
                world.patch<VAO>(entity, [&obj](VAO &vao_obj) {
                    vao_obj.count = static_cast<GLsizei>(obj.vertices.size());
                });
            }

            world.remove_if_exists<VBO<A>>(entity);
            return world.emplace<VBO<A>>(entity, obj);
        }

        template<std::size_t S>
        static auto emplace(
            entt::registry &world,
            const entt::entity &entity,
            const std::array<float, S> &in_vertices,
            std::size_t in_stride_size) -> VBO<A> &
        {
            return emplace(
                world, entity, std::vector<float>(in_vertices.begin(), in_vertices.end()), in_stride_size);
        }

        static auto on_destroy(entt::registry &world, const entt::entity &entity) -> void
        {
            spdlog::trace("engine::core::VBO<{}>: destroy of {}", magic_enum::enum_name(A).data(), entity);
            const auto &vbo = world.get<VBO<A>>(entity);
            CALL_OPEN_GL(::glDeleteBuffers(1, &vbo.object));
        }
    };

    struct EBO {
        static constexpr std::string_view name{"EBO"};

        unsigned int object;
        std::vector<std::uint32_t> indices;

        template<std::size_t S>
        static auto
            emplace(entt::registry &world, const entt::entity &entity, const std::array<std::uint32_t, S> &indices)
                -> EBO &
        {
            return emplace(world, entity, std::vector<std::uint32_t>(indices.begin(), indices.end()));
        }

        static auto
            emplace(entt::registry &world, const entt::entity &entity, const std::vector<std::uint32_t> &indices)
                -> EBO &
        {
            spdlog::trace("engine::core::EBO: emplace of {}", entity);

            const VAO *vao{nullptr};
            if (vao = world.try_get<VAO>(entity); !vao) { vao = &VAO::emplace(world, entity); }
            CALL_OPEN_GL(::glBindVertexArray(vao->object));

            EBO obj{0u, std::move(indices)};
            CALL_OPEN_GL(::glGenBuffers(1, &obj.object));

            CALL_OPEN_GL(::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj.object));
            CALL_OPEN_GL(::glBufferData(
                GL_ELEMENT_ARRAY_BUFFER,
                static_cast<GLsizei>(obj.indices.size() * sizeof(uint32_t)),
                obj.indices.data(),
                GL_STATIC_DRAW));

            world.patch<VAO>(
                entity, [&obj](VAO &vao_obj) { vao_obj.count = static_cast<GLsizei>(obj.indices.size()); });

            return world.emplace_or_replace<EBO>(entity, obj);
        }

        static auto on_destroy(entt::registry &world, const entt::entity &entity) -> void
        {
            spdlog::trace("engine::core::EBO: destroy of {}", entity);
            const auto &ebo = world.get<EBO>(entity);
            CALL_OPEN_GL(::glDeleteBuffers(1, &ebo.object));
        }
    };
};

template<Render::VAO::Attribute A>
std::string Render::VBO<A>::name = std::string("VBO::") + magic_enum::enum_name(A).data();


// the biggest cube containing a mesh objects
struct AABB {
    static constexpr std::string_view name{"AABB"};

    glm::dvec3 min;
    glm::dvec3 max;

    entt::entity guizmo{entt::null};

    static auto emplace(entt::registry &world, entt::entity e, const std::vector<float> &vertices) -> AABB &
    {
        glm::dvec3 min = {
            std::numeric_limits<double>::max(),
            std::numeric_limits<double>::max(),
            std::numeric_limits<double>::max()};
        glm::dvec3 max = {
            std::numeric_limits<double>::lowest(),
            std::numeric_limits<double>::lowest(),
            std::numeric_limits<double>::lowest()};

        constexpr auto default_pos = Position3f{};
        constexpr auto default_scale = Scale3f{};
        constexpr auto default_rot = Rotation3f{};

        // todo : avoid that

        auto pos = world.try_get<const Position3f>(e);
        if (pos == nullptr) { pos = &default_pos; }

        auto scale = world.try_get<const Scale3f>(e);
        if (scale == nullptr) { scale = &default_scale; }

        auto rot = world.try_get<const Rotation3f>(e);
        if (rot == nullptr) { rot = &default_rot; }

        auto model = glm::dmat4(1.0);
        model = glm::translate(model, pos->component);
        model = glm::rotate(model, glm::radians(rot->component.x), glm::dvec3(1.0, 0.0, 0.0));
        model = glm::rotate(model, glm::radians(rot->component.y), glm::dvec3(0.0, 1.0, 0.0));
        model = glm::rotate(model, glm::radians(rot->component.z), glm::dvec3(0.0, 0.0, 1.0));
        model = glm::scale(model, scale->component);

        constexpr auto size_stride = 3;
        for (auto i = 0ul; i != vertices.size(); i += size_stride) {
            const auto projected = model * glm::dvec4{vertices[i], vertices[i + 1], vertices[i + 2], 1};

            min.x = std::min(min.x, projected.x);
            min.y = std::min(min.y, projected.y);
            min.z = std::min(min.z, projected.z);

            max.x = std::max(max.x, projected.x);
            max.y = std::max(max.y, projected.y);
            max.z = std::max(max.z, projected.z);
        }

        AABB *aabb = world.try_get<AABB>(e);
        if (aabb == nullptr) {
            const auto guizmo = world.create();

            // clang-format off
            constexpr auto outlined_cube_colors = std::to_array({
                0.0f, 0.0f, 0.0f, 1.0f,
                0.0f, 0.0f, 0.0f, 1.0f,
                0.0f, 0.0f, 0.0f, 1.0f,
                0.0f, 0.0f, 0.0f, 1.0f,
                0.0f, 0.0f, 0.0f, 1.0f,
                0.0f, 0.0f, 0.0f, 1.0f,
                0.0f, 0.0f, 0.0f, 1.0f,
                0.0f, 0.0f, 0.0f, 1.0f
            });
            constexpr auto outlined_cube_indices = std::to_array<std::uint32_t>({
                0, 1, 1, 2, 2, 3, 3, 0, // Front
                4, 5, 5, 6, 6, 7, 7, 4, // Back
                0, 4, 1, 5, 2, 6, 3, 7
            });
            // clang-format on

            Render::VBO<Render::VAO::Attribute::COLOR>::emplace(world, guizmo, outlined_cube_colors, 4);

            Render::EBO::emplace(world, guizmo, outlined_cube_indices);
            world.get<Render::VAO>(guizmo).mode = Render::VAO::DisplayMode::LINES;
            const auto parent_name = world.try_get<Name>(e);
            const auto name = parent_name != nullptr ? std::string{parent_name->name} : fmt::format("{}", e);
            world.emplace<Name>(guizmo, fmt::format("<aabb::guizmo#{}>", name));
            world.emplace<Parent>(guizmo, e);

            aabb = &world.emplace<AABB>(e, min, max, guizmo);
            world.get_or_emplace<Children>(e).component.push_back(guizmo);

        } else {
            world.patch<AABB>(e, [&min, &max](auto &obj) {
                obj.min = min;
                obj.max = max;
            });
        }

        const auto outlined_cube_positions = std::to_array<float>(
            {static_cast<float>(min.x), static_cast<float>(min.y), static_cast<float>(max.z),
             static_cast<float>(max.x), static_cast<float>(min.y), static_cast<float>(max.z),
             static_cast<float>(max.x), static_cast<float>(max.y), static_cast<float>(max.z),
             static_cast<float>(min.x), static_cast<float>(max.y), static_cast<float>(max.z),
             static_cast<float>(min.x), static_cast<float>(min.y), static_cast<float>(min.z),
             static_cast<float>(max.x), static_cast<float>(min.y), static_cast<float>(min.z),
             static_cast<float>(max.x), static_cast<float>(max.y), static_cast<float>(min.z),
             static_cast<float>(min.x), static_cast<float>(max.y), static_cast<float>(min.z)});

        Render::VBO<Render::VAO::Attribute::POSITION>::emplace(world, aabb->guizmo, outlined_cube_positions, 3);

        return *aabb;
    }
};

// todo : use units package
template<std::size_t D, typename T>
struct Velocity {
    static constexpr std::string_view name{"Velocity"};

    // distance per seconds
    glm::vec<static_cast<int>(D), T> component{};
};

using Velocity3f = Velocity<3, double>;

template<std::size_t D, typename T>
struct Gravitable {
    static constexpr std::string_view name{"Gravitable"};

    glm::vec<static_cast<int>(D), T> component{};
};

using Gravitable3f = Gravitable<3, double>;

struct Mesh {
    static constexpr std::string_view name{"Mesh"};

    std::string filepath;
    std::string model_name;

    bool loaded_successfully;

    static auto emplace(entt::registry &world, const entt::entity &entity, const std::string &filepath) -> Mesh &
    {
        const auto loader = world.ctx<ResourceLoader *>();
        const auto model = loader->load<Model>(filepath);

        if (!model) {
            // error
            return world.emplace<Mesh>(entity, filepath, std::filesystem::path(filepath).filename(), false);
        }

        const Render::VAO *vao{nullptr};
        if (vao = world.try_get<Render::VAO>(entity); !vao) { vao = &Render::VAO::emplace(world, entity); }

        Render::VBO<Render::VAO::Attribute::POSITION>::emplace(world, entity, model->vertices, 3);
        Render::VBO<Render::VAO::Attribute::TEXTURE_2D>::emplace(world, entity, model->texcoords, 2);
        Render::VBO<Render::VAO::Attribute::NORMALS>::emplace(world, entity, model->normals, 3);
        Render::EBO::emplace(world, entity, model->indices);

        return world.emplace_or_replace<Mesh>(entity, filepath, std::filesystem::path(filepath).filename(), true);
    }
};

struct FillColor {
    static constexpr std::string_view name{"Fill Color"};

    // normalized value 0..1
    glm::vec4 component{1.0f, 1.0f, 1.0f, 1.0f};
};

struct Texture2D {
    static constexpr std::string_view name{"Texture2D"};

    std::string filepath;
    std::uint32_t textureID;
    std::shared_ptr<Texture> ref_resource;

    static const Texture2D empty;

    static auto emplace(entt::registry &world, entt::entity e, const std::string &filepath) -> Texture2D
    {
        auto &loader = *world.ctx<kawe::ResourceLoader *>();
        Texture2D texture{filepath, 0u, loader.load<Texture>(filepath)};

        if (!texture.ref_resource) {
            texture.ref_resource = std::make_shared<Texture>();
            return world.emplace_or_replace<Texture2D>(e, texture);
        }

        CALL_OPEN_GL(::glGenTextures(1, &texture.textureID));
        CALL_OPEN_GL(::glBindTexture(GL_TEXTURE_2D, texture.textureID));

        CALL_OPEN_GL(::glTexStorage2D(
            GL_TEXTURE_2D, 1, GL_RGBA8, texture.ref_resource->width, texture.ref_resource->height));
        CALL_OPEN_GL(::glTexSubImage2D(
            GL_TEXTURE_2D,
            0,
            0,
            0,
            texture.ref_resource->width,
            texture.ref_resource->height,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            texture.ref_resource->data));
        CALL_OPEN_GL(::glGenerateMipmap(GL_TEXTURE_2D));


        // CALL_OPEN_GL(::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT)); // GL_MIRRORED_REPEAT
        // CALL_OPEN_GL(::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT)); // GL_MIRRORED_REPEAT

        // CALL_OPEN_GL(::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
        // CALL_OPEN_GL(::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));


        CALL_OPEN_GL(::glBindTexture(GL_TEXTURE_2D, 0));

        return world.emplace_or_replace<Texture2D>(e, texture);
    }
};

struct Collider {
    static constexpr std::string_view name{"Collider"};

    // todo : should have a collision strategy

    // todo : remove me
    enum class CollisionStep {
        NONE, // no collision at all
        AABB, // aabb colliding
        // SAT, // check if there is a real collision //
        // https://en.wikipedia.org/wiki/Hyperplane_separation_theorem
        // STATIC_RESOLVE or PHYSIC_RESOLVE
    };

    CollisionStep step = CollisionStep::NONE;

    // todo ? : keep a reference of the entity colliding with ?
    // std::vector<entt::entity>
};

struct Clock {
    static constexpr std::string_view name{"Clock"};

    std::function<void(void)> callback;
    std::chrono::milliseconds refresh_rate;
    std::chrono::milliseconds current;

    static auto
    emplace(
        entt::registry &world,
        const entt::entity &entity,
        const std::chrono::milliseconds &refresh_rate,
        const std::function<void(void)> &callback
    )
        -> Clock &
    {
        Clock clock {
          .callback = callback,
          .refresh_rate = refresh_rate,
          .current = std::chrono::milliseconds::zero()
        };

        world.emplace<Clock>(entity, clock);

        world.ctx<entt::dispatcher *>()->sink<kawe::TimeElapsed>()
          .connect<&Clock::on_update>(world.get<Clock>(entity));

        return world.get<Clock>(entity);
    }

    auto on_update(const kawe::TimeElapsed &e) -> void
    {
        current += std::chrono::duration_cast<std::chrono::milliseconds>(e.elapsed);

        if (current >= refresh_rate) {
            callback();
            current = std::chrono::milliseconds::zero();
        }
    }
};

struct Pickable {
    static constexpr std::string_view name{"Pickable"};

    bool is_picked{false};
};

struct CameraData {
    static constexpr std::string_view name{"Camera Data"};

    Rect4<float> viewport{0.0f, 0.0f, 1.0f, 1.0f};

    glm::dmat4 projection{};
    glm::dmat4 view{};

    double fov{45.0};
    double near{0.1};
    double far{1000.0};

    glm::dvec3 imagePlaneHorizDir{};
    glm::dvec3 imagePlaneVertDir{};
    glm::dvec2 display{};

    glm::dvec3 target_center{0.0, 0.0, 0.0};
    glm::dvec3 up{0.0, 1.0, 0.0};

    static constexpr double DEFAULT_ROTATE_SPEED = 2.0 / 100.0;
    static constexpr double DEFAULT_ZOOM_FRACTION = 2.5 / 100.0;
    static constexpr double DEFAULT_TRANSLATE_SPEED = 0.5 / 100.0;

    // double fractionChangeX, double fractionChangeY

    static auto rotate(entt::registry &world, entt::entity e, const CameraData &cam, const glm::dvec2 &amount)
    {
        const auto setFromAxisAngle = [](const glm::dvec3 &axis, double angle) -> glm::dquat {
            const auto cosAng = std::cos(angle / 2.0);
            const auto sinAng = std::sin(angle / 2.0);
            const auto norm = std::sqrt(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);

            return {
                sinAng * axis.x / norm,
                sinAng * axis.y / norm,
                sinAng * axis.z / norm,
                cosAng};
        };

        const auto horizRot = setFromAxisAngle(cam.imagePlaneHorizDir, DEFAULT_ROTATE_SPEED * amount.y);
        const auto vertRot = setFromAxisAngle(cam.imagePlaneVertDir, -DEFAULT_ROTATE_SPEED * amount.x);
        const auto totalRot = horizRot * vertRot;

        const auto pos = world.get<Position3f>(e);

        world.patch<Position3f>(e, [&cam, viewVec = totalRot * (pos.component - cam.target_center)](auto &p) {
            p.component = cam.target_center + viewVec;
        });
        world.patch<CameraData>(e, [](auto &) {});
    }

    static auto zoom(entt::registry &world, entt::entity e, const CameraData &cam, double amount)
    {
        world.patch<Position3f>(e, [scaleFactor = std::pow(2.0, -amount * DEFAULT_ZOOM_FRACTION), &cam](auto &pos) {
            pos.component = cam.target_center + (pos.component - cam.target_center) * scaleFactor;
        });
        world.patch<CameraData>(e, [](auto &) {});
    }

    //  double changeHoriz, double changeVert,

    static auto
        translate(entt::registry &world, entt::entity e, const CameraData &cam, const glm::dvec2 &amount, bool parallelToViewPlane)
    {
        const auto &pos = world.get<Position3f>(e);
        const auto translateVec = parallelToViewPlane
                                      ? (cam.imagePlaneHorizDir * (cam.display.x * amount.x))
                                            + (cam.imagePlaneVertDir * (amount.y * cam.display.y))
                                      : (cam.target_center - pos.component) * amount.y;

        world.patch<Position3f>(
            e, [&translateVec](auto &p) { p.component += translateVec * DEFAULT_TRANSLATE_SPEED; });
        world.patch<CameraData>(
            e, [&translateVec](auto &c) { c.target_center += translateVec * DEFAULT_TRANSLATE_SPEED; });
    }
};

using Component = std::variant<
    std::monostate,
    // system
    Name,
    Parent,
    Children,
    CameraData,
    // rendering
    Mesh,
    Texture2D,
    FillColor,
    Render::VAO,
    Render::EBO,
    Render::VBO<Render::VAO::Attribute::POSITION>,
    Render::VBO<Render::VAO::Attribute::COLOR>,
    Render::VBO<Render::VAO::Attribute::TEXTURE_2D>,
    Render::VBO<Render::VAO::Attribute::NORMALS>,
    // transform
    Position3f,
    Rotation3f,
    Scale3f,
    // physics
    Gravitable3f,
    Velocity3f,
    Collider,
    AABB,
    Clock,
    Pickable>;

} // namespace kawe
