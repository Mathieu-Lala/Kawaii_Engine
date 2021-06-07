#pragma once

#include "Engine.hpp"

#include <numbers>

auto create_sphere(
    entt::registry &world,
    float radius,
    unsigned int rings,
    unsigned int sectors
) -> entt::entity {
    std::vector<GLfloat> sphere_vertices;
    std::vector<GLfloat> sphere_normals;
    std::vector<GLfloat> sphere_texcoords;
    std::vector<std::uint32_t> sphere_indices;

    const auto R = 1.0f / static_cast<float>(rings - 1);
    const auto S = 1.0f / static_cast<float>(sectors - 1);

    sphere_vertices.resize(rings * sectors * 3);
    sphere_normals.resize(rings * sectors * 3);
    sphere_texcoords.resize(rings * sectors * 2);
    auto v = sphere_vertices.begin();
    auto n = sphere_normals.begin();
    auto t = sphere_texcoords.begin();
    for (auto r = 0u; r < rings; r++)
        for (auto s = 0u; s < sectors; s++) {
            const auto y = std::sin(
                -std::numbers::pi_v<float> / 2.0f + std::numbers::pi_v<float> * static_cast<float>(r) * R);
            const auto x = std::cos(2.0f * std::numbers::pi_v<float> * static_cast<float>(s) * S)
                           * std::sin(std::numbers::pi_v<float> * static_cast<float>(r) * R);
            const auto z = std::sin(2.0f * std::numbers::pi_v<float> * static_cast<float>(s) * S)
                           * std::sin(std::numbers::pi_v<float> * static_cast<float>(r) * R);

            *t++ = static_cast<float>(s) * S;
            *t++ = static_cast<float>(r) * R;

            *v++ = x * radius;
            *v++ = y * radius;
            *v++ = z * radius;

            *n++ = x;
            *n++ = y;
            *n++ = z;
        }

    sphere_indices.resize(rings * sectors * 4);
    auto i = sphere_indices.begin();
    for (auto r = 0u; r < rings; r++)
        for (auto s = 0u; s < sectors; s++) {
            *i++ = r * sectors + (s);
            *i++ = r * sectors + (s + 1);
            *i++ = (r + 1) * sectors + (s + 1);
            *i++ = (r + 1) * sectors + (s);
        }

    const auto sphere = world.create();

    kawe::Render::VBO<kawe::Render::VAO::Attribute::POSITION>::emplace(world, sphere, sphere_vertices, 3);
    kawe::Render::EBO::emplace(world, sphere, sphere_indices);
    world.get<kawe::Render::VAO>(sphere).mode = kawe::Render::VAO::DisplayMode::TRIANGLE_STRIP_ADJACENCY;
    world.emplace<kawe::Position3f>(sphere, glm::vec3{0.0f, 2.0f, 3.0f});

    return sphere;
}