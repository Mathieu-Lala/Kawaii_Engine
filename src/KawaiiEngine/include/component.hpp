#pragma once

#include <string_view>
#include <string>
#include <variant>

#include <glm/glm.hpp>
#include <magic_enum.hpp>
#include <entt/entt.hpp>

#include <GL/glew.h>

namespace kawe {

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
        // static constexpr std::string_view name{"VBO::" + magic_enum::enum_name(A)};

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

            if (const auto vbo = world.try_get<VBO<A>>(entity); vbo != nullptr) world.remove<VBO<A>>(entity);
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
            emplace(entt::registry &world, const entt::entity &entity, const std::array<std::uint32_t, S> &vertices)
                -> EBO &
        {
            spdlog::trace("engine::core::EBO: emplace of {}", entity);

            const VAO *vao{nullptr};
            if (vao = world.try_get<VAO>(entity); !vao) { vao = &VAO::emplace(world, entity); }
            CALL_OPEN_GL(::glBindVertexArray(vao->object));

            EBO obj{};
            CALL_OPEN_GL(::glGenBuffers(1, &obj.object));

            CALL_OPEN_GL(::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj.object));
            CALL_OPEN_GL(
                ::glBufferData(GL_ELEMENT_ARRAY_BUFFER, S * sizeof(float), vertices.data(), GL_STATIC_DRAW));

            world.patch<VAO>(entity, [](VAO &vao_obj) { vao_obj.count = S; });

            return world.emplace<EBO>(entity);
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

template<std::size_t D, typename T>
struct Position {
    static constexpr std::string_view name{"Position"};

    glm::vec<static_cast<int>(D), T> component;
};

using Position3f = Position<3, float>;

template<std::size_t D, typename T>
struct Rotation {
    static constexpr std::string_view name{"Rotation"};

    glm::vec<static_cast<int>(D), T> component;
};

using Rotation3f = Rotation<3, float>;

template<std::size_t D, typename T>
struct Scale {
    static constexpr std::string_view name{"Scale"};

    glm::vec<static_cast<int>(D), T> component{1.0, 1.0, 1.0};
};

using Scale3f = Scale<3, float>;

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

using Velocity3f = Velocity<3, float>;

template<std::size_t D, typename T>
struct Gravitable {
    static constexpr std::string_view name{"Gravitable"};

    glm::vec<static_cast<int>(D), T> component{};
};

using Gravitable3f = Gravitable<3, float>;

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
    Velocity3f>;

} // namespace kawe
