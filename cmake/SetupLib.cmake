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
    if(${PkgConfig_FOUND})
        pkg_check_modules(EGL REQUIRED egl)
        list(APPEND SetupLib_libs ${EGL_LIBRARIES})

        # look for gles first, then opengl
        pkg_check_modules(GLESV2 glesv2)
        if(${GLESV2_FOUND})
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
    message(STATUS "--------------------------------------------------------------------------------")
    message(STATUS "SETUP BGFX (bgfx.cmake)")

    set(BGFX_AMALGAMATED            ON  CACHE BOOL "" FORCE)
    set(BGFX_BUILD_EXAMPLES         OFF CACHE BOOL "" FORCE)
    set(BGFX_BUILD_TOOLS            ON  CACHE BOOL "" FORCE)
    set(BGFX_BUILD_TOOLS_BIN2C      OFF CACHE BOOL "" FORCE)
    set(BGFX_BUILD_TOOLS_GEOMETRY   OFF CACHE BOOL "" FORCE)
    set(BGFX_BUILD_TOOLS_SHADER     ON  CACHE BOOL "" FORCE)
    set(BGFX_BUILD_TOOLS_TEXTURE    OFF CACHE BOOL "" FORCE)
    set(BGFX_CUSTOM_TARGETS         ON  CACHE BOOL "" FORCE)
    set(BGFX_INSTALL                OFF CACHE BOOL "" FORCE)
    set(BX_AMALGAMATED              ON  CACHE BOOL "" FORCE)
    set(BX_CONFIG_DEBUG             ON  CACHE BOOL "" FORCE)

    set(BGFX_CMAKE_DIR "${SetupLib_LOCAL_REPO_DIR}/bgfx.cmake")
    if(EXISTS "${BGFX_CMAKE_DIR}")
        message(STATUS "Using local BGFX ${BGFX_CMAKE_DIR}")

        set(BGFX_DIR "${SetupLib_LOCAL_REPO_DIR}/bgfx")
        set(BX_DIR   "${SetupLib_LOCAL_REPO_DIR}/bx")
        set(BIMG_DIR "${SetupLib_LOCAL_REPO_DIR}/bimg")

        add_subdirectory("${BGFX_CMAKE_DIR}" bgfx)

    else()
        set(BGFX_CMAKE_DIR "https://github.com/bkaradzic/bgfx.cmake")
        message(STATUS "Using remote BGFX ${BGFX_CMAKE_DIR}")
        FetchContent_Declare(
            bgfx_content
            GIT_REPOSITORY "${BGFX_CMAKE_DIR}"
            # arbitrary version, captured Sept2023
            # https://github.com/bkaradzic/bgfx.cmake/releases/tag/v1.122.8572-455
            GIT_TAG 6a8aba37fff38dee714b81f27e542c12522917b7
        )
        FetchContent_MakeAvailable(bgfx_content)
    endif()

    list(APPEND SetupLib_include_dirs "${BGFX_DIR}/include")
    list(APPEND SetupLib_include_dirs "${BX_DIR}/include")
    list(APPEND SetupLib_include_dirs "${BIMG_DIR}/include")

    if(BX_SILENCE_DEBUG_OUTPUT)
        target_compile_definitions(bx PUBLIC BX_SILENCE_DEBUG_OUTPUT=1)
    endif()

    target_compile_options(bgfx PUBLIC
        -Wno-tautological-compare
        # -Wno-deprecated-declarations
    )
    target_build_type(bgfx PUBLIC ${BUILD_TYPE})
    target_build_type(bx   PUBLIC ${BUILD_TYPE})
    target_build_type(bimg PUBLIC ${BUILD_TYPE})

    list(APPEND SetupLib_libs bgfx bimg_decode)
endmacro()


