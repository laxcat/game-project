# ############################################################################ #
# SETUP
# ############################################################################ #
# GLOBAL SETTINGS
cmake_minimum_required(VERSION 3.8)
set(CMAKE_CXX_STANDARD 17)

# INCLUDES
include(FetchContent)
include(SetupLib.cmake)

# PROJECT
project(game_project VERSION 0.1.0 LANGUAGES CXX)


# ############################################################################ #
# LIBRARIES
# ############################################################################ #
SetupLib_opengl()
SetupLib_bgfx()
SetupLib_glfw()
SetupLib_glm()
SetupLib_entt()
SetupLib_imgui()


# ############################################################################ #
# MAIN PROJECT
# ############################################################################ #
add_executable(GameProject
    src/main.cpp
    ${SetupLib_sources}
)

target_link_libraries(GameProject ${SetupLib_libs})
