#include "RenderManager.h"
#include "../MrManager.h"


void RenderManager::init() {
    gltfProgram = mm.mem.loadProgram("vs_tester", "fs_tester");
}
