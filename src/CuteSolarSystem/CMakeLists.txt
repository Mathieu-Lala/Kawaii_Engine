find_package(Vulkan REQUIRED FATAL_ERROR)

add_library(glfw_imgui_vulk STATIC ${CONAN_RES_DIRS_IMGUI}/bindings/imgui_impl_glfw.cpp
                                   ${CONAN_RES_DIRS_IMGUI}/bindings/imgui_impl_vulkan.cpp)
target_link_libraries(glfw_imgui_vulk PUBLIC project_options CONAN_PKG::glfw CONAN_PKG::imgui Vulkan::Vulkan)
target_include_directories(glfw_imgui_vulk PUBLIC ${CONAN_RES_DIRS_IMGUI}/bindings)
target_compile_definitions(glfw_imgui_vulk PUBLIC GLFW_INCLUDE_NONE GLFW_INCLUDE_VULKAN)

if(CMAKE_BUILD_TYPE EQUAL "Debug")
  target_compile_definitions(glfw_imgui_vulk PUBLIC IMGUI_VULKAN_DEBUG_REPORT)
endif(CMAKE_BUILD_TYPE EQUAL "Debug")

add_executable(app src/main.cpp src/RenderBackend.cpp src/RenderWindow.cpp)

target_link_libraries(app PRIVATE project_options project_warnings glfw_imgui_vulk CONAN_PKG::spdlog)
target_include_directories(app PRIVATE include)

install(TARGETS app DESTINATION bin)
