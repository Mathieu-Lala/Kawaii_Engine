#include "Engine.hpp"

int main()
{
    kawe::Engine engine{};

    engine.on_create = [](entt::registry &) {};

    engine.on_imgui = []() {
        ImGui::Begin("hello");
        ImGui::End();
    };

    engine.start();
}
