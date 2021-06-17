add_library(
  imgui STATIC
  ../../dependencies/imgui/imgui.cpp ../../dependencies/imgui/imgui_widgets.cpp ../../dependencies/imgui/imgui_draw.cpp
  ../../dependencies/imgui/imgui_demo.cpp ../../dependencies/imgui/imgui_tables.cpp)
# target_link_libraries(imgui PUBLIC project_options)
target_include_directories(imgui PUBLIC ../../dependencies/imgui)
