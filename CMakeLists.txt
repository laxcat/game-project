# SETUP
cmake_minimum_required(VERSION 3.24)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Utils)

# PROJECT
project(game_project_example VERSION 0.1.0 LANGUAGES CXX C OBJCXX OBJC)

set(CMAKE_BUILD_TYPE "")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(FORCE_OPENGL OFF)

if(NOT DEFINED BUILD_TYPE)
    set(BUILD_TYPE Release)
endif()

if(NOT DEFINED DEV_INTERFACE)
    set(DEV_INTERFACE 1)
endif()

set(CMAKE_SKIP_INSTALL_RULES ON QUIET)
if (NOT DEFINED BX_SILENCE_DEBUG_OUTPUT)
    set(BX_SILENCE_DEBUG_OUTPUT ON)
endif()

# MAIN ENGINE
set(SetupLib_include_dirs)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/engine")

# EXE
include_directories(${SetupLib_include_dirs})
set(EXE_NAME game_project)
add_executable(${EXE_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
target_compile_definitions(${EXE_NAME} PUBLIC DEV_INTERFACE=${DEV_INTERFACE})
target_build_type(${EXE_NAME} PUBLIC ${BUILD_TYPE})
target_link_libraries(${EXE_NAME} "game_project_engine" ${SetupLib_libs})
