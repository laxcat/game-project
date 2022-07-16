# Game Project

## Overview

Basic template for a game project.

### General Features:

* Custom BGFX renderer
	* Directional lights
	* Point lights
	* Distance fog
	* Psudo-physically-based rendering. (Blinn-Phong + simple adjustments based on metallicism/roughness)
* GLTF loading (text and binary)
* Dear ImGui interface
	* Control render settings like MSAA and vsync
	* Control directional and point lights
	* Control orbiting camera angle/target/fov
	* Visual widget for origin/axis visualization
	* Hot-swapable GLTF loading
	* Adjust distance fog
	* Adjust color of background
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

Setting `DEV_INTERFACE` to 0 (or omitting) will cull all developer interfaces and user input.

```bash
mkdir build
cd build
cmake .. -DDEV_INTERFACE=1
make
./IC_Dev_Desktop

```

## Additional Info

### Third Party

|Library|Author|License Type|Notes|
|---|---|---|---|
|[OpenGL]|Khronos Group|None|
|[CMake]|Kitware|3-Clause BSD||
|[BGFX]|Branimir Karadzic|BSD 2-Clause "Simplified" License||
|[GLFW]|Camilla Löwy|Zlib/libpng|Desktop only|
|[GLM]|g_truc|The Happy Bunny (Modified MIT)||
|[stb_image]|Sean Barrett|MIT||
|[tinygltf]|Syoyo Fujita|MIT||
|[Dear_ImGui]|Omar Cornut|MIT|Desktop only|
|[NativeFileDialog]|Michael Labbe|zlib|Desktop only|

[CMake]: <https://cmake.org/>
[GLFW]: <https://www.glfw.org/>
[OpenGL]: <https://www.opengl.org/>
[OpenGL ES]: <https://www.khronos.org/opengles/>
[stb_image]: <https://github.com/nothings/stb/blob/master/stb_image.h>
[GLM]: <https://github.com/g-truc/glm>
[BGFX]: <https://github.com/bkaradzic/bgfx>
[tinygltf]: <https://github.com/syoyo/tinygltf>
[Dear_ImGui]: <https://github.com/ocornut/imgui>
[NativeFileDialog]: <https://github.com/mlabbe/nativefiledialog>
[OpenGL context]: <https://www.khronos.org/opengl/wiki/OpenGL_Context>
[BGFX flavor of GLSL]: <https://bkaradzic.github.io/bgfx/tools.html#shader-compiler-shaderc>
