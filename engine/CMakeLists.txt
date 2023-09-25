# ---------------------------------------------------------------------------- #
# GLOBAL
# ---------------------------------------------------------------------------- #
cmake_minimum_required(VERSION 3.24)
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

set(CMAKE_DEBUG_POSTFIX d)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

project (game_project_engine
        VERSION 0.1.${BUILD_NUMBER}
        DESCRIPTION "Game Project"
        LANGUAGES CXX C OBJCXX OBJC  
        )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")

# ---------------------------------------------------------------------------- #
# LIBRARIES
# ---------------------------------------------------------------------------- #
include(SetupLib)

# add_custom_target(BGFXShader_engine_target)

if(FORCE_OPENGL)
    SetupLib_opengl()
endif()
SetupLib_bgfx()
SetupLib_glm()
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
    ${CMAKE_CURRENT_SOURCE_DIR}/common/modp_b64.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/common/platform.mm
    ${CMAKE_CURRENT_SOURCE_DIR}/common/string_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dev/print.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/memory/Array_Editor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/memory/CharKeys.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/memory/CharKeys_Editor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/memory/File.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/memory/File_Editor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/memory/FreeList.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/memory/FreeList_Editor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/memory/FSA.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/memory/FSA_Editor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/memory/GLTFLoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/memory/Gobj.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/memory/Gobj_Editor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/memory/MemMan.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/memory/MemMan_Block.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/memory/MemMan_BlockInfo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/memory/MemMan_Create.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/memory/MemMan_Editor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/memory/Pool_Editor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/render/CameraControl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/render/RenderSystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/worker/Worker.cpp
    ${SetupLib_sources}
)
if(DEV_INTERFACE)
    target_sources(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/dev/Editor.cpp
        # ${CMAKE_CURRENT_SOURCE_DIR}/dev/OriginWidget.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/dev/print.cpp
    )
endif()

include(Utils)
set(shaders_root "${CMAKE_CURRENT_SOURCE_DIR}/shader/shaders")
subdirlist(shader_dirs "${shaders_root}")
message(STATUS "shader_dirs ${shader_dirs}")
foreach(dir_name ${shader_dirs})
    bgfx_compile_shader_to_header(
        TYPE VERTEX
        SHADERS         ${shaders_root}/${dir_name}/vs_${dir_name}.sc
        VARYING_DEF     ${shaders_root}/${dir_name}/varying.def.sc
        OUTPUT_DIR      ${shaders_root}/${dir_name}
        INCLUDE_DIRS    ${shaders_root}/${dir_name}
                        ${BGFX_DIR}/src
    )
    bgfx_compile_shader_to_header(
        TYPE FRAGMENT
        SHADERS         ${shaders_root}/${dir_name}/fs_${dir_name}.sc
        VARYING_DEF     ${shaders_root}/${dir_name}/varying.def.sc
        OUTPUT_DIR      ${shaders_root}/${dir_name}
        INCLUDE_DIRS    ${shaders_root}/${dir_name}
                        ${BGFX_DIR}/src
    )
    target_sources(${PROJECT_NAME} PRIVATE ${shaders_root}/${dir_name}/vs_${dir_name}.sc)
    target_sources(${PROJECT_NAME} PRIVATE ${shaders_root}/${dir_name}/fs_${dir_name}.sc)
endforeach()

# CONFIGURE MAIN BINARY
set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY COMPILE_WARNING_AS_ERROR ON)
set_property(TARGET ${PROJECT_NAME} APPEND_STRING PROPERTY COMPILE_FLAGS ${SetupLib_flags})
target_link_libraries(${PROJECT_NAME} ${SetupLib_libs})
target_include_directories(${PROJECT_NAME} PUBLIC ${SetupLib_include_dirs})
target_compile_definitions(${PROJECT_NAME} PUBLIC ENABLE_IMGUI=${ENABLE_IMGUI})
target_compile_definitions(${PROJECT_NAME} PUBLIC DEV_INTERFACE=${DEV_INTERFACE})
target_compile_definitions(${PROJECT_NAME} PUBLIC FORCE_OPENGL=${FORCE_OPENGL})
# add_dependencies(${PROJECT_NAME} BGFXShader_engine_target)
