# Game Project

Bare bones framework for a general PC game, using some popular open source tools. Experimentation zone for finding and implementing more tools. See [SetupLib.cmake](cmake/SetupLib.cmake) for supported libraries.

## Build Instructions

```bash
mkdir build
cd build
cmake ..
make
./GameProject

```
Currently: builds for macOS, shaders compile for Metal, tested on Mac mini (M1, 2020), building as x86_64. Other platforms should be trivial to add when they become a priority.
