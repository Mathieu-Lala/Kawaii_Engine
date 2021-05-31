#include "Engine.hpp"
#include "component.hpp"

namespace data {

// clang-format off
constexpr auto cube_positions = std::to_array({
    -0.5f, 0.5f,  -0.5f, // Point A 0
    -0.5f, 0.5f,  0.5f,  // Point B 1
    0.5f,  0.5f,  -0.5f, // Point C 2
    0.5f,  0.5f,  0.5f,  // Point D 3
    -0.5f, -0.5f, -0.5f, // Point E 4
    -0.5f, -0.5f, 0.5f,  // Point F 5
    0.5f,  -0.5f, -0.5f, // Point G 6
    0.5f,  -0.5f, 0.5f,  // Point H 7
});

constexpr auto cube_colors = std::to_array({
    0.0f, 0.0f, 0.0f, 1.0f, // Point A 0
    0.0f, 0.0f, 1.0f, 1.0f, // Point B 1
    0.0f, 1.0f, 0.0f, 1.0f, // Point C 2
    0.0f, 1.0f, 1.0f, 1.0f, // Point D 3
    1.0f, 0.0f, 0.0f, 1.0f, // Point E 4
    1.0f, 0.0f, 1.0f, 1.0f, // Point F 5
    1.0f, 1.0f, 0.0f, 1.0f, // Point G 6
    1.0f, 1.0f, 1.0f, 1.0f // Point H 7
});

constexpr auto cube_indices = std::to_array({
    /*Above ABC,BCD*/
    0u, 1u, 2u, 1u, 2u, 3u,
    /*Following EFG,FGH*/
    4u, 5u, 6u, 5u, 6u, 7u,
    /*Left ABF,AEF*/
    0u, 1u, 5u, 0u, 4u, 5u,
    /*Right side CDH,CGH*/
    2u, 3u, 7u, 2u, 6u, 7u,
    /*ACG,AEG*/
    0u, 2u, 6u, 0u, 4u, 6u,
    /*Behind BFH,BDH*/
    1u, 5u, 7u, 1u, 3u, 7u
});
// clang-format on

} // namespace data

int main()
{
    kawe::Engine engine{};

    engine.on_create = [](entt::registry &world) {
        for (int y = 0; y != 100; y++) {
            for (int x = 0; x != 100; x++) {
                const auto cube = world.create();
                kawe::Render::VBO<kawe::Render::VAO::Attribute::POSITION>::emplace(
                    world, cube, data::cube_positions, 3);
                kawe::Render::VBO<kawe::Render::VAO::Attribute::COLOR>::emplace(world, cube, data::cube_colors, 4);
                kawe::Render::EBO::emplace(world, cube, data::cube_indices);
                world.emplace<kawe::Position3f>(cube, glm::vec3{x, 0, y});
            }
        }
    };

    engine.on_imgui = []() {
        ImGui::Begin("hello");
        ImGui::End();
    };

    engine.start();
}
