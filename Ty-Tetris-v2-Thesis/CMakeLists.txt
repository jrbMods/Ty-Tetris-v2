cmake_minimum_required(VERSION 3.14)
project(Project)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(FORCE_COLORED_OUTPUT "Always produce ANSI-colored output (GNU/Clang only)." TRUE)
if (FORCE_COLORED_OUTPUT)
  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    add_compile_options(-fdiagnostics-color=always)
  elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_compile_options(-fcolor-diagnostics)
  endif()
endif()

# Copy resources to build dir(s)
add_custom_target(copy_resources ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/resources ${CMAKE_BINARY_DIR}/resources
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/resources ${CMAKE_BINARY_DIR}/src/resources
)

# lib/ must provide glfw + glad targets (as in your existing repo)
add_subdirectory(lib)

find_package(OpenGL REQUIRED)

# ---------- Engine lib ----------
add_library(tinyengine STATIC
    src/engine/Shader.cpp
    src/engine/Renderer.cpp
    src/engine/Audio.cpp
    src/engine/Texture.cpp
    src/engine/DB.cpp
    vendor/sqlite/sqlite3.c
)

target_include_directories(tinyengine PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/lib/stb
    ${CMAKE_SOURCE_DIR}/vendor/miniaudio
    ${CMAKE_SOURCE_DIR}/vendor/sqlite
)

target_link_libraries(tinyengine PUBLIC
    glfw
    glad
    OpenGL::GL
)

if (WIN32)
  target_link_libraries(tinyengine PUBLIC opengl32)
  target_compile_options(tinyengine PRIVATE /utf-8)
  target_compile_definitions(tinyengine PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# ---------- Tetris app ----------
add_executable(Tetris
    src/app/main.cpp
    src/game/Tetris.cpp
    src/game/UI.cpp

    # Dear ImGui sources (adjust paths if needed)
    lib/imgui/imgui.cpp
    lib/imgui/imgui_draw.cpp
    lib/imgui/imgui_tables.cpp
    lib/imgui/imgui_widgets.cpp
    lib/imgui/backends/imgui_impl_glfw.cpp
    lib/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(Tetris PRIVATE
    ${CMAKE_SOURCE_DIR}/lib/imgui
    ${CMAKE_SOURCE_DIR}/lib/imgui/backends
    ${CMAKE_SOURCE_DIR}/lib/stb
    ${CMAKE_SOURCE_DIR}/vendor/miniaudio
)

target_link_libraries(Tetris PRIVATE tinyengine)

if (MSVC)
  target_compile_options(Tetris PRIVATE /utf-8)
  target_compile_definitions(Tetris PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
