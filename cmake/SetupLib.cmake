# NOTES
# Using specific git hashes speeds up build. (Doesn't have to check remote.)

include(FetchContent)

# OUTPUT
set(SetupLib_libs) # append lib names to this list
set(SetupLib_sources) # append compile sources to this list
set(SetupLib_flag) # append additional compile flags to this list


# ---------------------------------------------------------------------------- #
# OPENGL
# ---------------------------------------------------------------------------- #
macro(SetupLib_opengl)
    message(STATUS "SETUP OPENGL")
    find_package(OpenGL REQUIRED)
    include_directories(${OPENGL_INCLUDE_DIR})
    list(APPEND SetupLib_libs ${OPENGL_gl_LIBRARY})
endmacro()


# ---------------------------------------------------------------------------- #
# BGFX
# ---------------------------------------------------------------------------- #
macro(SetupLib_bgfx)
    message(STATUS "SETUP BGFX")
    FetchContent_Declare(
        bgfx_content
        GIT_REPOSITORY https://github.com/bkaradzic/bgfx.cmake
        GIT_TAG        e3b3cb5909882ec023cfe08fa52f106a2ddff08f # arbitrary, captured Oct.2021, https://github.com/bkaradzic/bgfx.cmake/releases/tag/v1.115.7924-e3b3cb5
    )
    set(BGFX_BUILD_TOOLS        ON  CACHE BOOL "" FORCE)
    set(BGFX_BUILD_EXAMPLES     OFF CACHE BOOL "" FORCE)
    set(BGFX_INSTALL            OFF CACHE BOOL "" FORCE)
    set(BGFX_CONFIG_DEBUG       OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(bgfx_content)
    # include_directories(${BGFX_DIR}/../) # the bgfx_content-src dir, so we can #include <bgfx/examples/...>
    # include_directories(${BGFX_DIR}/3rdparty/)
    include_directories(${BX_DIR}/include)
    include_directories(${BIMG_DIR}/include)
    list(APPEND SetupLib_libs bgfx)
endmacro()


# ---------------------------------------------------------------------------- #
# GLFW
# ---------------------------------------------------------------------------- #
macro(SetupLib_glfw)
    message(STATUS "SETUP GLFW")
    FetchContent_Declare(
        glfw_content
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        814b7929c5add4b0541ccad26fb81f28b71dc4d8 # arbitrary, captured Oct.2021, https://github.com/glfw/glfw/releases/tag/3.3.4
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
# ENTT
# ---------------------------------------------------------------------------- #
macro(SetupLib_entt)
    message(STATUS "SETUP ENTT")
    FetchContent_Declare(
        entt_content
        GIT_REPOSITORY https://github.com/skypjack/entt
        GIT_TAG        dd6863f71da1b360ec09c25912617a3423f08149 # arbitrary, captured Oct.2021, https://github.com/skypjack/entt/releases/tag/v3.8.1
    )
    FetchContent_MakeAvailable(entt_content)
    include_directories(${EnTT_SOURCE_DIR}/src)
    list(APPEND SetupLib_libs EnTT)
endmacro()


# ---------------------------------------------------------------------------- #
# CEREAL
# ---------------------------------------------------------------------------- #
macro(SetupLib_cereal)
    # Note: header only; nothing to "build"
    message(STATUS "SETUP CEREAL")
    FetchContent_Declare(
        cereal_content
        GIT_REPOSITORY https://github.com/USCiLab/cereal/
        GIT_TAG        02eace19a99ce3cd564ca4e379753d69af08c2c8 # arbitrary, captured Oct.2021, https://github.com/USCiLab/cereal/releases/tag/v1.3.0
        SOURCE_SUBDIR  do-nothing # avoid CMakeLists.txt in root
    )
    FetchContent_MakeAvailable(cereal_content)
    include_directories(${cereal_content_SOURCE_DIR}/include)
endmacro()


# ---------------------------------------------------------------------------- #
# IMGUI
# ---------------------------------------------------------------------------- #
macro(SetupLib_imgui)
    message(STATUS "SETUP IMGUI")
    FetchContent_Declare(
        imgui_content
        GIT_REPOSITORY https://github.com/ocornut/imgui
        GIT_TAG        e3e1fbcf025cf83413815751f7c33500e1314d57 # arbitrary, captured Oct.2021, https://github.com/ocornut/imgui/releases/tag/v1.84.2
    )
    FetchContent_MakeAvailable(imgui_content)
    include_directories(${imgui_content_SOURCE_DIR})
    list(APPEND SetupLib_sources
        ${imgui_content_SOURCE_DIR}/imgui.cpp
        ${imgui_content_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_content_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_content_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_content_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_content_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    )
    # list(APPEND SetupLib_flags "-fobjc-arc")
    # BGFX + IMGUI
    message(STATUS "SETUP BGFX+IMGUI")
    FetchContent_Declare(
        bgfx_imgui_content
        GIT_REPOSITORY https://github.com/pr0g/sdl-bgfx-imgui-starter
        GIT_TAG        1b7b8c917e3d9fbe7028766c960ab123eccaeb44 # arbitrary, captured Oct.2021, https://github.com/pr0g/sdl-bgfx-imgui-starter/commit/1b7b8c917e3d9fbe7028766c960ab123eccaeb44
        SOURCE_SUBDIR  bgfx-imgui # avoid CMakeLists.txt in root
    )
    FetchContent_MakeAvailable(bgfx_imgui_content)
    include_directories(${bgfx_imgui_content_SOURCE_DIR})
    list(APPEND SetupLib_sources
        ${bgfx_imgui_content_SOURCE_DIR}/bgfx-imgui/imgui_impl_bgfx.cpp
    )
endmacro()


# ---------------------------------------------------------------------------- #
# TINY GLTF
# ---------------------------------------------------------------------------- #
macro(SetupLib_tinygltf)
    message(STATUS "SETUP TINY GLTF")
    FetchContent_Declare(
        tinygltf_content
        GIT_REPOSITORY https://github.com/syoyo/tinygltf
        GIT_TAG        a159945db9d97e79a30cb34e2aaa45fd28dea576 # arbitrary, captured Oct.2021, https://github.com/syoyo/tinygltf/releases/tag/v2.5.0
    )
    FetchContent_MakeAvailable(tinygltf_content)
endmacro()
