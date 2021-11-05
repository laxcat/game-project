#include "RenderManager.h"
#include "../MrManager.h"


void RenderManager::init() {
    gltfProgram = mm.mem.loadProgram("vs_gltf", "fs_gltf");
}
