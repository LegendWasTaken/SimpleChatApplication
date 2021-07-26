cmake_minimum_required(VERSION 3.16)
project(ChatApplication)

add_subdirectory(ext)

set(CMAKE_CXX_STANDARD 20)

add_executable(ChatApplication
        src/main.cpp
        src/display.cpp
        src/display.h
        src/glad/glad.c
        src/imgui/imgui.cpp
        src/imgui/imgui_draw.cpp
        src/imgui/imgui_impl_opengl3.cpp
        src/imgui/imgui_tables.cpp
        src/imgui/imgui_widgets.cpp
        src/imgui/imgui_impl_glfw.cpp
        src/client_mode.h
        src/message.h
        src/network_processor.cpp
        src/network_processor.h)

target_include_directories(ChatApplication PRIVATE src ext)
target_link_libraries(ChatApplication PRIVATE glfw sockpp-static)
target_compile_definitions(ChatApplication PRIVATE -DGLFW_INCLUDE_NONE)
