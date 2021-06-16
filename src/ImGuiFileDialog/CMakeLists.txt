add_library(ImGuiFileDialog STATIC ../../dependencies/ImGuiFileDialog/ImGuiFileDialog.cpp)
target_link_libraries(ImGuiFileDialog PUBLIC project_options glfw_imgui_opengl3)
target_include_directories(ImGuiFileDialog PUBLIC ../../dependencies/ImGuiFileDialog)
