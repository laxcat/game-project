# ---------------------------------------------------------------------------- #
# SETUP
# ---------------------------------------------------------------------------- #
# GLOBAL SETTINGS
cmake_minimum_required(VERSION 3.11)
set(CMAKE_CXX_STANDARD 17)

# INCLUDES
include(cmake/SetupLib.cmake)
include(cmake/BGFXShader.cmake)

# PROJECT
project(game_project VERSION 0.1.0 LANGUAGES CXX)


# ---------------------------------------------------------------------------- #
# LIBRARIES
# ---------------------------------------------------------------------------- #
# SetupLib_opengl()
SetupLib_bgfx()
SetupLib_glfw()
# SetupLib_glm()
# SetupLib_entt()
SetupLib_imgui()


# ---------------------------------------------------------------------------- #
# MAIN PROJECT
# ---------------------------------------------------------------------------- #
# COMPILE SOURCES
add_executable(GameProject
    src/main.cpp
    ${SetupLib_sources}
)

# SHADERS
BGFXShader_compile(src/shaders)

# CONFIGURE MAIN BINARY
set_property(TARGET GameProject APPEND_STRING PROPERTY COMPILE_FLAGS ${SetupLib_flags})
target_link_libraries(GameProject ${SetupLib_libs})
add_dependencies(GameProject BGFXShader_target)
