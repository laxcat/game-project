# ---------------------------------------------------------------------------- #
# SETUP
# ---------------------------------------------------------------------------- #
# GLOBAL SETTINGS
cmake_minimum_required(VERSION 3.11)
set(CMAKE_CXX_STANDARD 17)

# INCLUDES
include(cmake/SetupLib.cmake)
include(cmake/BGFXShader.cmake)
include(cmake/ComponentList.cmake)

# PROJECT
project(game_project VERSION 0.1.0 LANGUAGES CXX)


# ---------------------------------------------------------------------------- #
# LIBRARIES
# ---------------------------------------------------------------------------- #
# SetupLib_opengl()
SetupLib_bgfx()
SetupLib_glfw()
SetupLib_glm()
SetupLib_entt()
SetupLib_cereal()
SetupLib_imgui()
SetupLib_tinygltf()


# ---------------------------------------------------------------------------- #
# MAIN PROJECT
# ---------------------------------------------------------------------------- #
# COMPILE SOURCES
add_executable(GameProject
    src/main.cpp
    src/animation/Animator.cpp
    src/develop/print.cpp
    src/develop/Editor.cpp
    src/render/GLTFLoader.cpp
    src/render/RenderSystem.cpp
    src/render/Tester.cpp
    src/render/TestQuadSystem.cpp
    ${SetupLib_sources}
)

# CODE GENERATION
ComponentList_generate(
    src/components/@list.h      # in
    src/components/all.h.in     # template
    src/components/all.h        # out
)

# SHADERS
BGFXShader_compile(
    ${CMAKE_SOURCE_DIR}/src/shader/shaders  # root of shaders to compile. expects shaders to be subdirs
    ${CMAKE_BINARY_DIR}/shaders             # shader output dir. will create folders for each api
    ${bgfx_content_BINARY_DIR}/shaderc      # path to shaderc
    ${bgfx_content_SOURCE_DIR}/bgfx/src     # path to bgfx shader includes
    "metal"                                 # list of ouput profiles. see shaderc help
)

# CONFIGURE MAIN BINARY
set_property(TARGET GameProject APPEND_STRING PROPERTY COMPILE_FLAGS ${SetupLib_flags})
target_link_libraries(GameProject ${SetupLib_libs})
add_dependencies(GameProject BGFXShader_target)
add_dependencies(GameProject ComponentList_target)
