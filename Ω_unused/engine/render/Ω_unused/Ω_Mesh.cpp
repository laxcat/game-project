#include "Mesh.h"
#include "../MrManager.h"


Material & Mesh::getMaterial() const {
    if (materialId == -1) return Renderable::DefaultMaterial;
    return mm.rendSys.at(renderableKey)->materials[materialId];
}
