# SetupLib
# Intended to be a repository of build configs for common external libraries 
# and dependencies.

# NOTES
# Using specific git hashes speeds up build. (Doesn't have to check remote.)
# Additionally, SetupLib_LOCAL_REPO_DIR to first look for local version of
# repos, potentially skipping download altogether.

include(FetchContent)
set(FETCHCONTENT_QUIET off)

# OUTPUT
set(SetupLib_libs "${SetupLib_libs}") # append lib names to this list
set(SetupLib_sources "${SetupLib_sources}") # append compile sources to this list
set(SetupLib_flag "${SetupLib_flag}") # append additional compile flags to this list
set(SetupLib_include_dirs "${SetupLib_include_dirs}") # append include_directories entries to this list


# ---------------------------------------------------------------------------- #
# OPENGL
# ---------------------------------------------------------------------------- #
macro(SetupLib_opengl)
    message(STATUS "SETUP OPENGL")
    if(APPLE)
        add_compile_definitions(BGFX_CONFIG_RENDERER_OPENGL=32)
    endif()
    find_package(PkgConfig)
    if (${PkgConfig_FOUND})
        pkg_check_modules(EGL REQUIRED egl)
        list(APPEND SetupLib_libs ${EGL_LIBRARIES})

        # look for gles first, then opengl
        pkg_check_modules(GLESV2 glesv2)
        if (${GLESV2_FOUND})
            list(APPEND SetupLib_libs ${GLESV2_LIBRARIES})
        elseif()
            pkg_check_modules(OPENGL REQUIRED opengl)
            list(APPEND SetupLib_libs ${OPENGL_LIBRARIES})
        endif()
    else()
        # Use official FindOpenGL.cmake
        find_package(OpenGL MODULE REQUIRED)
        list(APPEND SetupLib_libs ${OPENGL_gl_LIBRARY})
    endif()
endmacro()


# ---------------------------------------------------------------------------- #
# BGFX
# ---------------------------------------------------------------------------- #
macro(SetupLib_bgfx)
    message(STATUS "SETUP BGFX (bgfx.cmake)")

    set(BX_CONFIG_DEBUG             ON  CACHE BOOL "" FORCE)
    set(BGFX_BUILD_TOOLS            ON  CACHE BOOL "" FORCE)
    set(BGFX_BUILD_TOOLS_BIN2C      OFF CACHE BOOL "" FORCE)
    set(BGFX_BUILD_TOOLS_SHADER     ON  CACHE BOOL "" FORCE)
    set(BGFX_BUILD_TOOLS_GEOMETRY   OFF CACHE BOOL "" FORCE)
    set(BGFX_BUILD_TOOLS_TEXTURE    OFF CACHE BOOL "" FORCE)
    set(BGFX_BUILD_EXAMPLES         OFF CACHE BOOL "" FORCE)
    set(BGFX_INSTALL                OFF CACHE BOOL "" FORCE)
    set(BGFX_CUSTOM_TARGETS         ON  CACHE BOOL "" FORCE)

    set(BGFX_CMAKE_DIR "${SetupLib_LOCAL_REPO_DIR}/bgfx.cmake")
    if (EXISTS "${BGFX_CMAKE_DIR}")
        message(STATUS "Using local BGFX ${BGFX_CMAKE_DIR}")

        set(BGFX_DIR "${SetupLib_LOCAL_REPO_DIR}/bgfx")
        set(BX_DIR   "${SetupLib_LOCAL_REPO_DIR}/bx")
        set(BIMG_DIR "${SetupLib_LOCAL_REPO_DIR}/bimg")

        add_subdirectory("${BGFX_CMAKE_DIR}" "cmake-build")

    else()
        message(FATAL_ERROR "FetchContent not supported yet. Set SetupLib_LOCAL_REPO_DIR to directroy that contains bgfx/bx/bimg repos.")
        # set(BGFX_CMAKE_DIR "https://github.com/bkaradzic/bgfx.cmake")
        # message(STATUS "Using remote BGFX ${BGFX_CMAKE_DIR}")
        # FetchContent_Declare(
        #     bgfx_content
        #     GIT_REPOSITORY "${BGFX_CMAKE_DIR}"
        #     GIT_TAG        6a8aba37fff38dee714b81f27e542c12522917b7 # arbitrary, captured Sept2023, https://github.com/bkaradzic/bgfx.cmake/releases/tag/v1.122.8572-455
        # )
        # FetchContent_MakeAvailable(bgfx_content)
    endif()

    list(APPEND SetupLib_include_dirs "${BGFX_DIR}/include")
    list(APPEND SetupLib_include_dirs "${BX_DIR}/include")
    list(APPEND SetupLib_include_dirs "${BIMG_DIR}/include")

    if (BX_SILENCE_DEBUG_OUTPUT)
        target_compile_definitions(bx PUBLIC BX_SILENCE_DEBUG_OUTPUT=1)
    endif()

    set(SetupLib_shaderc "${CMAKE_CURRENT_BINARY_DIR}/cmake-build/cmake/bgfx/shaderc")

    # add_dependencies(BGFXShader_engine_target tools)

    list(APPEND SetupLib_libs bgfx bimg_decode)
