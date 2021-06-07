#pragma once

#include "Engine.hpp"

constexpr auto line_indices = std::to_array({
    0u, 1u
});

auto create_line(
    entt::registry &world,
    const glm::vec3 &start,
    const glm::vec3 &end,
    [[ maybe_unused ]]const glm::vec4 &color = glm::vec4(1.f)
) -> entt::entity {

    auto line = world.create();

    auto line_positions = std::to_array({
        start.x, start.y, start.z,
        end.x, end.y, end.z,
    });

    auto line_color = std::to_array({
        color.x, color.y, color.z, color.w,
        color.x, color.y, color.z, color.w
    });

    kawe::Render::VBO<kawe::Render::VAO::Attribute::POSITION>::emplace(world, line, line_positions, 3);
    kawe::Render::VBO<kawe::Render::VAO::Attribute::COLOR>::emplace(world, line, line_color, 4);
    kawe::Render::EBO::emplace(world, line, line_indices);
    world.get<kawe::Render::VAO>(line).mode = kawe::Render::VAO::DisplayMode::LINES;
    world.emplace<kawe::FillColor>(line, color);
    world.emplace<kawe::Position3f>(line, glm::vec3(0.f));

    return line;
}