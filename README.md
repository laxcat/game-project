# Game Project

## Overview

Basic template for a game project.

### General Features:

* Custom [memory manager]
* Custom [GLTF loading] (text and binary)
* Custom renderer using BGFX
	* Directional lights
	* Point lights
	* Distance fog
	* Psudo-physically-based rendering. (Blinn-Phong + simple adjustments based on metallicism/roughness)
* Dear ImGui interface
	* Control render settings like MSAA and vsync
	<!-- * Control directional and point lights -->
	* Control orbiting camera angle/target/fov
	<!-- * Visual widget for origin/axis visualization -->
	* GLTF loading, hot-swapping
		<!-- * Draw multiple instances -->
		<!-- * Edit material base colors -->
* Mouse drag and scroll for orbit and zoom camera control
* Keyboard shortcuts to enable debug overlays and animations (see below)
* Utilizes [bgfx_compile_shader_to_header] for compiling shaders
	* Automatically detects shader changes
	* Automatically prepares shaders for embedding into binary
* Custom CMake setup to download dependencies (uses [FetchContent] or optionally local repos)

#### Keyboard Shortcuts
|Key|Function|
|---|---|
|0|Hide debug overlay|
|1|Show dear imgui debug overlay|
|2|Show bgfx stats debug overlay|
|3|Show bgfx debgug text output debug overlay|
|4|Show wireframes|

## Supported Platforms

### Tested On:

* macOS (MBP M1 Max)

### Future Platforms:

* WebGL/WebGPU (emscripten)
* Windows
* Linux
* iOS/iPadOS/tvOS
* Android

## Build Instructions

Setting `DEV_INTERFACE` to 0 (default) will cull the libraries only used for the developer interface. See included [cmk] script for more available options.

```bash
mkdir build
cd build
cmake .. \
-DDEV_INTERFACE=1 \
-DSetupLib_LOCAL_REPO_DIR=/Example/Dev/SDKs
make
./game_project

```

## Additional Info

### Third Party

|Library|Author/Owner|License Type|Notes|
|---|---|---|---|
|[BGFX]|Branimir Karadzic|BSD 2-Clause "Simplified" License|Uses [BGFX flavor of GLSL] for shaders|
|[CMake]|Kitware|3-Clause BSD||
|[Dear_ImGui]|Omar Cornut|MIT|Desktop only|
|[GLFW]|Camilla Löwy|Zlib/libpng|Desktop only|
|[GLM]|g_truc|The Happy Bunny (Modified MIT)||
|[NativeFileDialog]|Michael Labbe|zlib|Desktop only|
|[OpenGL]|Khronos Group|None|
|[RapidJSON]|Milo Yip|BSD||
|[stb]|Sean Barrett|MIT|stb_image.h|

[memory manager]: <memory/MemMan.h>
[GLTF loading]: <memory/GLTFLoader.h>
[FetchContent]: <https://cmake.org/cmake/help/latest/module/FetchContent.html>
[bgfx_compile_shader_to_header]: <https://github.com/bkaradzic/bgfx.cmake#bgfx_compile_shader_to_header>
[cmk]: <build/cmk>

[BGFX]: <https://github.com/bkaradzic/bgfx>
[BGFX flavor of GLSL]: <https://bkaradzic.github.io/bgfx/tools.html#shader-compiler-shaderc>
[CMake]: <https://cmake.org/>
[Dear_ImGui]: <https://github.com/ocornut/imgui>
[GLFW]: <https://www.glfw.org/>
[GLM]: <https://github.com/g-truc/glm>
[NativeFileDialog]: <https://github.com/mlabbe/nativefiledialog>
[OpenGL]: <https://www.opengl.org/>
[RapidJSON]: <https://github.com/Tencent/rapidjson>
[stb]: <https://github.com/nothings/stb>
