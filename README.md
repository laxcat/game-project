# Game Project

## Overview

Basic template for a game project.

### General Features:

* Custom renderer using BGFX
	* Directional lights
	* Point lights
	* Distance fog
	* Psudo-physically-based rendering. (Blinn-Phong + simple adjustments based on metallicism/roughness)
* Custom memory manager
* Custom GLTF loading (text and binary)
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

#### Keyboard Shortcuts
|Key|Function|
|---|---|
|0|Hide debug overlay|
|1|Show dear imgui debug overlay|
|2|Show bgfx stats debug overlay|
|3|Show bgfx debgug text output debug overlay|
|4|Show wireframes|

## Build Instructions

Setting `DEV_INTERFACE` to 0 (default) will cull the libraries only used for the developer interface.

```bash
mkdir build
cd build
cmake .. -DDEV_INTERFACE=1
make
./IC_Dev_Desktop

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
