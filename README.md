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
 
### Desktop Prototype Features:

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

## Library Build Instructions

### Prerequisites

From a clean install of Ubuntu 20.04, the following needed to be installed first. MacOS needs to have Xcode and developer tools installed.

```bash
sudo apt install libglu1-mesa-dev cmake clang git
```
Once basic build tools are installed:

```bash
mkdir build
cd build
cmake ../lib
make
```

Output:

```
assets
├── shaders
│   ├── essl
│   │   ├── fs_gltf.bin
│   │   ├── fs_unlit.bin
│   │   ├── vs_gltf.bin
│   │   └── vs_unlit.bin
│   └── glsl
│       ├── fs_gltf.bin
│       ├── fs_unlit.bin
│       ├── vs_gltf.bin
│       └── vs_unlit.bin
├── rav4.glb
├── sedan.glb
└── truck.glb
libgame_project_engine.a
shaderc
```

* `assets/shaders/essl` OpenGL ES 2 versions of the shaders (glsl version: 100es)
* `assets/shaders/glsl` OpenGL versions of the shaders (glsl version: 150)
* `assets/*.glb` files are copied from /assets during build.
* `libgame_project_engine.a` is our main library.
* `shaderc` See below.

### Shaders and `shaderc`

The build process needs a BGFX utility `shaderc` to compile the shaders. This is built during the build process. It then compiles the shaders into bytecode at build-time, and the shader bytecode is loaded at run-time.

### Build status:

* Builds for Ubuntu 20.04 (vm, arm64), but library result is untested.
* Builds for Ubuntu 20.04 (vm, x86_64), but library restult is untested.
* Builds for MacOS 12.2 (Apple M1 Max), library is tested and works in MacOS desktop version.


## Desktop Prototype Build Instructions

The destop version builds the library above as a dependency, then utilizes that static libray in it's own build process. It adds a developer interface (Dear ImGUI), a windowing system (GLFW), and mouse and keyboard input.

### Prerequisites

The desktop version had some extra prerequisites. On a clean Ubuntu 20.04 install, GLFW detected that I was using X windowing system and asked for the following to be installed first. It is not tested on a system using Wayland, which will likely have its own prequisites.

```bash
sudo apt install libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
```

The GLTFs can be swapped out by the user in the developer interface. This required a file-picker dialog, which makes GTK a prerequisite.

```bash
sudo apt install libgtk2.0-dev
```

### Build

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
