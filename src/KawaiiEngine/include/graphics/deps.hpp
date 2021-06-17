#pragma once

#include "helpers/warnings.hpp"

DISABLE_WARNING_PUSH
DISABLE_WARNING_OLD_CAST
#include <imgui.h>
DISABLE_WARNING_POP
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace kawe {

namespace ImGuiHelper {

template<typename... Args>
inline auto Text(const std::string_view format, Args &&... args)
{
    ::ImGui::TextUnformatted(fmt::format(format, std::forward<Args>(args)...).c_str());
}

} // namespace ImGuiHelper

} // namespace kawe
