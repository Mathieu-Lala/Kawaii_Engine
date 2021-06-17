#pragma once

#include "graphics/deps.hpp"
#include "graphics/Window.hpp"

namespace kawe {

struct Recorder {
    Window &window;

    auto draw() -> void
    {
        if (!ImGui::Begin("KAWE: Recorder")) return ImGui::End();

        if (ImGui::Button("screenshot")) {
            std::filesystem::create_directories("screenshot");
            window.screenshot(fmt::format("screenshot/screenshot_{}.png", time_to_string()));
        }

        ImGui::End();
    }
};

} // namespace kawe