endmacro()


# ---------------------------------------------------------------------------- #
# GLFW
# ---------------------------------------------------------------------------- #
macro(SetupLib_glfw)
    message(STATUS "SETUP GLFW")
    FetchContent_Declare(
        glfw_content
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        7d5a16ce714f0b5f4efa3262de22e4d948851525 # arbitrary, captured March2022, https://github.com/glfw/glfw/releases/tag/3.3.6
    )
    set(GLFW_BUILD_DOCS         OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS        OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES     OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(glfw_content)
    list(APPEND SetupLib_libs glfw)
endmacro()


# ---------------------------------------------------------------------------- #
# GLM
# ---------------------------------------------------------------------------- #
macro(SetupLib_glm)
    message(STATUS "SETUP GLM")
    FetchContent_Declare(
        glm_content
        GIT_REPOSITORY https://github.com/g-truc/glm
        GIT_TAG        bf71a834948186f4097caa076cd2663c69a10e1e # arbitrary, captured Oct.2021, https://github.com/g-truc/glm/releases/tag/0.9.9.8
    )
    FetchContent_MakeAvailable(glm_content)
    list(APPEND SetupLib_libs glm)
endmacro()


# ---------------------------------------------------------------------------- #
# IMGUI
# ---------------------------------------------------------------------------- #
# TODO: configure other renderers. assuming BGFX for now.
macro(SetupLib_imgui)
    message(STATUS "SETUP IMGUI")
    FetchContent_Declare(
        imgui_content
        GIT_REPOSITORY https://github.com/ocornut/imgui
        GIT_TAG        1ebb91382757777382b3629ced2a573996e46453 # 1.89.5, captured May.2023, https://github.com/ocornut/imgui/releases/tag/v1.89.5
    )
    FetchContent_MakeAvailable(imgui_content)
    include_directories(${imgui_content_SOURCE_DIR})
    list(APPEND SetupLib_include_dirs "${imgui_content_SOURCE_DIR}")
    list(APPEND SetupLib_sources
        "${imgui_content_SOURCE_DIR}/imgui.cpp"
        "${imgui_content_SOURCE_DIR}/imgui_draw.cpp"
        "${imgui_content_SOURCE_DIR}/imgui_tables.cpp"
        "${imgui_content_SOURCE_DIR}/imgui_widgets.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/common/imgui_bgfx_glfw/imgui_bgfx_glfw.cpp"
    )
    FetchContent_Declare(
        imgui_memory_editor_content
        GIT_REPOSITORY https://github.com/ocornut/imgui_club/
        GIT_TAG        d4cd9896e15a03e92702a578586c3f91bbde01e8 # captured Feb.2023
    )
    FetchContent_MakeAvailable(imgui_memory_editor_content)
    include_directories(${imgui_memory_editor_content_SOURCE_DIR})
    list(APPEND SetupLib_include_dirs "${imgui_memory_editor_content_SOURCE_DIR}")
endmacro()


# ---------------------------------------------------------------------------- #
# TINY GLTF
# ---------------------------------------------------------------------------- #
macro(SetupLib_tinygltf)
    message(STATUS "SETUP TINY GLTF")
    FetchContent_Declare(
        tinygltf_content
        GIT_REPOSITORY https://github.com/syoyo/tinygltf
        GIT_TAG        6ed7c39d71b81b1454238c72dd2d3c3380c8ef36 # arbitrary, captured March2022, https://github.com/syoyo/tinygltf/commit/6ed7c39d71b81b1454238c72dd2d3c3380c8ef36
    )
    set(TINYGLTF_BUILD_LOADER_EXAMPLE OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(tinygltf_content)
    include_directories(${tinygltf_content_SOURCE_DIR})
    list(APPEND SetupLib_include_dirs "${tinygltf_content_SOURCE_DIR}")
    list(APPEND SetupLib_sources "${SetupLib_FILES_PATH}/tiny_gltf/tiny_gltf.cpp")
endmacro()


# ---------------------------------------------------------------------------- #
# NATIVE FILE DIALOG
# ---------------------------------------------------------------------------- #
macro(SetupLib_nativefiledialog)
    message(STATUS "SETUP NATIVE FILE DIALOG")
    FetchContent_Declare(
        nativefiledialog_content
        GIT_REPOSITORY https://github.com/mlabbe/nativefiledialog
        GIT_TAG        67345b80ebb429ecc2aeda94c478b3bcc5f7888e # arbitrary, captured Dec.2021, https://github.com/mlabbe/nativefiledialog/releases/tag/release_116
    )
    FetchContent_MakeAvailable(nativefiledialog_content)
    include_directories(${nativefiledialog_content_SOURCE_DIR}/src/include)
    list(APPEND SetupLib_include_dirs "${nativefiledialog_content_SOURCE_DIR}/src/include")
    list(APPEND SetupLib_sources ${nativefiledialog_content_SOURCE_DIR}/src/nfd_common.c)
    if(APPLE)
        list(APPEND SetupLib_sources "${nativefiledialog_content_SOURCE_DIR}/src/nfd_cocoa.m")
    elseif(UNIX AND NOT APPLE)
        find_package(GTK2)
        list(APPEND SetupLib_libs           ${GTK2_LIBRARIES})
        list(APPEND SetupLib_include_dirs   ${GTK2_INCLUDE_DIRS})
        list(APPEND SetupLib_sources        "${nativefiledialog_content_SOURCE_DIR}/src/nfd_gtk.c")
    endif()
endmacro()


# ---------------------------------------------------------------------------- #
# RAPID JSON
# ---------------------------------------------------------------------------- #
macro(SetupLib_rapidjson)
    message(STATUS "SETUP RAPID JSON")
    FetchContent_Declare(
        rapidjason_content
        GIT_REPOSITORY https://github.com/Tencent/rapidjson
        GIT_TAG        f54b0e47a08782a6131cc3d60f94d038fa6e0a51 # arbitrary, captured Aug.2022, https://github.com/Tencent/rapidjson/releases/tag/v1.1.0
    )
    set(RAPIDJSON_BUILD_DOC                 OFF CACHE BOOL "" FORCE)
    set(RAPIDJSON_BUILD_EXAMPLES            OFF CACHE BOOL "" FORCE)
    set(RAPIDJSON_BUILD_TESTS               OFF CACHE BOOL "" FORCE)
    set(RAPIDJSON_BUILD_THIRDPARTY_GTEST    OFF CACHE BOOL "" FORCE)
    set(RAPIDJSON_BUILD_CXX11               ON  CACHE BOOL "" FORCE)
    set(RAPIDJSON_BUILD_CXX17               ON  CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(rapidjason_content)
    include_directories(${rapidjason_content_SOURCE_DIR}/include)
    list(APPEND SetupLib_include_dirs "${rapidjason_content_SOURCE_DIR}/include")
endmacro()


# ---------------------------------------------------------------------------- #
# STB
# ---------------------------------------------------------------------------- #
macro(SetupLib_stb)
    message(STATUS "SETUP STB")
    FetchContent_Declare(
        stb_content
        GIT_REPOSITORY https://github.com/nothings/stb
        GIT_TAG        5736b15f7ea0ffb08dd38af21067c314d6a3aae9 # arbitrary, captured Sept.2023, https://github.com/nothings/stb/commit/5736b15f7ea0ffb08dd38af21067c314d6a3aae9
    )
    FetchContent_MakeAvailable(stb_content)
    include_directories(${stb_content_SOURCE_DIR})
    list(APPEND SetupLib_include_dirs "${stb_content_SOURCE_DIR}")
endmacro()


