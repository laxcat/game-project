# OUTPUT
set(SetupLib_libs) # append lib names to this list
set(SetupLib_sources) # append compile sources to this list
set(SetupLib_flag) # append additional compile flags to this list


# OPENGL
macro(SetupLib_opengl)
find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})
list(APPEND SetupLib_libs ${OPENGL_gl_LIBRARY})
endmacro()


# BGFX
macro(SetupLib_bgfx)
FetchContent_Declare(
    bgfx_content
    GIT_REPOSITORY https://github.com/bkaradzic/bgfx.cmake
    GIT_TAG        v1.115.7918-39387a2 # arbitrary, captured Sept.2021
)
set(BGFX_BUILD_TOOLS        OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_EXAMPLES     OFF CACHE BOOL "" FORCE)
set(BGFX_INSTALL            OFF CACHE BOOL "" FORCE)
set(BGFX_CONFIG_DEBUG       OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(bgfx_content)
include_directories(${BX_DIR}/include)
list(APPEND SetupLib_libs bgfx)
endmacro()


# GLFW
macro(SetupLib_glfw)
FetchContent_Declare(
    glfw_content
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.3.4 # arbitrary, captured Sept.2021
)
set(GLFW_BUILD_DOCS         OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS        OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES     OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw_content)
list(APPEND SetupLib_libs glfw)
endmacro()


# GLM
macro(SetupLib_glm)
FetchContent_Declare(
    glm_content
    GIT_REPOSITORY https://github.com/g-truc/glm
    GIT_TAG        0.9.9.8 # arbitrary, captured Sept.2021
)
FetchContent_MakeAvailable(glm_content)
list(APPEND SetupLib_libs glm)
endmacro()


# ENTT
macro(SetupLib_entt)
FetchContent_Declare(
    entt_content
    GIT_REPOSITORY https://github.com/skypjack/entt
    GIT_TAG        v3.8.1 # arbitrary, captured Sept.2021
)
FetchContent_MakeAvailable(entt_content)
include_directories(${EnTT_SOURCE_DIR}/src)
list(APPEND SetupLib_libs EnTT)
endmacro()


# IMGUI
macro(SetupLib_imgui)
FetchContent_Declare(
    imgui_content
    GIT_REPOSITORY https://github.com/ocornut/imgui
)
FetchContent_MakeAvailable(imgui_content)
include_directories(${imgui_content_SOURCE_DIR})
list(APPEND SetupLib_sources
    ${imgui_content_SOURCE_DIR}/imgui.cpp
    ${imgui_content_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_content_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_content_SOURCE_DIR}/imgui_widgets.cpp
)
# list(APPEND SetupLib_flags "-fobjc-arc")
# BGFX + IMGUI
FetchContent_Declare(
    bgfx_imgui_content
    GIT_REPOSITORY https://github.com/pr0g/sdl-bgfx-imgui-starter
    GIT_TAG        origin/main
    SOURCE_SUBDIR  bgfx-imgui # avoid CMakeLists.txt in root
)
FetchContent_MakeAvailable(bgfx_imgui_content)
include_directories(${bgfx_imgui_content_SOURCE_DIR})
list(APPEND SetupLib_sources
    ${bgfx_imgui_content_SOURCE_DIR}/bgfx-imgui/imgui_impl_bgfx.cpp
)
endmacro()
