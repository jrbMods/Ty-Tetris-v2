cmake_minimum_required(VERSION 3.14)
project(Project)

if(MSVC)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_CURRENT_BINARY_DIR})
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_CURRENT_BINARY_DIR})
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_CURRENT_BINARY_DIR})
    endforeach(OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES)
endif()

set(sourceFiles
    vendor/stb_image.cpp
    main.cpp
    structs.cpp
    ui.cpp
)

add_executable(Project ${sourceFiles})

target_include_directories(Project PRIVATE ${CMAKE_SOURCE_DIR})

target_link_libraries(Project PRIVATE glad glfw imgui glm)

# ---- Tetris target ----
# Source lives at src/tetris/main.cpp
add_executable(Tetris
    tetris/main.cpp
)

# Your root already sets C++20; this just asserts a minimum for Tetris.
target_compile_features(Tetris PRIVATE cxx_std_17)

# OpenGL (adds OpenGL::GL on non-Windows; we also add opengl32 on Windows below)
find_package(OpenGL REQUIRED)

# Include dirs — keep your repo root and dependencies include (for glad/glad.h)
target_include_directories(Tetris PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/dependencies/include
)

# Link against the SAME libs your Project target uses so we reuse that glfw/glad
target_link_libraries(Tetris PRIVATE
    glad
    glfw
    imgui
    glm
    OpenGL::GL
)

# Windows: make sure opengl32 is pulled in
if (WIN32)
    target_link_libraries(Tetris PRIVATE opengl32)
endif()

# MSVC quality-of-life flags (UTF-8 & CRT)
if (MSVC)
    target_compile_options(Tetris PRIVATE /utf-8)
    target_compile_definitions(Tetris PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# Ensure resources copy runs before Tetris (target is defined in root CMakeLists)
add_dependencies(Tetris copy_resources)

target_include_directories(Tetris PRIVATE ${CMAKE_SOURCE_DIR}/lib/stb)


