add_library(
  kawaii_engine STATIC
  src/graphics/Window.cpp src/graphics/Shader.cpp src/EventProvider.cpp src/widgets/ComponentInspector.cpp
  src/resources/ResourceLoader.cpp src/Component.cpp src/deps/deps_impl.cpp)

target_link_libraries(
  kawaii_engine
  PUBLIC project_options
         project_warnings
         glfw_imgui_opengl3
         CONAN_PKG::spdlog
         CONAN_PKG::glm
         CONAN_PKG::stb
         CONAN_PKG::nlohmann_json
         CONAN_PKG::tinyobjloader
         CONAN_PKG::magic_enum
         CONAN_PKG::entt
         CONAN_PKG::stb
         CONAN_PKG::mp-units
         ImGuiFileDialog)
target_include_directories(kawaii_engine PUBLIC include)
target_compile_definitions(kawaii_engine PUBLIC MAGIC_ENUM_RANGE_MIN=0 MAGIC_ENUM_RANGE_MAX=512 GLM_CONFIG_XYZW_ONLY)

install(TARGETS kawaii_engine DESTINATION lib)

install(DIRECTORY asset DESTINATION .)

# add_custom_target(vert.spv ALL COMMAND glslc ${CMAKE_CURRENT_SOURCE_DIR}/asset/shader/shader.vert -o vert.spv)
# install(FILES ${CMAKE_CURRENT_BINARY_DIR}/vert.spv DESTINATION asset/shader)
#
# add_custom_target(frag.spv ALL COMMAND glslc ${CMAKE_CURRENT_SOURCE_DIR}/asset/shader/shader.frag -o frag.spv)
# install(FILES ${CMAKE_CURRENT_BINARY_DIR}/frag.spv DESTINATION asset/shader)