# ---------------------------------------------------------------------------- #
# GLFW
# ---------------------------------------------------------------------------- #
macro(SetupLib_glfw)
    message(STATUS "--------------------------------------------------------------------------------")
    message(STATUS "SETUP GLFW")
    set(GLFW_DIR "${SetupLib_LOCAL_REPO_DIR}/glfw")
    if(EXISTS "${GLFW_DIR}")
        message(STATUS "Using local GLFW ${GLFW_DIR}")
        add_subdirectory("${GLFW_DIR}" glfw)
    else()
        set(GLFW_DIR "https://github.com/glfw/glfw")
        message(STATUS "Using remote GLFW ${GLFW_DIR}")
        FetchContent_Declare(
            glfw_content
            GIT_REPOSITORY https://github.com/glfw/glfw.git
            # arbitrary version, captured Sept.2022
            # https://github.com/glfw/glfw/releases/tag/3.3.8
            GIT_TAG 7482de6071d21db77a7236155da44c172a7f6c9e
        )
        set(GLFW_BUILD_DOCS         OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_TESTS        OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_EXAMPLES     OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(glfw_content)
    endif()
    target_build_type(glfw PUBLIC ${BUILD_TYPE})
    list(APPEND SetupLib_libs glfw)
endmacro()


# ---------------------------------------------------------------------------- #
# GLM
# ---------------------------------------------------------------------------- #
macro(SetupLib_glm)
    message(STATUS "--------------------------------------------------------------------------------")
    message(STATUS "SETUP GLM")
    set(GLM_DIR "${SetupLib_LOCAL_REPO_DIR}/glm")
    if(EXISTS "${GLM_DIR}")
        message(STATUS "Using local GLM ${GLM_DIR}")
        add_subdirectory("${GLM_DIR}" glm)
    else()
        set(GLM_DIR "https://github.com/g-truc/glm")
        message(STATUS "Using remote GLM ${GLM_DIR}")
        FetchContent_Declare(
            glm_content
            GIT_REPOSITORY "${GLM_DIR}"
            # arbitrary version, captured Oct.2021
            # https://github.com/g-truc/glm/releases/tag/0.9.9.8
            GIT_TAG bf71a834948186f4097caa076cd2663c69a10e1e
        )
        FetchContent_MakeAvailable(glm_content)
    endif()
    target_build_type(glm INTERFACE ${BUILD_TYPE})
    list(APPEND SetupLib_libs glm)
endmacro()


# ---------------------------------------------------------------------------- #
# IMGUI
# ---------------------------------------------------------------------------- #
# TODO: configure other renderers. assuming BGFX for now.
macro(SetupLib_imgui)
    message(STATUS "--------------------------------------------------------------------------------")
    message(STATUS "SETUP IMGUI")
    set(IMGUI_DIR "${SetupLib_LOCAL_REPO_DIR}/imgui")
    if(EXISTS "${IMGUI_DIR}")
        message(STATUS "Using local Dear ImGui ${IMGUI_DIR}")
        set(imgui_content_SOURCE_DIR "${IMGUI_DIR}")
    else()
        set(IMGUI_DIR "https://github.com/ocornut/imgui")
        message(STATUS "Using remote Dear ImGui ${IMGUI_DIR}")
        FetchContent_Declare(
            imgui_content
            GIT_REPOSITORY "${IMGUI_DIR}"
            # 1.89.5, captured May.2023
            # https://github.com/ocornut/imgui/releases/tag/v1.89.5
            GIT_TAG 1ebb91382757777382b3629ced2a573996e46453
        )
        FetchContent_MakeAvailable(imgui_content)
    endif()
    include_directories(${imgui_content_SOURCE_DIR})
    list(APPEND SetupLib_include_dirs "${imgui_content_SOURCE_DIR}")
    list(APPEND SetupLib_sources
        "${imgui_content_SOURCE_DIR}/imgui.cpp"
        "${imgui_content_SOURCE_DIR}/imgui_draw.cpp"
        "${imgui_content_SOURCE_DIR}/imgui_tables.cpp"
        "${imgui_content_SOURCE_DIR}/imgui_widgets.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/common/imgui_bgfx_glfw/imgui_bgfx_glfw.cpp"
    )

    message(STATUS "SETUP IMGUI MEMORY EDITOR")
    set(IMGUI_CLUB_DIR "${SetupLib_LOCAL_REPO_DIR}/imgui_club")
    if(EXISTS "${IMGUI_CLUB_DIR}")
        message(STATUS "Using local Dear ImGui Club ${IMGUI_CLUB_DIR}")
        set(imgui_memory_editor_content_SOURCE_DIR "${IMGUI_CLUB_DIR}")
    else()
        message(STATUS "Using remote Dear ImGui Club ${IMGUI_CLUB_DIR}")
        set(IMGUI_CLUB_DIR "https://github.com/ocornut/imgui_club")
        FetchContent_Declare(
            imgui_memory_editor_content
            GIT_REPOSITORY "${IMGUI_CLUB_DIR}"
            # captured Feb.2023
            GIT_TAG d4cd9896e15a03e92702a578586c3f91bbde01e8
        )
        FetchContent_MakeAvailable(imgui_memory_editor_content)
    endif()
    include_directories(${imgui_memory_editor_content_SOURCE_DIR})
    list(APPEND SetupLib_include_dirs "${imgui_memory_editor_content_SOURCE_DIR}")
endmacro()


# ---------------------------------------------------------------------------- #
# NATIVE FILE DIALOG
# ---------------------------------------------------------------------------- #
macro(SetupLib_nativefiledialog)
    message(STATUS "--------------------------------------------------------------------------------")
    message(STATUS "SETUP NATIVE FILE DIALOG")
    set(NFD_DIR "${SetupLib_LOCAL_REPO_DIR}/nativefiledialog")
    if(EXISTS "${NFD_DIR}")
        message(STATUS "Using local NativeFileDialog ${NFD_DIR}")
        set(nfd_content_SOURCE_DIR "${NFD_DIR}")
    else()
        set(NFD_DIR "https://github.com/mlabbe/nativefiledialog")
        message(STATUS "Using remote NativeFileDialog ${NFD_DIR}")
        FetchContent_Declare(
            nfd_content
            GIT_REPOSITORY "${NFD_DIR}"
            # arbitrary, captured Dec.2021
            # https://github.com/mlabbe/nativefiledialog/releases/tag/release_116
            GIT_TAG 67345b80ebb429ecc2aeda94c478b3bcc5f7888e
        )
        FetchContent_MakeAvailable(nfd_content)
    endif()
    include_directories(${nfd_content_SOURCE_DIR}/src/include)
    list(APPEND SetupLib_include_dirs "${nfd_content_SOURCE_DIR}/src/include")
    list(APPEND SetupLib_sources ${nfd_content_SOURCE_DIR}/src/nfd_common.c)
    if(APPLE)
        set(nfd_cocoa_file "${nfd_content_SOURCE_DIR}/src/nfd_cocoa.m")
        set_source_files_properties("${nfd_cocoa_file}" PROPERTIES COMPILE_FLAGS -Wno-deprecated-declarations)
        list(APPEND SetupLib_sources "${nfd_cocoa_file}")
    elseif(UNIX AND NOT APPLE)
        find_package(GTK2)
        list(APPEND SetupLib_libs           "${GTK2_LIBRARIES}")
        list(APPEND SetupLib_include_dirs   "${GTK2_INCLUDE_DIRS}")
        list(APPEND SetupLib_sources        "${nfd_content_SOURCE_DIR}/src/nfd_gtk.c")
    endif()
endmacro()


# ---------------------------------------------------------------------------- #
# RAPID JSON
# ---------------------------------------------------------------------------- #
macro(SetupLib_rapidjson)
    message(STATUS "--------------------------------------------------------------------------------")
    message(STATUS "SETUP RAPID JSON")

    set(RAPIDJSON_BUILD_DOC                 OFF CACHE BOOL "" FORCE)
    set(RAPIDJSON_BUILD_EXAMPLES            OFF CACHE BOOL "" FORCE)
    set(RAPIDJSON_BUILD_TESTS               OFF CACHE BOOL "" FORCE)
    set(RAPIDJSON_BUILD_THIRDPARTY_GTEST    OFF CACHE BOOL "" FORCE)
    set(RAPIDJSON_BUILD_CXX11               ON  CACHE BOOL "" FORCE)
    set(RAPIDJSON_BUILD_CXX17               ON  CACHE BOOL "" FORCE)

    set(RAPIDJSON_DIR "${SetupLib_LOCAL_REPO_DIR}/rapidjson")
    if(EXISTS "${RAPIDJSON_DIR}")
        message(STATUS "Using local RapidJSON ${RAPIDJSON_DIR}")
        add_subdirectory("${RAPIDJSON_DIR}" rapidjson)
        set(rapidjason_content_SOURCE_DIR "${RAPIDJSON_DIR}")
    else()
        set(RAPIDJSON_DIR "https://github.com/Tencent/rapidjson")
        message(STATUS "Using remote RapidJSON ${RAPIDJSON_DIR}")
        FetchContent_Declare(
            rapidjason_content
            GIT_REPOSITORY "${RAPIDJSON_DIR}"
            # arbitrary, captured Aug.2022
            # https://github.com/Tencent/rapidjson/releases/tag/v1.1.0
            GIT_TAG f54b0e47a08782a6131cc3d60f94d038fa6e0a51
        )
        FetchContent_MakeAvailable(rapidjason_content)
    endif()
    target_build_type(RapidJSON INTERFACE ${BUILD_TYPE})
    include_directories(${rapidjason_content_SOURCE_DIR}/include)
    list(APPEND SetupLib_include_dirs "${rapidjason_content_SOURCE_DIR}/include")
endmacro()


# ---------------------------------------------------------------------------- #
# STB
# ---------------------------------------------------------------------------- #
macro(SetupLib_stb)
    message(STATUS "--------------------------------------------------------------------------------")
    message(STATUS "SETUP STB")
    set(STB_DIR "${SetupLib_LOCAL_REPO_DIR}/stb")
    if(EXISTS "${STB_DIR}")
        message(STATUS "Using local stb ${STB_DIR}")
        set(stb_content_SOURCE_DIR "${STB_DIR}")
    else()
        set(STB_DIR "https://github.com/nothings/stb")
        message(STATUS "Using local stb ${STB_DIR}")
        FetchContent_Declare(
            stb_content
            GIT_REPOSITORY "${STB_DIR}"
            # arbitrary, captured Sept.2023
            # https://github.com/nothings/stb/commit/5736b15f7ea0ffb08dd38af21067c314d6a3aae9
            GIT_TAG 5736b15f7ea0ffb08dd38af21067c314d6a3aae9
        )
        FetchContent_MakeAvailable(stb_content)
    endif()
    include_directories(${stb_content_SOURCE_DIR})
    list(APPEND SetupLib_include_dirs "${stb_content_SOURCE_DIR}")
endmacro()


