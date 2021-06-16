add_library(glfw_imgui_opengl3 STATIC ../../dependencies/imgui/backends/imgui_impl_glfw.cpp
                                      ../../dependencies/imgui/backends/imgui_impl_opengl3.cpp)
target_link_libraries(glfw_imgui_opengl3 PUBLIC project_options CONAN_PKG::glfw imgui CONAN_PKG::glew)
target_include_directories(glfw_imgui_opengl3 PUBLIC ../../dependencies/imgui/backends)
target_compile_definitions(glfw_imgui_opengl3 PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLEW)
