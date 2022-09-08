# ---------------------------------------------------------------------------- #
# GLOBAL
# ---------------------------------------------------------------------------- #
cmake_minimum_required(VERSION 3.11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug, Release, or MinSizeRel." FORCE)
    message(STATUS "CMAKE_BUILD_TYPE not set, defaulting to Release.")
endif()

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

# ---------------------------------------------------------------------------- #
# PROJECT
# ---------------------------------------------------------------------------- #
if(NOT BUILD_NUMBER)
    set(BUILD_NUMBER 0)
endif()

if(DEV_INTERFACE)
    set(ENABLE_IMGUI 1)
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

project (game_project_engine
        VERSION 0.1.${BUILD_NUMBER}
        DESCRIPTION "Game Project"
        LANGUAGES CXX C OBJCXX OBJC  
        )

# ---------------------------------------------------------------------------- #
# LIBRARIES
# ---------------------------------------------------------------------------- #
include(SetupLib)

if(FORCE_OPENGL)
    SetupLib_opengl()
endif()
SetupLib_bgfx()
SetupLib_glm()
SetupLib_tinygltf()
SetupLib_glfw()
SetupLib_rapidjson()
if(ENABLE_IMGUI)
    SetupLib_imgui()
endif()
if(DEV_INTERFACE)
    SetupLib_nativefiledialog()
endif()

set(SetupLib_include_dirs ${SetupLib_include_dirs} PARENT_SCOPE)

# ---------------------------------------------------------------------------- #
# CONFIGURE MAIN PROJECT
# ---------------------------------------------------------------------------- #

# COMPILE SOURCES
add_library(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/main_desktop.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MrManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/animation/Animator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common/platform.mm
    ${CMAKE_CURRENT_SOURCE_DIR}/common/string_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common/modp_b64.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/dev/print.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/memory/MemSys.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/memory/MemSys_Entity.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/render/CameraControl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/render/GLTFLoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/render/Mesh.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/render/RenderSystem.cpp
    ${SetupLib_sources}
)
if(DEV_INTERFACE)
    target_sources(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/dev/Editor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/dev/OriginWidget.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/dev/print.cpp
    )
endif()

# SHADERS
include(BGFXCompileShaders)
set(BGFXCompileShaders_OuputProfiles "metal")
if(FORCE_OPENGL)
    set(BGFXCompileShaders_OuputProfiles "150")
endif()
BGFXCompileShaders(
    embed
    shader_dirs     "${CMAKE_CURRENT_SOURCE_DIR}/shader/shaders/gltf"
                    "${CMAKE_CURRENT_SOURCE_DIR}/shader/shaders/unlit"
    profile_list    "${BGFXCompileShaders_OuputProfiles}"
    target          BGFXShader_engine_target
)

# CONFIGURE MAIN BINARY
set_property(TARGET ${PROJECT_NAME} APPEND_STRING PROPERTY COMPILE_FLAGS ${SetupLib_flags})
target_link_libraries(${PROJECT_NAME} ${SetupLib_libs})
target_compile_definitions(${PROJECT_NAME} PUBLIC ENABLE_IMGUI=${ENABLE_IMGUI})
target_compile_definitions(${PROJECT_NAME} PUBLIC DEV_INTERFACE=${DEV_INTERFACE})
target_compile_definitions(${PROJECT_NAME} PUBLIC FORCE_OPENGL=${FORCE_OPENGL})
add_dependencies(${PROJECT_NAME} BGFXShader_engine_target)
