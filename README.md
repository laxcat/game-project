# Game Project

Bare bones framework for a general PC game, using popular open source tools:
- glfw
- bgfx
- imgui
- entt
- glm

## Build Instructions
Currently: builds for macOS, shaders compile for Metal, tested on Mac mini (M1, 2020), building as x86_64. Other platforms should be trivial to add when they become a priority.

```bash
mkdir build
cd build
cmake ..
make
./GameProject

```
