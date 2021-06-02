#pragma once

#include <string_view>
#include <filesystem>
#include <string>
#include <variant>
#include <limits>

#include <glm/glm.hpp>
#include <magic_enum.hpp>
#include <entt/entt.hpp>

#include <GL/glew.h>

#include "resources/ResourceLoader.hpp"
#include "State.hpp"

namespace kawe {

template<std::size_t D, typename T>
struct Position {
    static constexpr std::string_view name{"Position"};
    static constexpr glm::vec<static_cast<int>(D), T> default_value{0.0f, 0.0f, 0.0f};

    glm::vec<static_cast<int>(D), T> component{default_value};
};

using Position3f = Position<3, double>;

template<std::size_t D, typename T>
struct Rotation {
    static constexpr std::string_view name{"Rotation"};
    static constexpr glm::vec<static_cast<int>(D), T> default_value{0.0f, 0.0f, 0.0f};

    glm::vec<static_cast<int>(D), T> component{default_value};
};

using Rotation3f = Rotation<3, double>;

template<std::size_t D, typename T>
struct Scale {
    static constexpr std::string_view name{"Scale"};
    static constexpr glm::vec<static_cast<int>(D), T> default_value{1.0f, 1.0f, 1.0f};

    glm::vec<static_cast<int>(D), T> component{default_value};
};

using Scale3f = Scale<3, double>;

// the biggest cube containing a mesh objects
struct AABB {
    static constexpr std::string_view name{"AABB"};

    glm::dvec3 min;
    glm::dvec3 max;

    static auto emplace(entt::registry &world, entt::entity e, const std::vector<float> &vertices) -> AABB &
    {
        glm::dvec3 min = {
            std::numeric_limits<double>::max(),
            std::numeric_limits<double>::max(),
            std::numeric_limits<double>::max()};
        glm::dvec3 max = {
            std::numeric_limits<double>::min(),
            std::numeric_limits<double>::min(),
            std::numeric_limits<double>::min()};

        const auto state = world.ctx<State *>();

        constexpr auto default_pos = Position3f{Position3f::default_value};
        constexpr auto default_scale = Scale3f{Scale3f::default_value};
        constexpr auto default_rot = Rotation3f{Rotation3f::default_value};

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

        const auto mvp = state->projection * state->view * model;
        spdlog::info(
            "{} {} {} {}\n"
            "{} {} {} {}\n"
            "{} {} {} {}\n"
            "{} {} {} {}\n",
            mvp[0][0],
            mvp[0][1],
            mvp[0][2],
            mvp[0][3],
            mvp[1][0],
            mvp[1][1],
            mvp[1][2],
            mvp[1][3],
            mvp[2][0],
            mvp[2][1],
            mvp[2][2],
            mvp[2][3],
            mvp[3][0],
            mvp[3][1],
            mvp[3][2],
            mvp[3][3]);


        constexpr auto size_stride = 3;
        for (auto i = 0ul; i != vertices.size(); i += size_stride) {
            spdlog::info("vertices {} {} {}", vertices[i], vertices[i + 1], vertices[i + 2]);

            const auto projected = mvp * glm::dvec4{vertices[i], vertices[i + 1], vertices[i + 2], 1};
            spdlog::info("projected : {} {} {}", projected.x, projected.y, projected.z);

            min.x = std::min(min.x, projected.x);
            min.y = std::min(min.y, projected.y);
            min.z = std::min(min.z, projected.z);

            max.x = std::max(max.x, projected.x);
            max.y = std::max(max.y, projected.y);
            max.z = std::max(max.z, projected.z);
        }

        return world.emplace_or_replace<AABB>(e, min, max);
    }
};

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

        static constexpr DisplayMode DEFAULT_MODE{DisplayMode::TRIANGLES};

        static auto emplace(entt::registry &world, const entt::entity &entity) -> VAO &
        {
            spdlog::trace("engine::core::VAO: emplace to {}", entity);
            VAO obj{0u, DEFAULT_MODE, 0};
            CALL_OPEN_GL(::glGenVertexArrays(1, &obj.object));
            return world.emplace<VAO>(entity, obj);
        }

        static auto on_destroy(entt::registry &world, const entt::entity &entity) -> void
        {
            spdlog::trace("engine::core::VAO: destroy of {}", entity);
            const auto &vao = world.get<VAO>(entity);
            CALL_OPEN_GL(::glDeleteVertexArrays(1, &vao.object));
        }

        enum class Attribute { POSITION, COLOR, NORMALS };
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

            if (A == VAO::Attribute::POSITION) { AABB::emplace(world, entity, obj.vertices); }

            if (const auto vbo = world.try_get<VBO<A>>(entity); vbo != nullptr) {
                world.remove<VBO<A>>(entity);
            }
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

            EBO obj{};
            CALL_OPEN_GL(::glGenBuffers(1, &obj.object));

            CALL_OPEN_GL(::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj.object));
            CALL_OPEN_GL(::glBufferData(
                GL_ELEMENT_ARRAY_BUFFER,
                static_cast<GLsizei>(indices.size() * sizeof(uint32_t)),
                indices.data(),
                GL_STATIC_DRAW));

            world.patch<VAO>(
                entity, [&indices](VAO &vao_obj) { vao_obj.count = static_cast<GLsizei>(indices.size()); });

            return world.emplace_or_replace<EBO>(entity);
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


struct Name {
    static constexpr std::string_view name{"Name"};

    std::string component;
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
        auto loader = world.ctx<ResourceLoader *>();
        auto model = loader->load<kawe::Model>(filepath);

        Mesh mesh{filepath, std::filesystem::path(filepath).filename(), false};

        spdlog::info(mesh.filepath);
        spdlog::info(mesh.model_name);
        spdlog::info(mesh.loaded_successfully);

        if (!model) return world.emplace<Mesh>(entity, mesh);

        const Render::VAO *vao{nullptr};
        if (vao = world.try_get<Render::VAO>(entity); !vao) { vao = &Render::VAO::emplace(world, entity); }

        // TODO: support normals & texcoords.
        // TODO: check if VAO & EBO aren't already emplaced.
        kawe::Render::VBO<kawe::Render::VAO::Attribute::POSITION>::emplace(world, entity, model->vertices, 3);
        kawe::Render::EBO::emplace(world, entity, model->indices);

        mesh.loaded_successfully = true;

        return world.emplace_or_replace<Mesh>(entity, mesh);
    }
};


struct Collider {
    static constexpr std::string_view name{"Collider"};

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

using Component = std::variant<
    std::monostate,
    Name,
    Render::VAO,
    Render::EBO,
    Render::VBO<Render::VAO::Attribute::POSITION>,
    Render::VBO<Render::VAO::Attribute::COLOR>,
    Position3f,
    Rotation3f,
    Scale3f,
    Gravitable3f,
    Velocity3f,
    Mesh,
    Collider,
    AABB>;

} // namespace kawe
