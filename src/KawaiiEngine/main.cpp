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

    engine.on_create = []([[maybe_unused]]entt::registry &world) {

        auto model = world.create();

        kawe::Mesh::emplace(world, model, "../../../dependencies/Kawaii_Engine/src/KawaiiEngine/asset/models/viking_room.obj");
        world.emplace<kawe::Position3f>(model, glm::vec3(0.0f));
    };

    engine.on_imgui = []() {
        ImGui::Begin("hello");
        ImGui::End();
    };

    engine.start();
}
